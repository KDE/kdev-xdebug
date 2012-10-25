/*
* XDebug Debugger Support
*
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

#ifndef XDEBUGLAUNCHCONFIGURATIONPAGE_H
#define XDEBUGLAUNCHCONFIGURATIONPAGE_H

#include <interfaces/launchconfigurationpage.h>

namespace Ui {
    class LaunchConfigurationWidget;
}

namespace XDebug
{

class ConfigPageFactory : public KDevelop::LaunchConfigurationPageFactory
{
public:
    virtual KDevelop::LaunchConfigurationPage* createWidget(QWidget* parent);
};

class ConfigPage : public KDevelop::LaunchConfigurationPage
{
Q_OBJECT
public:
    ConfigPage( QWidget* parent = 0 );
    virtual KIcon icon() const;
    virtual void loadFromConfiguration(const KConfigGroup& cfg, KDevelop::IProject* = 0);
    virtual void saveToConfiguration(KConfigGroup, KDevelop::IProject* = 0 ) const;
    virtual QString title() const;
private:
    Ui::LaunchConfigurationWidget *m_ui;
};

}

#endif
