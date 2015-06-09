// This is the support for QStringList.
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


#include <Python.h>

#include <QStringList>

#include "qpycore_sip.h"


// Convert a QStringList to a Python list object.
PyObject *qpycore_PyObject_FromQStringList(const QStringList &qstrlst)
{
    PyObject *obj;

    if ((obj = PyList_New(qstrlst.size())) == NULL)
        return NULL;

    for (int i = 0; i < qstrlst.size(); ++i)
    {
        QString *qs = new QString(qstrlst.at(i));
        PyObject *qs_obj = sipConvertFromNewType(qs, sipType_QString, 0);

        if (!qs_obj)
        {
            Py_DECREF(obj);
            delete qs;

            return 0;
        }

        PyList_SET_ITEM(obj, i, qs_obj);
    }

    return obj;
}


// Convert a Python sequence object to a QStringList.  This should only be
// called after a successful call to qpycore_PySequence_Check_QStringList().
QStringList qpycore_PySequence_AsQStringList(PyObject *obj)
{
    QStringList qstrlst;
    SIP_SSIZE_T len = PySequence_Size(obj);

    for (SIP_SSIZE_T i = 0; i < len; ++i)
    {
        PyObject *itm = PySequence_ITEM(obj, i);
        int state, iserr = 0;
        QString *qs = reinterpret_cast<QString *>(sipConvertToType(itm, sipType_QString, 0, SIP_NOT_NONE, &state, &iserr));

        Py_DECREF(itm);

        if (iserr)
        {
            // This should never happen.
            sipReleaseType(qs, sipType_QString, state);
            return QStringList();
        }

        qstrlst.append(*qs);

        sipReleaseType(qs, sipType_QString, state);
    }

    return qstrlst;
}


// See if a Python sequence object can be converted to a QStringList.
int qpycore_PySequence_Check_QStringList(PyObject *obj)
{
    SIP_SSIZE_T len;

    if (!PySequence_Check(obj) || (len = PySequence_Size(obj)) < 0)
        return 0;

    for (SIP_SSIZE_T i = 0; i < len; ++i)
    {
        PyObject *itm = PySequence_ITEM(obj, i);
        bool ok = (itm && sipCanConvertToType(itm, sipType_QString, SIP_NOT_NONE));

        Py_XDECREF(itm);

        if (!ok)
            return 0;
    }

    return 1;
}
