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

#include <KProcess>
#include <kconfiggroup.h>
#include <kicon.h>
#include <klocale.h>
#include <kshell.h>
#include <KMessageBox>
#include <KParts/MainWindow>

#include <outputview/outputmodel.h>
#include <interfaces/ilaunchconfiguration.h>
#include <util/environmentgrouplist.h>
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


#include "debugsession.h"
#include "xdebugplugin.h"
#include "connection.h"

namespace XDebug {

XDebugJob::XDebugJob( DebugSession* session, KDevelop::ILaunchConfiguration* cfg, QObject* parent)
    : KDevelop::OutputJob(parent), m_proc(0), m_session(session)
{
    setCapabilities(Killable);

    session->setLaunchConfiguration(cfg);

    IExecuteScriptPlugin* iface = KDevelop::ICore::self()->pluginController()->pluginForExtension("org.kdevelop.IExecuteScriptPlugin")->extension<IExecuteScriptPlugin>();
    Q_ASSERT(iface);

    KDevelop::EnvironmentGroupList l(KGlobal::config());
    QString envgrp = iface->environmentGroup(cfg);

    QString err;
    QString interpreter = iface->interpreter( cfg, err );

    if( !err.isEmpty() )
    {
        setError( -1 );
        setErrorText( err );
        return;
    }

    KUrl script = iface->script( cfg, err );

    if( !err.isEmpty() )
    {
        setError( -3 );
        setErrorText( err );
        return;
    }

    QString remoteHost = iface->remoteHost( cfg, err );
    if( !err.isEmpty() )
    {
        setError( -4 );
        setErrorText( err );
        return;
    }

    if( envgrp.isEmpty() )
    {
        kWarning() << "Launch Configuration:" << cfg->name() << i18n("No environment group specified, looks like a broken "
                       "configuration, please check run configuration '%1'. "
                       "Using default environment group.", cfg->name() );
        envgrp = l.defaultGroup();
    }

    QStringList arguments = iface->arguments( cfg, err );
    if( !err.isEmpty() )
    {
        setError( -2 );
        setErrorText( err );
    }

    if( error() != 0 )
    {
        kWarning() << "Launch Configuration:" << cfg->name() << "oops, problem" << errorText();
        return;
    }

    QString ideKey = "kdev"+QString::number(qrand());
    //TODO NIKO set ideKey to session?

    m_proc = new KProcess( this );

    m_lineMaker = new KDevelop::ProcessLineMaker( m_proc, this );

    setStandardToolView(KDevelop::IOutputView::RunView);
    setBehaviours(KDevelop::IOutputView::AllowUserClose | KDevelop::IOutputView::AutoScroll);
    setModel( new KDevelop::OutputModel(), KDevelop::IOutputView::TakeOwnership );

    connect( m_lineMaker, SIGNAL(receivedStdoutLines(const QStringList&)), model(), SLOT(appendLines(QStringList)) );
    connect( m_proc, SIGNAL(error(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)) );
    connect( m_proc, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(processFinished(int,QProcess::ExitStatus)) );

    QStringList env = l.createEnvironment( envgrp, m_proc->systemEnvironment());
    env << "XDEBUG_CONFIG=\"remote_enable=1 \"";
    m_proc->setEnvironment( env );

    KUrl wc = iface->workingDirectory( cfg );
    if( !wc.isValid() || wc.isEmpty() )
    {
        wc = KUrl( QFileInfo( script.toLocalFile() ).absolutePath() );
    }
    m_proc->setWorkingDirectory( wc.toLocalFile() );
    m_proc->setProperty( "executable", interpreter );

    QStringList program;
    if (!remoteHost.isEmpty()) {
        program << "ssh";
        program << remoteHost;
        program << "XDEBUG_CONFIG=\"remote_enable=1 \"";
    }
    program << interpreter;
    program << "-d xdebug.remote_enable=1";
    program << "-d xdebug.remote_port="+QString::number(9000);
    program << "-d xdebug.idekey="+ideKey;
    program << script.toLocalFile();
    program << arguments;

    
    kDebug() << "setting app:" << program;

    m_proc->setOutputChannelMode(KProcess::MergedChannels);

    m_proc->setProgram( program );

    setTitle(cfg->name());
    setStandardToolView(KDevelop::IOutputView::DebugView);

}

void XDebugJob::start()
{
    kDebug() << "launching?" << m_proc;
    if( m_proc )
    {
        m_session->listenForConnection();

        startOutput();
        appendLine( i18n("Starting: %1", m_proc->program().join(" ") ) );
        m_proc->start();
    } else
    {
        kWarning() << "No process, something went wrong when creating the job";
        // No process means we've returned early on from the constructor, some bad error happened
        emitResult();
    }
}

KProcess* XDebugJob::process() const
{
    return m_proc;
}


bool XDebugJob::doKill()
{
    kDebug();
    if (m_session) m_session->stopDebugger();
    return true;
}

void XDebugJob::processFinished( int exitCode , QProcess::ExitStatus status )
{
    m_lineMaker->flushBuffers();
    if (exitCode == 0 && status == QProcess::NormalExit)
        appendLine( i18n("*** Exited normally ***") );
    else
        if (status == QProcess::NormalExit)
            appendLine( i18n("*** Exited with return code: %1 ***", QString::number(exitCode)) );
        else
            if (error() == KJob::KilledJobError)
                appendLine( i18n("*** Process aborted ***") );
            else
                appendLine( i18n("*** Crashed with return code: %1 ***", QString::number(exitCode)) );
    kDebug() << "Process done";
    emitResult();

    if (m_session && m_session->connection()) {
        m_session->connection()->setState(DebugSession::EndedState);
    }
}

void XDebugJob::processError( QProcess::ProcessError error )
{
    if( error == QProcess::FailedToStart )
    {
        setError( -1 );
        QString errmsg =  i18n("Could not start program '%1'. Make sure that the "
                           "path is specified correctly.", m_proc->property("executable").toString() );
        KMessageBox::error( KDevelop::ICore::self()->uiController()->activeMainWindow(), errmsg, i18n("Could not start application") );
        setErrorText( errmsg );
        emitResult();
    }
    kDebug() << "Process error";

    if (m_session && m_session->connection()) {
        m_session->connection()->setState(DebugSession::EndedState);
    }
}

void XDebugJob::appendLine(const QString& l)
{
    if (KDevelop::OutputModel* m = model()) {
        m->appendLine(l);
    }
}

KDevelop::OutputModel* XDebugJob::model()
{
    return dynamic_cast<KDevelop::OutputModel*>( KDevelop::OutputJob::model() );
}


XDebugBrowserJob::XDebugBrowserJob(DebugSession* session, KDevelop::ILaunchConfiguration* cfg, QObject* parent)
    : KJob(parent), m_session(session)
{
    setCapabilities(Killable);

    session->setLaunchConfiguration(cfg);

    IExecuteBrowserPlugin* iface = KDevelop::ICore::self()->pluginController()
        ->pluginForExtension("org.kdevelop.IExecuteBrowserPlugin")->extension<IExecuteBrowserPlugin>();
    Q_ASSERT(iface);

    QString err;
    m_url = iface->url(cfg, err);
    if (!err.isEmpty()) {
        m_url.clear();
        setError( -1 );
        setErrorText( err );
        return;
    }

    setObjectName(cfg->name());

    connect(m_session, SIGNAL(finished()), SLOT(sessionFinished()));

    m_session->setAcceptMultipleConnections(true);
}

void XDebugBrowserJob::start()
{
    kDebug() << "launching?" << m_url;
    if (m_url.isValid()) {
        if (!m_session->listenForConnection()) {
            kWarning() << "listening for connection failed";
            emitResult();
            return;
        }
        KUrl url = m_url;
        url.addQueryItem("XDEBUG_SESSION_START", "kdev");
        if (!QDesktopServices::openUrl(url)) {
            kWarning() << "openUrl failed, something went wrong when creating the job";
            emitResult();
        }
    }
}


bool XDebugBrowserJob::doKill()
{
    kDebug();
    m_session->stopDebugger();
    KUrl url = m_url;
    url.addQueryItem("XDEBUG_SESSION_STOP_NO_EXEC", "kdev");
    QDesktopServices::openUrl(url);
    return true;
}


void XDebugBrowserJob::sessionFinished()
{
    emitResult();
}


}

#include "debugjob.moc"
