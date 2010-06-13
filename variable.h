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

#ifndef XDEBUGVARIABLE_H
#define XDEBUGVARIABLE_H

#include <QtCore/QMap>

#include <debugger/variable/variablecollection.h>

class QDomElement;

namespace XDebug
{
class PropertyGetCallback;
class Variable : public KDevelop::Variable
{
public:
    Variable(KDevelop::TreeModel* model, KDevelop::TreeItem* parent,
                const QString& expression,
                const QString& display = "");

    ~Variable();


public: // Variable overrides
    virtual void attachMaybe(QObject *callback = 0, const char *callbackMethod = 0);
    virtual void fetchMoreChildren();

protected:
    QString fullName() const;

private: // Internal
    friend class PropertyGetCallback;
    friend class VariableController;

    void handleProperty(const QDomElement &xml);

    QString m_fullName;

};
}

#endif
