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
#ifndef XDEBUGLAUNCHCONFIG
#define XDEBUGLAUNCHCONFIG

#include <QProcess>

#include <interfaces/ilauncher.h>

#include "xdebugplugin.h"

class KProcess;
namespace KDevelop
{
class ILaunchConfiguration;
}

namespace XDebug
{

class XDebugLauncher : public KDevelop::ILauncher
{
public:
    XDebugLauncher( XDebugPlugin* plugin );
    QList< KDevelop::LaunchConfigurationPageFactory* > configPages() const override;
    QString description() const override;
    QString id() override;
    QString name() const override;
    KJob* start(const QString& launchMode, KDevelop::ILaunchConfiguration* cfg) override;
    QStringList supportedModes() const override;

protected:
    XDebugPlugin* m_plugin;
private:
    QList<KDevelop::LaunchConfigurationPageFactory*> m_factoryList;
};

class XDebugBrowserLauncher : public XDebugLauncher
{
public :
    XDebugBrowserLauncher( XDebugPlugin* plugin );
    KJob* start(const QString& launchMode, KDevelop::ILaunchConfiguration* cfg) override;
};

}

#endif
