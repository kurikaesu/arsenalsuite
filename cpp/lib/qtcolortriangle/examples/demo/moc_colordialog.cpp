/****************************************************************************
** Meta object code from reading C++ file 'colordialog.h'
**
** Created: Mon Jan 9 15:27:33 2006
**      by: The Qt Meta Object Compiler version 59 (Qt 4.1.1-snapshot-20060105)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "colordialog.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'colordialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.1.1-snapshot-20060105. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_ColorDialog[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      15,   13,   12,   12, 0x0a,
      32,   12,   12,   12, 0x0a,
      44,   12,   12,   12, 0x0a,
      58,   12,   12,   12, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ColorDialog[] = {
    "ColorDialog\0\0c\0setColor(QColor)\0setRed(int)\0setGreen(int)\0"
    "setBlue(int)\0"
};

const QMetaObject ColorDialog::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ColorDialog,
      qt_meta_data_ColorDialog, 0 }
};

const QMetaObject *ColorDialog::metaObject() const
{
    return &staticMetaObject;
}

void *ColorDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ColorDialog))
	return static_cast<void*>(const_cast<ColorDialog*>(this));
    return QWidget::qt_metacast(_clname);
}

int ColorDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: setColor(*reinterpret_cast< const QColor(*)>(_a[1])); break;
        case 1: setRed(*reinterpret_cast< int(*)>(_a[1])); break;
        case 2: setGreen(*reinterpret_cast< int(*)>(_a[1])); break;
        case 3: setBlue(*reinterpret_cast< int(*)>(_a[1])); break;
        }
        _id -= 4;
    }
    return _id;
}
