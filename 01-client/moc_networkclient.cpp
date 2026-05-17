/****************************************************************************
** Meta object code from reading C++ file 'networkclient.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.9)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "network/networkclient.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'networkclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.9. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_NetworkClient_t {
    QByteArrayData data[19];
    char stringdata0[241];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NetworkClient_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NetworkClient_t qt_meta_stringdata_NetworkClient = {
    {
QT_MOC_LITERAL(0, 0, 13), // "NetworkClient"
QT_MOC_LITERAL(1, 14, 9), // "connected"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 12), // "disconnected"
QT_MOC_LITERAL(4, 38, 20), // "recognizeResultReady"
QT_MOC_LITERAL(5, 59, 15), // "RecognizeResult"
QT_MOC_LITERAL(6, 75, 6), // "result"
QT_MOC_LITERAL(7, 82, 13), // "errorOccurred"
QT_MOC_LITERAL(8, 96, 5), // "error"
QT_MOC_LITERAL(9, 102, 14), // "audioFileReady"
QT_MOC_LITERAL(10, 117, 8), // "fileName"
QT_MOC_LITERAL(11, 126, 9), // "audioData"
QT_MOC_LITERAL(12, 136, 11), // "onConnected"
QT_MOC_LITERAL(13, 148, 14), // "onDisconnected"
QT_MOC_LITERAL(14, 163, 11), // "onReadyRead"
QT_MOC_LITERAL(15, 175, 7), // "onError"
QT_MOC_LITERAL(16, 183, 28), // "QAbstractSocket::SocketError"
QT_MOC_LITERAL(17, 212, 11), // "socketError"
QT_MOC_LITERAL(18, 224, 16) // "onReconnectTimer"

    },
    "NetworkClient\0connected\0\0disconnected\0"
    "recognizeResultReady\0RecognizeResult\0"
    "result\0errorOccurred\0error\0audioFileReady\0"
    "fileName\0audioData\0onConnected\0"
    "onDisconnected\0onReadyRead\0onError\0"
    "QAbstractSocket::SocketError\0socketError\0"
    "onReconnectTimer"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NetworkClient[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   64,    2, 0x06 /* Public */,
       3,    0,   65,    2, 0x06 /* Public */,
       4,    1,   66,    2, 0x06 /* Public */,
       7,    1,   69,    2, 0x06 /* Public */,
       9,    2,   72,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    0,   77,    2, 0x08 /* Private */,
      13,    0,   78,    2, 0x08 /* Private */,
      14,    0,   79,    2, 0x08 /* Private */,
      15,    1,   80,    2, 0x08 /* Private */,
      18,    0,   83,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString, QMetaType::QByteArray,   10,   11,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 16,   17,
    QMetaType::Void,

       0        // eod
};

void NetworkClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NetworkClient *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->recognizeResultReady((*reinterpret_cast< const RecognizeResult(*)>(_a[1]))); break;
        case 3: _t->errorOccurred((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->audioFileReady((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 5: _t->onConnected(); break;
        case 6: _t->onDisconnected(); break;
        case 7: _t->onReadyRead(); break;
        case 8: _t->onError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 9: _t->onReconnectTimer(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 8:
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
            using _t = void (NetworkClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkClient::connected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkClient::disconnected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const RecognizeResult & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkClient::recognizeResultReady)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkClient::errorOccurred)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & , const QByteArray & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkClient::audioFileReady)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject NetworkClient::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_NetworkClient.data,
    qt_meta_data_NetworkClient,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NetworkClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NetworkClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NetworkClient.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NetworkClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void NetworkClient::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NetworkClient::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NetworkClient::recognizeResultReady(const RecognizeResult & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void NetworkClient::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void NetworkClient::audioFileReady(const QString & _t1, const QByteArray & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
