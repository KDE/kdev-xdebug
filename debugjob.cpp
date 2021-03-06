/*
 * XDebug Debugger Support
 *
 * Copyright 2006 Vladimir Prus <ghost@cs.msu.su>
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
 * Copyright 2009 Andreas Pakulat <apaku@gmx.de>
 * Copyright 2009 Niko Sams <niko.sams@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "debugjob.h"

#include <QFileInfo>
#include <QDesktopServices>

#include <QDebug>
#include <KProcess>
#include <KMessageBox>
#include <KParts/MainWindow>
#include <KLocalizedString>

#include <outputview/outputmodel.h>
#include <interfaces/ilaunchconfiguration.h>
#include <execute/iexecuteplugin.h>
#include <interfaces/iproject.h>
#include <project/interfaces/iprojectbuilder.h>
#include <project/builderjob.h>
#include <interfaces/iuicontroller.h>
#include <project/interfaces/ibuildsystemmanager.h>
#include <util/executecompositejob.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/icore.h>
#include <util/processlinemaker.h>
#include <executescript/iexecutescriptplugin.h>
#include <iexecutebrowserplugin.h>

#include <kdevplatform_version.h>
#if KDEVPLATFORM_VERSION < QT_VERSION_CHECK(5,1,40)
#include <util/environmentgrouplist.h>
#else
#include <util/environmentprofilelist.h>
#endif

#include "debugsession.h"
#include "xdebugplugin.h"
#include "connection.h"
#include "debuggerdebug.h"

namespace XDebug {
XDebugJob::XDebugJob(DebugSession* session, KDevelop::ILaunchConfiguration* cfg, QObject* parent)
    : KDevelop::OutputJob(parent)
    , m_proc(nullptr)
    , m_session(session)
{
    setCapabilities(Killable);

    session->setLaunchConfiguration(cfg);

    setObjectName(cfg->name());

    IExecuteScriptPlugin* iface = KDevelop::ICore::self()->pluginController()->pluginForExtension("org.kdevelop.IExecuteScriptPlugin")->extension<IExecuteScriptPlugin>();
    Q_ASSERT(iface);

#if KDEVPLATFORM_VERSION < QT_VERSION_CHECK(5,1,40)
    KDevelop::EnvironmentGroupList l(KSharedConfig::openConfig());
    QString envgrp = iface->environmentGroup(cfg);
#else
    KDevelop::EnvironmentProfileList l(KSharedConfig::openConfig());
    QString envgrp = iface->environmentProfileName(cfg);
#endif

    QString err;
    QString interpreter = iface->interpreter(cfg, err);

    if (!err.isEmpty()) {
        setError(-1);
        setErrorText(err);
        return;
    }

    QUrl script = iface->script(cfg, err);

    if (!err.isEmpty()) {
        setError(-3);
        setErrorText(err);
        return;
    }

    QString remoteHost = iface->remoteHost(cfg, err);
    if (!err.isEmpty()) {
        setError(-4);
        setErrorText(err);
        return;
    }

    if (envgrp.isEmpty()) {
        qCWarning(KDEV_PHP_DEBUGGER) << "Launch Configuration:" << cfg->name() << i18n("No environment group specified, looks like a broken "
                                                                     "configuration, please check run configuration '%1'. "
                                                                     "Using default environment group.", cfg->name());
#if KDEVPLATFORM_VERSION < QT_VERSION_CHECK(5,1,40)
        envgrp = l.defaultGroup();
#else
        envgrp = l.defaultProfileName();
#endif
    }

    QStringList arguments = iface->arguments(cfg, err);
    if (!err.isEmpty()) {
        setError(-2);
        setErrorText(err);
    }

    if (error() != 0) {
        qCWarning(KDEV_PHP_DEBUGGER) << "Launch Configuration:" << cfg->name() << "oops, problem" << errorText();
        return;
    }

    QString ideKey = "kdev" + QString::number(qrand());
    //TODO NIKO set ideKey to session?

    m_proc = new KProcess(this);

    m_lineMaker = new KDevelop::ProcessLineMaker(m_proc, this);

    setStandardToolView(KDevelop::IOutputView::RunView);
    setBehaviours(KDevelop::IOutputView::AllowUserClose | KDevelop::IOutputView::AutoScroll);
    KDevelop::OutputModel* m = new KDevelop::OutputModel();
    m->setFilteringStrategy(KDevelop::OutputModel::ScriptErrorFilter);
    setModel(m);

    connect(m_lineMaker, &KDevelop::ProcessLineMaker::receivedStdoutLines, model(),&KDevelop::OutputModel::appendLines);
    
#if QT_VERSION < 0x050600
    connect(m_proc, static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
#else
    connect(m_proc, &QProcess::errorOccurred,
#endif    
    this, &XDebugJob::processError);
    
    connect(m_proc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)> (&QProcess::finished), this, &XDebugJob::processFinished);

    QStringList env = l.createEnvironment(envgrp, m_proc->systemEnvironment());
    env << "XDEBUG_CONFIG=\"remote_enable=1 \"";
    m_proc->setEnvironment(env);

    QUrl wc = iface->workingDirectory(cfg);
    if (!wc.isValid() || wc.isEmpty()) {
        wc = QUrl(QFileInfo(script.toLocalFile()).absolutePath());
    }
    m_proc->setWorkingDirectory(wc.toLocalFile());
    m_proc->setProperty("executable", interpreter);

    QStringList program;
    if (!remoteHost.isEmpty()) {
        program << "ssh";
        QStringList parts = remoteHost.split(":");
        program << parts.first();
        if (parts.length() > 1) {
            program << "-p " + parts.at(1);
        }
        program << "XDEBUG_CONFIG=\"remote_enable=1 \"";
    }
    qCDebug(KDEV_PHP_DEBUGGER) << program;
    program << interpreter;
    program << "-d xdebug.remote_enable=1";
    QString remoteHostSetting = cfg->config().readEntry("RemoteHost", QString());
    int remotePortSetting = cfg->config().readEntry("RemotePort", 9000);
    if (!remoteHostSetting.isEmpty()) {
        program << "-d xdebug.remote_host=" + remoteHostSetting;
    }
    program << "-d xdebug.remote_port=" + QString::number(remotePortSetting);
    program << "-d xdebug.idekey=" + ideKey;
    program << script.toLocalFile();
    program << arguments;

    qCDebug(KDEV_PHP_DEBUGGER) << "setting app:" << program;

    m_proc->setOutputChannelMode(KProcess::MergedChannels);

    m_proc->setProgram(program);

    setTitle(cfg->name());
    setStandardToolView(KDevelop::IOutputView::DebugView);
}

void XDebugJob::start()
{
    qCDebug(KDEV_PHP_DEBUGGER) << "launching?" << m_proc;
    if (m_proc) {
        QString err;
        if (!m_session->listenForConnection(err)) {
            qCWarning(KDEV_PHP_DEBUGGER) << "listening for connection failed";
            setError(-1);
            setErrorText(err);
            emitResult();
            m_session->stopDebugger();
            return;
        }

        startOutput();
        qCDebug(KDEV_PHP_DEBUGGER) << "starting" << m_proc->program().join(" ");
        appendLine(i18n("Starting: %1", m_proc->program().join(" ")));
        m_proc->start();
    } else
    {
        qCWarning(KDEV_PHP_DEBUGGER) << "No process, something went wrong when creating the job";
        // No process means we've returned early on from the constructor, some bad error happened
        emitResult();
        m_session->stopDebugger();
    }
}

KProcess* XDebugJob::process() const
{
    return m_proc;
}

bool XDebugJob::doKill()
{
    qCDebug(KDEV_PHP_DEBUGGER);
    if (m_session) {
        m_session->stopDebugger();
    }
    return true;
}

void XDebugJob::processFinished(int exitCode, QProcess::ExitStatus status)
{
    m_lineMaker->flushBuffers();
    if (exitCode == 0 && status == QProcess::NormalExit) {
        appendLine(i18n("*** Exited normally ***"));
    } else
    if (status == QProcess::NormalExit) {
        appendLine(i18n("*** Exited with return code: %1 ***", QString::number(exitCode)));
    } else
    if (error() == KJob::KilledJobError) {
        appendLine(i18n("*** Process aborted ***"));
    } else {
        appendLine(i18n("*** Crashed with return code: %1 ***", QString::number(exitCode)));
    }
    qCDebug(KDEV_PHP_DEBUGGER) << "Process done";
    emitResult();
}

void XDebugJob::processError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        setError(-1);
        QString errmsg =  i18n("Could not start program '%1'. Make sure that the "
                               "path is specified correctly.", m_proc->property("executable").toString());
        setErrorText(errmsg);
        m_session->stopDebugger();
        emitResult();
    }
    qCDebug(KDEV_PHP_DEBUGGER) << "Process error";
}

void XDebugJob::appendLine(const QString& l)
{
    if (KDevelop::OutputModel* m = model()) {
        m->appendLine(l);
    }
}

KDevelop::OutputModel* XDebugJob::model()
{
    return dynamic_cast<KDevelop::OutputModel*>(KDevelop::OutputJob::model());
}

XDebugBrowserJob::XDebugBrowserJob(DebugSession* session, KDevelop::ILaunchConfiguration* cfg, QObject* parent)
    : KJob(parent)
    , m_session(session)
{
    setCapabilities(Killable);

    session->setLaunchConfiguration(cfg);

    setObjectName(cfg->name());

    IExecuteBrowserPlugin* iface = KDevelop::ICore::self()->pluginController()
                                   ->pluginForExtension("org.kdevelop.IExecuteBrowserPlugin")->extension<IExecuteBrowserPlugin>();
    Q_ASSERT(iface);

    QString err;
    m_url = iface->url(cfg, err);
    if (!err.isEmpty()) {
        m_url.clear();
        setError(-1);
        setErrorText(err);
        return;
    }
    m_browser = iface->browser(cfg);

    setObjectName(cfg->name());

    connect(m_session, &KDevelop::IDebugSession::finished,this, &XDebugBrowserJob::sessionFinished);

    m_session->setAcceptMultipleConnections(true);
}

void XDebugBrowserJob::start()
{
    qCDebug(KDEV_PHP_DEBUGGER) << "launching?" << m_url;
    if (!m_url.isValid()) {
        emitResult();
        return;
    }

    QString err;
    if (!m_session->listenForConnection(err)) {
        qCWarning(KDEV_PHP_DEBUGGER) << "listening for connection failed";
        setError(-1);
        setErrorText(err);
        emitResult();
        m_session->stopDebugger();
        return;
    }

    QUrl url = m_url;
    url.setQuery("XDEBUG_SESSION_START=kdev");

    launchBrowser(url);
}


void XDebugBrowserJob::processFailedToStart()
{
    qCWarning(KDEV_PHP_DEBUGGER) << "Cannot start application" << m_browser;
    setError(-1);
    QString errmsg =  i18n("Could not start program '%1'. Make sure that the "
                            "path is specified correctly.", m_browser);
    setErrorText(errmsg);
    emitResult();
    qCDebug(KDEV_PHP_DEBUGGER) << "Process error";
    m_session->stopDebugger();
}


void XDebugBrowserJob::launchBrowser(QUrl &url)
{
   if (m_browser.isEmpty()) {
        if (!QDesktopServices::openUrl(url)) {
            qCWarning(KDEV_PHP_DEBUGGER) << "openUrl failed, something went wrong when creating the job";
            emitResult();
            m_session->stopDebugger();
        }
    } else {
        KProcess proc(this);

        proc.setProgram(QStringList() << m_browser << url.url());
        if( !proc.startDetached() )
        {
            processFailedToStart();
        }
    }
}

bool XDebugBrowserJob::doKill()
{
    qCDebug(KDEV_PHP_DEBUGGER);
    m_session->stopDebugger();
    QUrl url = m_url;
    url.setQuery("XDEBUG_SESSION_STOP_NO_EXEC=kdev");
    launchBrowser(url);
    return true;
}

void XDebugBrowserJob::sessionFinished()
{
    emitResult();
}

}
