/*
   Copyright 2009 Niko Sams <niko.sams@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef XDEBUG_BREAKPOINTCONTROLLER_H
#define XDEBUG_BREAKPOINTCONTROLLER_H

#include <debugger/interfaces/ibreakpointcontroller.h>
#include <debugger/interfaces/idebugsession.h>

class QDomDocument;
namespace XDebug {
class DebugSession;

class BreakpointController : public KDevelop::IBreakpointController
{
Q_OBJECT
public:
    BreakpointController(DebugSession* parent);

protected:
    virtual void sendMaybe(KDevelop::Breakpoint* breakpoint);

private slots:
    void stateChanged(KDevelop::IDebugSession::DebuggerState state);

private:
    DebugSession *debugSession();
    void handleSetBreakpoint(KDevelop::Breakpoint* breakpoint, const QDomDocument &xml);
    void handleBreakpointList(const QDomDocument &xml);

    QMap<KDevelop::Breakpoint*, QString> m_ids;
};

}

#endif // XDEBUG_BREAKPOINTCONTROLLER_H
