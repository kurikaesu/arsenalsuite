// This is the SIP interface definition for the Qt support for the standard
// Python DBus bindings.
//
// Copyright (c) 2012 Riverbank Computing Limited
//
// Licensed under the Academic Free License version 2.1
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#include <dbus/dbus-python.h>

#include <QCoreApplication>
#include <QSocketNotifier>
#include <QTimer>
#include <QTimerEvent>

#include "helper.h"


// The callback to add a watch.
extern "C" {static dbus_bool_t add_watch(DBusWatch *watch, void *data);}
static dbus_bool_t add_watch(DBusWatch *watch, void *data)
{
    pyqtDBusHelper *hlp = reinterpret_cast<pyqtDBusHelper *>(data);

    int fd = dbus_watch_get_fd(watch);
    unsigned int flags = dbus_watch_get_flags(watch);
    dbus_bool_t enabled = dbus_watch_get_enabled(watch);

    pyqtDBusHelper::Watcher watcher;
    watcher.watch = watch;

    if (flags & DBUS_WATCH_READABLE)
    {
        watcher.read = new QSocketNotifier(fd, QSocketNotifier::Read, hlp);
        watcher.read->setEnabled(enabled);
        hlp->connect(watcher.read, SIGNAL(activated(int)), SLOT(readSocket(int)));
    }

    if (flags & DBUS_WATCH_WRITABLE)
    {
        watcher.write = new QSocketNotifier(fd, QSocketNotifier::Write, hlp);
        watcher.write->setEnabled(enabled);
        hlp->connect(watcher.write, SIGNAL(activated(int)), SLOT(writeSocket(int)));
    }

    hlp->watchers.insertMulti(fd, watcher);

    return true;
}


// The callback to remove a watch.
extern "C" {static void remove_watch(DBusWatch *watch, void *data);}
static void remove_watch(DBusWatch *watch, void *data)
{
    pyqtDBusHelper *hlp = reinterpret_cast<pyqtDBusHelper *>(data);

    int fd = dbus_watch_get_fd(watch);

    pyqtDBusHelper::Watchers::iterator it = hlp->watchers.find(fd);

    while (it != hlp->watchers.end() && it.key() == fd)
    {
        pyqtDBusHelper::Watcher &watcher = it.value();

        if (watcher.watch == watch)
        {
            if (watcher.read)
                delete watcher.read;

            if (watcher.write)
                delete watcher.write;

            hlp->watchers.erase(it);

            return;
        }

        ++it;
    }
}


// The callback to toggle a watch.
extern "C" {static void toggle_watch(DBusWatch *watch, void *data);}
static void toggle_watch(DBusWatch *watch, void *data)
{
    pyqtDBusHelper *hlp = reinterpret_cast<pyqtDBusHelper *>(data);

    int fd = dbus_watch_get_fd(watch);
    unsigned int flags = dbus_watch_get_flags(watch);
    dbus_bool_t enabled = dbus_watch_get_enabled(watch);

    pyqtDBusHelper::Watchers::const_iterator it = hlp->watchers.find(fd);

    while (it != hlp->watchers.end() && it.key() == fd)
    {
        const pyqtDBusHelper::Watcher &watcher = it.value();

        if (watcher.watch == watch)
        {
            if (flags & DBUS_WATCH_READABLE && watcher.read)
                watcher.read->setEnabled(enabled);

            if (flags & DBUS_WATCH_WRITABLE && watcher.write)
                watcher.write->setEnabled(enabled);

            return;
        }

        ++it;
    }
}


