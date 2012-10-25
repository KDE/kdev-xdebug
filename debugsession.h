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

#ifndef XDEBUG_DEBUGSESSION_H
#define XDEBUG_DEBUGSESSION_H

#include <QtCore/QString>
#include <debugger/interfaces/idebugsession.h>

namespace KDevelop {
    class ILaunchConfiguration;
}
class KJob;
class QTcpServer;
class KProcess;

namespace XDebug {
class Connection;

class DebugSession : public KDevelop::IDebugSession
{
    Q_OBJECT
public:
    DebugSession();
    ~DebugSession();

    void setLaunchConfiguration(KDevelop::ILaunchConfiguration *cfg);
    void setAcceptMultipleConnections(bool v);

    bool listenForConnection();
    
    bool waitForState(DebuggerState state, int msecs = 30000);
    bool waitForFinished(int msecs = 30000);
    bool waitForConnected(int msecs = 30000);

    Connection* connection();

    virtual DebuggerState state() const;
    
    virtual bool restartAvaliable() const;

    virtual QPair<KUrl, int> convertToLocalUrl(const QPair<KUrl, int>& url) const;
    virtual QPair<KUrl, int> convertToRemoteUrl(const QPair<KUrl, int>& url) const;

private:
    virtual KDevelop::IFrameStackModel* createFrameStackModel();

Q_SIGNALS:
    void output(QString line);
    void outputLine(QString line);
//     void initDone(const QString& ideKey);

public Q_SLOTS:
    virtual void run();
    virtual void stepOut();
    virtual void stepOverInstruction();
    virtual void stepInto();
    virtual void stepIntoInstruction();
    virtual void stepOver();
    virtual void jumpToCursor();
    virtual void runToCursor();
    virtual void interruptDebugger();
    virtual void stopDebugger();
    virtual void restartDebugger();

    void eval(QByteArray source);

private Q_SLOTS:
    void incomingConnection();
    void _stateChanged(KDevelop::IDebugSession::DebuggerState);
    void connectionClosed();
    void currentPositionChanged(const KUrl &url, int line);
private:
    void closeServer();

private:
    QTcpServer* m_server;
    Connection *m_connection;
    KDevelop::ILaunchConfiguration *m_launchConfiguration;
    bool m_acceptMultipleConnections;
};

}

#endif // XDEBUG_DEBUGSESSION_H
