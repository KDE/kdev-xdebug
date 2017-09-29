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


#include "connection.h"

#include <QtNetwork/QTcpSocket>
#include <QtCore/QSocketNotifier>
#include <QtCore/QTextCodec>
#include <QDomElement>

#include <QDebug>
#include <QUrl>

#include <interfaces/icore.h>
#include <interfaces/idebugcontroller.h>

#include "debugsession.h"

namespace XDebug {

Connection::Connection(QTcpSocket* socket, QObject * parent)
    : QObject(parent),
    m_socket(socket),
    m_currentState(DebugSession::NotStartedState),
    m_lastTransactionId(0)
{
    Q_ASSERT(m_socket);
    m_socket->setParent(this);

    m_codec = QTextCodec::codecForLocale();

    connect(m_socket, SIGNAL(disconnected()), this, SIGNAL(closed()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error( QAbstractSocket::SocketError)));

    connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

Connection::~Connection()
{
}

void Connection::close() {
    delete m_socket;
    m_socket = nullptr;
}


void Connection::error(QAbstractSocket::SocketError error)
{
    //qWarning() << m_socket->errorString();
    qWarning() << error;
}

void Connection::readyRead()
{
    while (m_socket && m_socket->bytesAvailable()) {
        long length;
        {
            QByteArray data;
            char c;
            while(m_socket->getChar(&c)) {
                if(c==0) break;
                data.append(c);
            }
            length = data.toLong();
        }
        QByteArray data;
        while (data.length() <= length) {
            if (!data.isEmpty() && !m_socket->waitForReadyRead()) {
                return;
            }
            data += m_socket->read(length - data.length() + 1);
        }
        //qDebug() << data;

        QDomDocument doc;
        doc.setContent(data);
        if (doc.documentElement().tagName() == "init") {
            processInit(doc);
        } else if (doc.documentElement().tagName() == "response") {
            processResponse(doc);
        } else if (doc.documentElement().tagName() == "stream") {
            processStream(doc);
        } else {
            //qWarning() << "unknown element" << xml->name();
        }
    }
}
void Connection::sendCommand(const QString& cmd, QStringList arguments, const QByteArray& data, CallbackBase* callback)
{
    Q_ASSERT(m_socket);
    Q_ASSERT(m_socket->isOpen());

    arguments.prepend("-i " + QString::number(++m_lastTransactionId));
    if (callback) {
        m_callbacks[m_lastTransactionId] = callback;
    }

    QByteArray out = m_codec->fromUnicode(cmd);
    if (!arguments.isEmpty()) {
        out += ' ' + m_codec->fromUnicode(arguments.join(QString(' ')));
    }
    if (!data.isEmpty()) {
        out += " -- " + data.toBase64();
    }
    qDebug() << out;
    m_socket->write(out);
    m_socket->write("\0", 1);
}

void Connection::processInit(const QDomDocument &xml)
{
    qDebug() << "idekey" << xml.documentElement().attribute("idekey");

    sendCommand("feature_get -n encoding");
    sendCommand("stderr -c 1"); //copy stderr to IDE
    sendCommand("stdout -c 1"); //copy stdout to IDE



    setState(DebugSession::StartingState);
}

void Connection::processResponse(const QDomDocument &xml)
{
    QString status = xml.documentElement().attribute("status");
    if (status == "running") {
        setState(DebugSession::ActiveState);
    } else if (status == "stopping") {
        setState(DebugSession::StoppingState);
    } else if (status == "stopped") {
        setState(DebugSession::StoppedState);
    } else if (status == "break") {
        setState(DebugSession::PausedState);
        QDomElement el = xml.documentElement().firstChildElement();
        if (el.nodeName() == "xdebug:message") {
            QUrl file = QUrl(el.attribute("filename"));
            int lineNum = el.attribute("lineno").toInt()-1;
            emit currentPositionChanged(file, lineNum);
        }
    }
    if (xml.documentElement().attribute("command") == "feature_get" && xml.documentElement().attribute("feature_name") == "encoding") {
        QTextCodec* c = QTextCodec::codecForName(xml.documentElement().text().toUtf8());
        if (c) {
            m_codec = c;
        }
    }

    CallbackBase* callback = nullptr;
    if (xml.documentElement().hasAttribute("transaction_id")) {
        int transactionId = xml.documentElement().attribute("transaction_id").toInt();
        if (m_callbacks.contains(transactionId)) {
            callback = m_callbacks[transactionId];
            m_callbacks.remove(transactionId);
        }
    }
    if (callback && !callback->allowError()) {
        //if callback doesn't handle errors himself
        QDomElement el = xml.documentElement().firstChildElement();
        if (el.nodeName() == "error") {
            qWarning() << "error" << el.attribute("code") << "for transaction" << xml.documentElement().attribute("transaction_id");
            qDebug() << el.firstChildElement().text();
            Q_ASSERT(false);
        }
    }
    if (callback) {
        callback->execute(xml);
        delete callback;
    }
}

void Connection::setState(DebugSession::DebuggerState state)
{
    qDebug() << state;
    if (m_currentState != state) {
        m_currentState = state;
    }
    emit stateChanged(state);
}

DebugSession::DebuggerState Connection::currentState()
{
    return m_currentState;
}


void Connection::processStream(const QDomDocument &xml)
{
    if (xml.documentElement().attribute("encoding") == "base64") {
        /* We ignore the output type for now
        KDevelop::IRunProvider::OutputTypes outputType;
        if (xml->attributes().value("type") == "stdout") {
            outputType = KDevelop::IRunProvider::StandardOutput;
        } else if (xml->attributes().value("type") == "stderr") {
            outputType = KDevelop::IRunProvider::StandardError;
        } else {
            qWarning() << "unknown output type" << xml->attributes().value("type");
            return;
        }
        */

        QString c = m_codec->toUnicode(QByteArray::fromBase64(xml.documentElement().text().toUtf8()));
        //qDebug() << c;
        emit output(c);
        m_outputLine += c;
        int pos = m_outputLine.indexOf('\n');
        if (pos != -1) {
            emit outputLine(m_outputLine.left(pos));
            m_outputLine = m_outputLine.mid(pos+1);
        }
    } else {
        qWarning() << "unknown encoding" << xml.documentElement().attribute("encoding");
    }
}

void Connection::processFinished(int exitCode)
{
    Q_UNUSED(exitCode);
    /*
    QMapIterator<KDevelop::IRunProvider::OutputTypes, QString> i(m_outputLine);
    while (i.hasNext()) {
        i.next();
        emit outputLine(i.value(), i.key());
    }
    */
}

QTcpSocket* Connection::socket() {
    return m_socket;
}

}

