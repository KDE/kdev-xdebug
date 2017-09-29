/*
 * XDebug-specific Variable
 *
 * Copyright 2009 Vladimir Prus <ghost@cs.msu.su>
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

#include "variable.h"
#include "debugsession.h"

#include <QDebug>

#include <QDomElement>
#include <QXmlStreamReader>

#include <interfaces/icore.h>
#include <debugger/interfaces/ivariablecontroller.h>
#include <debugger/interfaces/iframestackmodel.h>

#include "connection.h"

namespace XDebug {

static bool hasStartedSession()
{
    KDevelop::IDebugSession *session = KDevelop::ICore::self()->debugController()->currentSession();
    if (!session)
        return false;

    KDevelop::IDebugSession::DebuggerState s = session->state();
    return s != KDevelop::IDebugSession::NotStartedState
        && s != KDevelop::IDebugSession::EndedState;
}

Variable::Variable(KDevelop::TreeModel* model, KDevelop::TreeItem* parent,
            const QString& expression, const QString& display)
: KDevelop::Variable(model, parent, expression, display)
{
}

Variable::~Variable()
{
}

class PropertyGetCallback : public CallbackBase
{
public:
    PropertyGetCallback(Variable *variable, QObject *callback, const char *callbackMethod)
    : m_variable(variable), m_callback(callback), m_callbackMethod(callbackMethod)
    {}

    void execute(const QDomDocument &xml) override
    {
        qDebug() << xml.toString();
        Q_ASSERT(xml.documentElement().attribute("command") == "property_get");

        if (!m_variable) return;

        bool hasValue = false;
        QDomElement el = xml.documentElement().firstChildElement();
        if (el.nodeName() == "error") {
            qDebug() << el.firstChildElement().text();
            //hasValue=false
        } else {
            el = xml.documentElement().firstChildElement("property");
            hasValue = !el.isNull();
            if (hasValue) {
                m_variable->handleProperty(el);
            }
        }

        if (m_callback && m_callbackMethod) {
            QMetaObject::invokeMethod(m_callback, m_callbackMethod, Q_ARG(bool, hasValue));
        }
    }

    bool allowError() const override { return true; }
private:
    QPointer<Variable> m_variable;
    QObject *m_callback;
    const char *m_callbackMethod;
};

void Variable::attachMaybe(QObject *callback, const char *callbackMethod)
{
    if (hasStartedSession()) {
        // FIXME: Eventually, should be a property of variable.
        KDevelop::IDebugSession* is = KDevelop::ICore::self()->debugController()->currentSession();
        DebugSession* s = static_cast<DebugSession*>(is);
        QStringList args;
        args << "-n " + expression();
        args << QString("-d %0").arg(s->frameStackModel()->currentFrame());
        s->connection()->sendCommand("property_get", args, QByteArray(),
                                     new PropertyGetCallback(this, callback, callbackMethod));
    }
}

void Variable::fetchMoreChildren()
{
    // FIXME: should not even try this if app is not started.
    // Probably need to disable open, or something
    if (hasStartedSession()) {
        // FIXME: Eventually, should be a property of variable.
        KDevelop::IDebugSession* is = KDevelop::ICore::self()->debugController()->currentSession();
        DebugSession* s = static_cast<DebugSession*>(is);
        qDebug() << expression() << m_fullName;
        QStringList args;
        args << "-n " + m_fullName;
        args << QString("-d %0").arg(s->frameStackModel()->currentFrame());
        s->connection()->sendCommand("property_get", args, QByteArray(),
                                     new PropertyGetCallback(this, 0, 0));
    }
}


void Variable::handleProperty(const QDomElement &xml)
{
    Q_ASSERT(!xml.isNull());
    Q_ASSERT(xml.nodeName() == "property");

    setInScope(true);

    m_fullName = xml.attribute("fullname");
    //qDebug() << m_fullName;
    if (xml.firstChild().isText()) {
        QString v  = xml.firstChild().toText().data();
        if (xml.attribute("encoding") == "base64") {
            //TODO: use Connection::m_codec->toUnicode
            v = QString::fromUtf8(QByteArray::fromBase64(xml.text().toAscii()));
        }
        //qDebug() << "value" << v;
        setValue(v);
    }

    QMap<QString, Variable*> existing;
    for (int i = 0; i < childCount() - (hasMore() ? 1 : 0) ; i++) {
        Q_ASSERT(dynamic_cast<Variable*>(child(i)));
        Variable* v = static_cast<Variable*>(child(i));
        existing[v->expression()] = v;
    }

    QSet<QString> current;
    QDomElement el = xml.firstChildElement("property");
    while (!el.isNull()) {
        QString name = el.attribute("name");
        //qDebug() << name;
        current << name;
        Variable* v = 0;
        if( !existing.contains(name) ) {
            v = new Variable(model(), this, name);
            appendChild( v, false );
        } else {
            v = existing[name];
        }

        v->handleProperty(el);

        el = el.nextSiblingElement("property");
    }

    for (int i = 0; i < childCount() - (hasMore() ? 1 : 0); ++i) {
        Q_ASSERT(dynamic_cast<KDevelop::Variable*>(child(i)));
        KDevelop::Variable* v = static_cast<KDevelop::Variable*>(child(i));
        if (!current.contains(v->expression())) {
            removeChild(i);
            --i;
            delete v;
        }
    }
    if (!childCount() && xml.attribute("children") == "1") {
        qDebug() << "has more" << this;
        setHasMore(true);
        if (isExpanded()) {
            fetchMoreChildren();
        }
    }
}


QString Variable::fullName() const
{
    return m_fullName;
}


}
