// This is the implementation of the various Chimera helpers.
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


#include "sipAPIQtDBus.h"

#include "qpydbus_chimera_helpers.h"

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusSignature>
#include <QDBusVariant>
#include <QMetaType>


// Forward declarations.
static PyObject *from_variant_type(const QDBusArgument &arg);
static PyObject *from_array_type(const QDBusArgument &arg);
static PyObject *from_structure_type(const QDBusArgument &arg);
static PyObject *from_map_type(const QDBusArgument &arg);
static PyObject *from_qstring(const QString &arg);
static PyObject *from_qvariant(const QVariant &arg);


// Convert a QVariant to a Python object.
bool qpydbus_to_pyobject(const QVariant *varp, PyObject **objp)
{
    // Handle QDBusObjectPath.
    if (varp->userType() == qMetaTypeId<QDBusObjectPath>())
    {
        *objp = from_qstring(varp->value<QDBusObjectPath>().path());

        return true;
    }

    // Handle QDBusSignature.
    if (varp->userType() == qMetaTypeId<QDBusSignature>())
    {
        *objp = from_qstring(varp->value<QDBusSignature>().signature());

        return true;
    }

    // Handle QDBusVariant.
    if (varp->userType() == qMetaTypeId<QDBusVariant>())
    {
        *objp = from_qvariant(varp->value<QDBusVariant>().variant());

        return true;
    }

    // Anything else must be a QDBusArgument.
    if (varp->userType() != qMetaTypeId<QDBusArgument>())
        return false;

    QDBusArgument arg = varp->value<QDBusArgument>();

    switch (arg.currentType())
    {
    case QDBusArgument::BasicType:
        *objp = from_qvariant(arg.asVariant());
        break;

    case QDBusArgument::VariantType:
        *objp = from_variant_type(arg);
        break;

    case QDBusArgument::ArrayType:
        *objp = from_array_type(arg);
        break;

    case QDBusArgument::StructureType:
        *objp = from_structure_type(arg);
        break;

    case QDBusArgument::MapType:
        *objp = from_map_type(arg);
        break;

    default:
        PyErr_Format(PyExc_TypeError, "unsupported DBus argument type %d",
                (int)arg.currentType());
        *objp = 0;
    }

    return true;
}


// Convert a QDBusArgument variant type to a Python object.
static PyObject *from_variant_type(const QDBusArgument &arg)
{
    QDBusVariant dbv;

    arg >> dbv;

    return from_qvariant(dbv.variant());
}


// Convert a QDBusArgument array type to a Python object.
static PyObject *from_array_type(const QDBusArgument &arg)
{
    QVariantList vl;

    arg.beginArray();

    while (!arg.atEnd())
        vl.append(arg.asVariant());

    arg.endArray();

    PyObject *obj = PyList_New(vl.count());

    if (!obj)
        return 0;

    for (int i = 0; i < vl.count(); ++i)
    {
        PyObject *itm = from_qvariant(vl.at(i));

        if (!itm)
        {
            Py_DECREF(obj);
            return 0;
        }

        PyList_SET_ITEM(obj, i, itm);
    }

    return obj;
}


// Convert a QDBusArgument structure type to a Python object.
static PyObject *from_structure_type(const QDBusArgument &arg)
{
    QVariantList vl;

    arg.beginStructure();

    while (!arg.atEnd())
        vl.append(arg.asVariant());

    arg.endStructure();

    PyObject *obj = PyTuple_New(vl.count());

    if (!obj)
        return 0;

    for (int i = 0; i < vl.count(); ++i)
    {
        PyObject *itm = from_qvariant(vl.at(i));

        if (!itm)
        {
            Py_DECREF(obj);
            return 0;
        }

        PyTuple_SET_ITEM(obj, i, itm);
    }

    return obj;
}


// Convert a QDBusArgument map type to a Python object.
static PyObject *from_map_type(const QDBusArgument &arg)
{
    PyObject *obj = PyDict_New();

    if (!obj)
        return 0;

    arg.beginMap();

    while (!arg.atEnd())
    {
        arg.beginMapEntry();

        PyObject *key = from_qvariant(arg.asVariant());
        PyObject *value = from_qvariant(arg.asVariant());

        arg.endMapEntry();

        if (!key || !value)
        {
            Py_XDECREF(key);
            Py_XDECREF(value);
            Py_DECREF(obj);

            return 0;
        }

        int rc = PyDict_SetItem(obj, key, value);

        Py_DECREF(key);
        Py_DECREF(value);

        if (rc < 0)
        {
            Py_DECREF(obj);

            return 0;
        }
    }

    arg.endMap();

    return obj;
}


// Convert a QString to a Python object.
static PyObject *from_qstring(const QString &arg)
{
    QString *heap = new QString(arg);
    PyObject *obj = sipConvertFromNewType(heap, sipType_QString, 0);

    if (!obj)
        delete heap;

    return obj;
}


// Convert a QVariant to a Python object.
static PyObject *from_qvariant(const QVariant &arg)
{
    QVariant *heap = new QVariant(arg);
    PyObject *obj = sipConvertFromNewType(heap, sipType_QVariant, 0);

    if (!obj)
        delete heap;

    return obj;
}
