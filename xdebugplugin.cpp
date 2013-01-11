/*
 * XDebug Debugger Support
 *
 * Copyright 1999-2001 John Birch <jbb@kdevelop.org>
 * Copyright 2001 by Bernd Gehrmann <bernd@kdevelop.org>
 * Copyright 2006 Vladimir Prus <ghost@cs.msu.su>
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
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

#include "xdebugplugin.h"

#include <QDir>
#include <QToolTip>
#include <QByteArray>
#include <QTimer>
#include <QMenu>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QSignalMapper>

#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kstatusbar.h>
#include <kparts/part.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kpluginfactory.h>
#include <kaboutdata.h>
#include <KToolBar>
#include <KDialog>
#include <kwindowsystem.h>
#include <KXmlGuiWindow>
#include <KXMLGUIFactory>

#include <sublime/view.h>

#include <interfaces/icore.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/context.h>
#include <interfaces/idebugcontroller.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/launchconfigurationtype.h>
#include <executescript/iexecutescriptplugin.h>
#include <iexecutebrowserplugin.h>

#include "debugsession.h"
#include "launchconfig.h"
#include "version.h"

K_PLUGIN_FACTORY(KDevXDebugDebuggerFactory, registerPlugin<XDebug::XDebugPlugin>(); )
K_EXPORT_PLUGIN(KDevXDebugDebuggerFactory(KAboutData("kdevxdebug", "kdevxdebug", ki18n("XDebug Support"), KDEVXDEBUG_VERSION_STR, ki18n("Support for debugging PHP scripts in XDebug"), KAboutData::License_GPL)
    .addAuthor(ki18n("Niko Sams"), ki18n("Author"), "niko.sams@gmail.com", "http://nikosams.blogspot.com")
))


namespace XDebug
{

XDebugPlugin::XDebugPlugin( QObject *parent, const QVariantList & ) :
    KDevelop::IPlugin( KDevXDebugDebuggerFactory::componentData(), parent )
{
    core()->debugController()->initializeUi();

    kDebug();
//     connect(m_server, SIGNAL(sessionStarted(DebugSession*)), SLOT(sessionStarted(DebugSession*)));
//     connect(m_server, SIGNAL(outputLine(DebugSession*,QString,KDevelop::IRunProvider::OutputTypes)), SLOT(outputLine(DebugSession*,QString,KDevelop::IRunProvider::OutputTypes)));
//     connect(m_server, SIGNAL(stateChanged(DebugSession*,KDevelop::IDebugSession::DebuggerState)), SLOT(debuggerStateChanged(DebugSession*,KDevelop::IDebugSession::DebuggerState)));
    {
        IExecuteScriptPlugin* iface = KDevelop::ICore::self()->pluginController()->pluginForExtension("org.kdevelop.IExecuteScriptPlugin")->extension<IExecuteScriptPlugin>();
        Q_ASSERT(iface);
        KDevelop::LaunchConfigurationType* type = core()->runController()->launchConfigurationTypeForId( iface->scriptAppConfigTypeId() );
        Q_ASSERT(type);
        type->addLauncher( new XDebugLauncher( this ) );
    }
    {
        IExecuteBrowserPlugin* iface = KDevelop::ICore::self()->pluginController()->pluginForExtension("org.kdevelop.IExecuteBrowserPlugin")->extension<IExecuteBrowserPlugin>();
        Q_ASSERT(iface);
        KDevelop::LaunchConfigurationType* type = core()->runController()->launchConfigurationTypeForId( iface->browserAppConfigTypeId() );
        Q_ASSERT(type);
        type->addLauncher( new XDebugBrowserLauncher( this ) );
    }
}

XDebugPlugin::~XDebugPlugin()
{
}

DebugSession* XDebugPlugin::createSession() const
{
    DebugSession *session = new DebugSession();
    KDevelop::ICore::self()->debugController()->addSession(session);
    return session;
}

/*

void XDebugPlugin::sessionStarted(DebugSession* session)
{
    kDebug() << session;
    KDevelop::ICore::self()->debugController()->addSession(session);
}

void XDebugPlugin::outputLine(XDebug::DebugSession* session, QString line, KDevelop::IRunProvider::OutputTypes type)
{
    if (session->process() && m_jobs.contains(session->process())) {
        emit output(m_jobs[session->process()], line, type);
    }
}

bool XDebugPlugin::execute(const KDevelop::IRun & run, KJob* job)
{
    Q_ASSERT(instrumentorsProvided().contains(run.instrumentor()));

    QString path = run.executable().toLocalFile();
    if (path.endsWith("php")) {
        path = "";
    }
    path += " " + run.arguments().join(" ");
    path = path.trimmed();
    kDebug() << path;

    KProcess* process = m_server->startDebugger(path);
    m_jobs[process] = job;

    return !!job;
}

void XDebugPlugin::abort(KJob* job) {

}


void XDebugPlugin::debuggerStateChanged(DebugSession* session, KDevelop::IDebugSession::DebuggerState state)
{
    switch (state) {
        case KDevelop::IDebugSession::StartingState:
        case KDevelop::IDebugSession::ActiveState:
            break;
        case KDevelop::IDebugSession::PausedState:
            break;
        case KDevelop::IDebugSession::NotStartedState:
        case KDevelop::IDebugSession::StoppingState:
        case KDevelop::IDebugSession::StoppedState:
            if (session->process() && m_jobs.contains(session->process())) {
                emit finished(m_jobs[session->process()]);
            }
            break;
    }
}
*/
}

#include "xdebugplugin.moc"
