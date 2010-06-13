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


#include "launchconfig.h"

#include <QFileInfo>

#include <KProcess>
#include <kconfiggroup.h>
#include <kicon.h>
#include <klocale.h>
#include <kshell.h>
#include <kmessagebox.h>
#include <kparts/mainwindow.h>

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
#include <iexecutescriptplugin.h>

#include "debugsession.h"
#include "xdebugplugin.h"
#include "debugjob.h"
#include "launchconfigurationpage.h"


namespace XDebug {

XDebugLauncher::XDebugLauncher( XDebugPlugin* p ) : m_plugin( p )
{
    m_factoryList << new ConfigPageFactory();
}

QList< KDevelop::LaunchConfigurationPageFactory* > XDebugLauncher::configPages() const
{
    return m_factoryList;;
}

QString XDebugLauncher::id()
{
    return "xdebug";
}

QString XDebugLauncher::name() const
{
    return i18n("XDebug");
}

KJob* XDebugLauncher::start(const QString& launchMode, KDevelop::ILaunchConfiguration* cfg)
{
    Q_ASSERT(cfg);
    if( !cfg )
    {
        return 0;
    }
    if( launchMode == "debug" )
    {
        return new XDebugJob( m_plugin->createSession(), cfg );
    }
    kWarning() << "Unknown launch mode" << launchMode << "for config:" << cfg->name();
    return 0;
}

QStringList XDebugLauncher::supportedModes() const
{
    return QStringList() << "debug";
}

QString XDebugLauncher::description() const
{
    return i18n("Executes a php script with xdebug enabled");
}

XDebugBrowserLauncher::XDebugBrowserLauncher(XDebugPlugin* plugin)
    : XDebugLauncher(plugin)
{
}

KJob* XDebugBrowserLauncher::start(const QString& launchMode, KDevelop::ILaunchConfiguration* cfg)
{
    Q_ASSERT(cfg);
    if( !cfg ) return 0;

    if( launchMode == "debug" ) {
        return new XDebugBrowserJob( m_plugin->createSession(), cfg );
    }
    kWarning() << "Unknown launch mode" << launchMode << "for config:" << cfg->name();
    return 0;
}

}
