// This defines the subset of the interface to Qt5's internal
// QMetaObjectBuilder class.  The internal representation of a QMetaObject
// changed in Qt5 (specifically revision 7) sufficiently to justify using this
// internal code.  The alternative would be to reverse engineer other internal
// data structures which would be even more fragile.
//
// Copyright (c) 2012 Riverbank Computing Limited <info@riverbankcomputing.com>
// 
// This file is part of PyQt.
// 
// This file may be used under the terms of the GNU General Public
// License versions 2.0 or 3.0 as published by the Free Software
// Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
// included in the packaging of this file.  Alternatively you may (at
// your option) use any later version of the GNU General Public
// License if such license has been publicly approved by Riverbank
// Computing Limited (or its successors, if any) and the KDE Free Qt
// Foundation. In addition, as a special exception, Riverbank gives you
// certain additional rights. These rights are described in the Riverbank
// GPL Exception version 1.1, which can be found in the file
// GPL_EXCEPTION.txt in this package.
// 
// If you are unsure which license is appropriate for your use, please
// contact the sales department at sales@riverbankcomputing.com.
// 
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.


#ifndef _QPYCORE_QMETAOBJECTBUILDER_H
#define _QPYCORE_QMETAOBJECTBUILDER_H


#include <qglobal.h>

#if QT_VERSION >= 0x050000

#include <QByteArray>
#include <QMetaObject>

QT_BEGIN_NAMESPACE


class QMetaMethodBuilder;
class QMetaPropertyBuilder;


class Q_CORE_EXPORT QMetaObjectBuilder
{
public:
    QMetaObjectBuilder();
    virtual ~QMetaObjectBuilder();

    void setClassName(const QByteArray &name);
    void setSuperClass(const QMetaObject *meta);
    QMetaMethodBuilder addSlot(const QByteArray &signature);
    QMetaMethodBuilder addSignal(const QByteArray &signature);
    QMetaPropertyBuilder addProperty(const QByteArray &name,
            const QByteArray &type, int notifierId=-1);
    int addClassInfo(const QByteArray &name, const QByteArray &value);
    int addRelatedMetaObject(const QMetaObject *meta);
    int indexOfSignal(const QByteArray &signature);
    QMetaObject *toMetaObject() const;

private:
    Q_DISABLE_COPY(QMetaObjectBuilder)

    void *d;
};


class Q_CORE_EXPORT QMetaMethodBuilder
{
public:
    QMetaMethodBuilder() : _mobj(0), _index(0) {}

    void setReturnType(const QByteArray &type);

private:
    const QMetaObjectBuilder *_mobj;
    int _index;
};


class Q_CORE_EXPORT QMetaPropertyBuilder
{
public:
    QMetaPropertyBuilder() : _mobj(0), _index(0) {}

    void setReadable(bool value);
    void setWritable(bool value);
    void setResettable(bool value);
    void setDesignable(bool value);
    void setScriptable(bool value);
    void setStored(bool value);
    void setUser(bool value);
    void setStdCppSet(bool value);
    void setEnumOrFlag(bool value);
    void setConstant(bool value);
    void setFinal(bool value);

private:
    const QMetaObjectBuilder *_mobj;
    int _index;
};


QT_END_NAMESPACE

#endif


#endif
