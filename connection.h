/*
 * XDebug Debugger Support
 *
 * Copyright (C) 2004-2006 by Thiago Silva <thiago.silva@kdemail.net>
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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <QtCore/QObject>
#include <QtNetwork/QAbstractSocket>
#include <QStringList>

#include "debugsession.h"

class QDomDocument;
class QTcpSocket;

namespace XDebug {
class StackModel;

class CallbackBase {
public:
    virtual void execute(const QDomDocument &) = 0;
    virtual ~CallbackBase() {};

    /**
     * @return true if the callback handles errors himself. Else the callback won't be executed if an error is returned
     */
    virtual bool allowError() const = 0;
};

template<class Handler, class Cookie>
class CallbackWithCookie : public CallbackBase {
public:
    CallbackWithCookie(Handler* scope, void (Handler::*method)(Cookie*, const QDomDocument &), Cookie* cookie = 0, bool allowError = false)
        : m_cookie(cookie), m_scope(scope), m_method(method), m_allowError(allowError)
    {}

    void execute(const QDomDocument & xml) override
    {
        return (m_scope->*m_method)(m_cookie, xml);
    }

    bool allowError() const override { return m_allowError; }

private:
    Cookie* m_cookie;
    Handler* m_scope;
    void (Handler::*m_method)(Cookie*, const QDomDocument &);
    bool m_allowError;
};

template<class Handler>
class Callback : public CallbackBase {
public:
    Callback(Handler* scope, void (Handler::*method)(const QDomDocument &), bool allowError = false)
        : m_scope(scope), m_method(method), m_allowError(allowError)
    {}

    void execute(const QDomDocument & xml) override
    {
        return (m_scope->*m_method)(xml);
    }

    bool allowError() const override { return m_allowError; }

private:
    Handler* m_scope;
    void (Handler::*m_method)(const QDomDocument &);
    bool m_allowError;
};

class Connection : public QObject
{
    Q_OBJECT
public:
    Connection(QTcpSocket* socket, QObject * parent = 0);
    ~Connection() override;

    void close();

    void sendCommand(const QString& cmd, QStringList arguments = QStringList(),
                const QByteArray& data = QByteArray(), CallbackBase* callback = 0);
    void setState(DebugSession::DebuggerState state);
    DebugSession::DebuggerState currentState();

    QTcpSocket* socket();

public Q_SLOTS:
    void processFinished(int exitCode);

Q_SIGNALS:
    void stateChanged(KDevelop::IDebugSession::DebuggerState status);
    void currentPositionChanged(const KUrl &fileName, int lineNum);
    void output(QString content);
    void outputLine(QString content);
//     void initDone(const QString& ideKey);
    void closed();

private Q_SLOTS:
    void readyRead();
    void error(QAbstractSocket::SocketError error);

private:
    void processInit(const QDomDocument & xml);
    void processResponse(const QDomDocument &xml);
    void processStream(const QDomDocument &xml);

    QTcpSocket* m_socket;

    QString m_outputLine;
    DebugSession::DebuggerState m_currentState;
    QTextCodec* m_codec;
    StackModel* m_stackModel;

    int m_lastTransactionId;
    QMap<int, CallbackBase*> m_callbacks;
};

}
#endif