// The callback to add a timeout.
extern "C" {static dbus_bool_t add_timeout(DBusTimeout *timeout, void *data);}
static dbus_bool_t add_timeout(DBusTimeout *timeout, void *data)
{
    // Nothing to do if the timeout is disabled.
    if (!dbus_timeout_get_enabled(timeout))
        return true;

    // Pretend it is successful if there is no application instance.  This can
    // happen if the global application gets garbage collected before the
    // dbus-python main loop does.
    if (!QCoreApplication::instance())
        return true;

    pyqtDBusHelper *hlp = reinterpret_cast<pyqtDBusHelper *>(data);

    int id = hlp->startTimer(dbus_timeout_get_interval(timeout));

    if (!id)
        return false;

    hlp->timeouts[id] = timeout;

    return true;
}


// The callback to remove a timeout.
extern "C" {static void remove_timeout(DBusTimeout *timeout, void *data);}
static void remove_timeout(DBusTimeout *timeout, void *data)
{
    pyqtDBusHelper *hlp = reinterpret_cast<pyqtDBusHelper *>(data);

    pyqtDBusHelper::Timeouts::iterator it = hlp->timeouts.begin();

    while (it != hlp->timeouts.end())
        if (it.value() == timeout)
        {
            hlp->killTimer(it.key());
            it = hlp->timeouts.erase(it);
        }
        else
            ++it;
}


// The callback to toggle a timeout.
extern "C" {static void toggle_timeout(DBusTimeout *timeout, void *data);}
static void toggle_timeout(DBusTimeout *timeout, void *data)
{
    remove_timeout(timeout, data);
    add_timeout(timeout, data);
}


// The callback to delete a helper instance.
extern "C" {static void dbus_qt_delete_helper(void *data);}
static void dbus_qt_delete_helper(void *data)
{
    delete reinterpret_cast<pyqtDBusHelper *>(data);
}


// The callback to wakeup the event loop.
extern "C" {static void wakeup_main(void *data);}
static void wakeup_main(void *data)
{
    pyqtDBusHelper *hlp = reinterpret_cast<pyqtDBusHelper *>(data);

    QTimer::singleShot(0, hlp, SLOT(dispatch()));
}


// The callback to set up a DBus connection.
extern "C" {static dbus_bool_t dbus_qt_conn(DBusConnection *conn, void *data);}
static dbus_bool_t dbus_qt_conn(DBusConnection *conn, void *data)
{
    bool rc;

    Py_BEGIN_ALLOW_THREADS

    pyqtDBusHelper *hlp = reinterpret_cast<pyqtDBusHelper *>(data);

    hlp->connections.append(conn);

    if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch,
            toggle_watch, data, 0))
        rc = false;
    else if (!dbus_connection_set_timeout_functions(conn, add_timeout,
            remove_timeout, toggle_timeout, data, 0))
        rc = false;
    else
        rc = true;

    dbus_connection_set_wakeup_main_function(conn, wakeup_main, hlp, 0);

    Py_END_ALLOW_THREADS

    return rc;
}


// The callback to set up a DBus server.
extern "C" {static dbus_bool_t dbus_qt_srv(DBusServer *srv, void *data);}
static dbus_bool_t dbus_qt_srv(DBusServer *srv, void *data)
{
    bool rc;

    Py_BEGIN_ALLOW_THREADS

    if (!dbus_server_set_watch_functions(srv, add_watch, remove_watch,
            toggle_watch, data, 0))
        rc = false;
    else if (!dbus_server_set_timeout_functions(srv, add_timeout,
            remove_timeout, toggle_timeout, data, 0))
        rc = false;
    else
        rc = true;

    Py_END_ALLOW_THREADS

    return rc;
}


// Create a new helper instance.
pyqtDBusHelper::pyqtDBusHelper() : QObject()
{
}


// Handle a socket being ready to read.
void pyqtDBusHelper::readSocket(int fd)
{
    Watchers::const_iterator it = watchers.find(fd);

    while (it != watchers.end() && it.key() == fd)
    {
        const Watcher &watcher = it.value();

        if (watcher.read && watcher.read->isEnabled())
        {
            watcher.read->setEnabled(false);
            dbus_watch_handle(watcher.watch, DBUS_WATCH_READABLE);

            // The notifier could have just been destroyed.
            if (watcher.read)
                watcher.read->setEnabled(true);

            break;
        }

        ++it;
    }

    dispatch();
}


