/****************************************************************************
** Meta object code from reading C++ file 'settingswindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.9)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../ui/settingswindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'settingswindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.9. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SettingsWindow_t {
    QByteArrayData data[14];
    char stringdata0[191];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SettingsWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SettingsWindow_t qt_meta_stringdata_SettingsWindow = {
    {
QT_MOC_LITERAL(0, 0, 14), // "SettingsWindow"
QT_MOC_LITERAL(1, 15, 10), // "backToMain"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 11), // "configSaved"
QT_MOC_LITERAL(4, 39, 13), // "NetworkConfig"
QT_MOC_LITERAL(5, 53, 6), // "config"
QT_MOC_LITERAL(6, 60, 16), // "onTestConnection"
QT_MOC_LITERAL(7, 77, 15), // "onTestConnected"
QT_MOC_LITERAL(8, 93, 11), // "onTestError"
QT_MOC_LITERAL(9, 105, 28), // "QAbstractSocket::SocketError"
QT_MOC_LITERAL(10, 134, 11), // "socketError"
QT_MOC_LITERAL(11, 146, 13), // "onSaveClicked"
QT_MOC_LITERAL(12, 160, 16), // "onDefaultClicked"
QT_MOC_LITERAL(13, 177, 13) // "onBackClicked"

    },
    "SettingsWindow\0backToMain\0\0configSaved\0"
    "NetworkConfig\0config\0onTestConnection\0"
    "onTestConnected\0onTestError\0"
    "QAbstractSocket::SocketError\0socketError\0"
    "onSaveClicked\0onDefaultClicked\0"
    "onBackClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SettingsWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    1,   55,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   58,    2, 0x08 /* Private */,
       7,    0,   59,    2, 0x08 /* Private */,
       8,    1,   60,    2, 0x08 /* Private */,
      11,    0,   63,    2, 0x08 /* Private */,
      12,    0,   64,    2, 0x08 /* Private */,
      13,    0,   65,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void SettingsWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SettingsWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->backToMain(); break;
        case 1: _t->configSaved((*reinterpret_cast< const NetworkConfig(*)>(_a[1]))); break;
        case 2: _t->onTestConnection(); break;
        case 3: _t->onTestConnected(); break;
        case 4: _t->onTestError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 5: _t->onSaveClicked(); break;
        case 6: _t->onDefaultClicked(); break;
        case 7: _t->onBackClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SettingsWindow::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsWindow::backToMain)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SettingsWindow::*)(const NetworkConfig & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsWindow::configSaved)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SettingsWindow::staticMetaObject = { {
    &QMainWindow::staticMetaObject,
    qt_meta_stringdata_SettingsWindow.data,
    qt_meta_data_SettingsWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SettingsWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SettingsWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int SettingsWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void SettingsWindow::backToMain()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SettingsWindow::configSaved(const NetworkConfig & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
