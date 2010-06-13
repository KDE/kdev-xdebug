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

#ifndef QTEST_CONNECTIONTEST
#define QTEST_CONNECTIONTEST

#include <QtCore/QObject>
namespace KDevelop {
    class TestCore;
}
class ConnectionTest : public QObject
{
Q_OBJECT
private slots:
    void init();
    void cleanup();

    void testStdOutput();
    void testShowStepInSource();
    void testMultipleSessions();
    void testStackModel();
    void testBreakpoint();
    void testDisableBreakpoint();
    void testChangeLocationBreakpoint();
    void testDeleteBreakpoint();
    void testConditionalBreakpoint();
    void testBreakpointError();
    void testVariablesLocals();
    void testVariablesSuperglobals();
    void testVariableExpanding();
    void testPhpCrash();
    void testConnectionClosed();
    void testMultipleConnectionsClosed();
    void testVariableUpdates();

private:
    KDevelop::TestCore* m_core;
};

#endif