// Handle a socket being ready to write.
void pyqtDBusHelper::writeSocket(int fd)
{
    Watchers::const_iterator it = watchers.find(fd);

    while (it != watchers.end() && it.key() == fd)
    {
        const Watcher &watcher = it.value();

        if (watcher.write && watcher.write->isEnabled())
        {
            watcher.write->setEnabled(false);
            dbus_watch_handle(watcher.watch, DBUS_WATCH_WRITABLE);

            // The notifier could have just been destroyed.
            if (watcher.write)
                watcher.write->setEnabled(true);

            break;
        }

        ++it;
    }
}


// Handoff to the connection dispatcher while there is data.
void pyqtDBusHelper::dispatch()
{
    for (Connections::const_iterator it = connections.constBegin(); it != connections.constEnd(); ++it)
        while (dbus_connection_dispatch(*it) == DBUS_DISPATCH_DATA_REMAINS)
            ;
}


// Reimplemented to handle timer events.
void pyqtDBusHelper::timerEvent(QTimerEvent *e)
{
    DBusTimeout *timeout = timeouts.value(e->timerId());

    if (timeout)
        dbus_timeout_handle(timeout);
}


PyDoc_STRVAR(DBusQtMainLoop__doc__,
"DBusQtMainLoop([set_as_default=False]) -> NativeMainLoop\n"
"\n"
"Return a NativeMainLoop object.\n"
"\n"
"If the keyword argument set_as_default is given and is True, set the new\n"
"main loop as the default for all new Connection or Bus instances.\n");

extern "C" {static PyObject *DBusQtMainLoop(PyObject *, PyObject *args, PyObject *kwargs);}
static PyObject *DBusQtMainLoop(PyObject *, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_Size(args) != 0)
    {
        PyErr_SetString(PyExc_TypeError, "DBusQtMainLoop() takes no positional arguments");
        return 0;
    }

    int set_as_default = 0;
    static char *argnames[] = {"set_as_default", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", argnames, &set_as_default))
        return 0;

    pyqtDBusHelper *hlp = new pyqtDBusHelper;

    PyObject *mainloop = DBusPyNativeMainLoop_New4(dbus_qt_conn, dbus_qt_srv,
                dbus_qt_delete_helper, hlp);

    if (!mainloop)
    {
        delete hlp;
        return 0;
    }

    if (set_as_default)
    {
        PyObject *func = PyObject_GetAttrString(_dbus_bindings_module, "set_default_main_loop");

        if (!func)
        {
            Py_DECREF(mainloop);
            return 0;
        }

        PyObject *res = PyObject_CallFunctionObjArgs(func, mainloop, 0);
        Py_DECREF(func);

        if (!res)
        {
            Py_DECREF(mainloop);
            return 0;
        }

        Py_DECREF(res);
    }

    return mainloop;
}


// The table of module functions.
static PyMethodDef module_functions[] = {
    {"DBusQtMainLoop", (PyCFunction)DBusQtMainLoop, METH_VARARGS|METH_KEYWORDS,
    DBusQtMainLoop__doc__},
    {0, 0, 0, 0}
};


// The module entry point.
#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_qt()
{
    static PyModuleDef module_def = {
        PyModuleDef_HEAD_INIT,
        "qt",
        NULL,
        -1,
        module_functions,
    };

    // Import the generic part of the Python DBus bindings.
    if (import_dbus_bindings("dbus.mainloop.qt") < 0)
        return 0;

    return PyModule_Create(&module_def);
}
#else
PyMODINIT_FUNC initqt()
{
    // Import the generic part of the Python DBus bindings.
    if (import_dbus_bindings("dbus.mainloop.qt") < 0)
        return;

    Py_InitModule("qt", module_functions);
}
#endif
