/****************************************************************************
** Meta object code from reading C++ file 'globalsignals.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.9)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../utils/globalsignals.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'globalsignals.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.9. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_GlobalSignals_t {
    QByteArrayData data[31];
    char stringdata0[403];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_GlobalSignals_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_GlobalSignals_t qt_meta_stringdata_GlobalSignals = {
    {
QT_MOC_LITERAL(0, 0, 13), // "GlobalSignals"
QT_MOC_LITERAL(1, 14, 15), // "videoFrameReady"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 5), // "frame"
QT_MOC_LITERAL(4, 37, 16), // "captureTriggered"
QT_MOC_LITERAL(5, 54, 17), // "captureImageReady"
QT_MOC_LITERAL(6, 72, 9), // "imageData"
QT_MOC_LITERAL(7, 82, 16), // "rfidCardDetected"
QT_MOC_LITERAL(8, 99, 6), // "cardId"
QT_MOC_LITERAL(9, 106, 15), // "plateRecognized"
QT_MOC_LITERAL(10, 122, 11), // "plateNumber"
QT_MOC_LITERAL(11, 134, 10), // "confidence"
QT_MOC_LITERAL(12, 145, 22), // "plateRecognitionFailed"
QT_MOC_LITERAL(13, 168, 6), // "reason"
QT_MOC_LITERAL(14, 175, 16), // "audioPlayRequest"
QT_MOC_LITERAL(15, 192, 4), // "text"
QT_MOC_LITERAL(16, 197, 14), // "audioDataReady"
QT_MOC_LITERAL(17, 212, 9), // "audioData"
QT_MOC_LITERAL(18, 222, 21), // "audioPlaybackFinished"
QT_MOC_LITERAL(19, 244, 19), // "vehicleEntrySuccess"
QT_MOC_LITERAL(20, 264, 9), // "entryTime"
QT_MOC_LITERAL(21, 274, 18), // "vehicleExitSuccess"
QT_MOC_LITERAL(22, 293, 3), // "fee"
QT_MOC_LITERAL(23, 297, 20), // "parkingStatusUpdated"
QT_MOC_LITERAL(24, 318, 9), // "available"
QT_MOC_LITERAL(25, 328, 5), // "total"
QT_MOC_LITERAL(26, 334, 14), // "alarmTriggered"
QT_MOC_LITERAL(27, 349, 24), // "networkConnectionChanged"
QT_MOC_LITERAL(28, 374, 9), // "connected"
QT_MOC_LITERAL(29, 384, 12), // "networkError"
QT_MOC_LITERAL(30, 397, 5) // "error"

    },
    "GlobalSignals\0videoFrameReady\0\0frame\0"
    "captureTriggered\0captureImageReady\0"
    "imageData\0rfidCardDetected\0cardId\0"
    "plateRecognized\0plateNumber\0confidence\0"
    "plateRecognitionFailed\0reason\0"
    "audioPlayRequest\0text\0audioDataReady\0"
    "audioData\0audioPlaybackFinished\0"
    "vehicleEntrySuccess\0entryTime\0"
    "vehicleExitSuccess\0fee\0parkingStatusUpdated\0"
    "available\0total\0alarmTriggered\0"
    "networkConnectionChanged\0connected\0"
    "networkError\0error"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_GlobalSignals[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      15,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   89,    2, 0x06 /* Public */,
       4,    0,   92,    2, 0x06 /* Public */,
       5,    1,   93,    2, 0x06 /* Public */,
       7,    1,   96,    2, 0x06 /* Public */,
       9,    2,   99,    2, 0x06 /* Public */,
      12,    1,  104,    2, 0x06 /* Public */,
      14,    1,  107,    2, 0x06 /* Public */,
      16,    1,  110,    2, 0x06 /* Public */,
      18,    0,  113,    2, 0x06 /* Public */,
      19,    2,  114,    2, 0x06 /* Public */,
      21,    2,  119,    2, 0x06 /* Public */,
      23,    2,  124,    2, 0x06 /* Public */,
      26,    1,  129,    2, 0x06 /* Public */,
      27,    1,  132,    2, 0x06 /* Public */,
      29,    1,  135,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QImage,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QByteArray,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString, QMetaType::Float,   10,   11,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void, QMetaType::QByteArray,   17,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QDateTime,   10,   20,
    QMetaType::Void, QMetaType::QString, QMetaType::Double,   10,   22,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   24,   25,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void, QMetaType::Bool,   28,
    QMetaType::Void, QMetaType::QString,   30,

       0        // eod
};

void GlobalSignals::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GlobalSignals *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->videoFrameReady((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 1: _t->captureTriggered(); break;
        case 2: _t->captureImageReady((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 3: _t->rfidCardDetected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->plateRecognized((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 5: _t->plateRecognitionFailed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->audioPlayRequest((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->audioDataReady((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 8: _t->audioPlaybackFinished(); break;
        case 9: _t->vehicleEntrySuccess((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QDateTime(*)>(_a[2]))); break;
        case 10: _t->vehicleExitSuccess((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 11: _t->parkingStatusUpdated((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 12: _t->alarmTriggered((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->networkConnectionChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 14: _t->networkError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GlobalSignals::*)(const QImage & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::videoFrameReady)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::captureTriggered)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QByteArray & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::captureImageReady)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::rfidCardDetected)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QString & , float );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::plateRecognized)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::plateRecognitionFailed)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::audioPlayRequest)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QByteArray & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::audioDataReady)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::audioPlaybackFinished)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QString & , const QDateTime & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::vehicleEntrySuccess)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QString & , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::vehicleExitSuccess)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::parkingStatusUpdated)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::alarmTriggered)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::networkConnectionChanged)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (GlobalSignals::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GlobalSignals::networkError)) {
                *result = 14;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject GlobalSignals::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_GlobalSignals.data,
    qt_meta_data_GlobalSignals,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *GlobalSignals::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GlobalSignals::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSignals.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int GlobalSignals::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void GlobalSignals::videoFrameReady(const QImage & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void GlobalSignals::captureTriggered()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void GlobalSignals::captureImageReady(const QByteArray & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void GlobalSignals::rfidCardDetected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void GlobalSignals::plateRecognized(const QString & _t1, float _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void GlobalSignals::plateRecognitionFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void GlobalSignals::audioPlayRequest(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void GlobalSignals::audioDataReady(const QByteArray & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void GlobalSignals::audioPlaybackFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void GlobalSignals::vehicleEntrySuccess(const QString & _t1, const QDateTime & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void GlobalSignals::vehicleExitSuccess(const QString & _t1, double _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void GlobalSignals::parkingStatusUpdated(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void GlobalSignals::alarmTriggered(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void GlobalSignals::networkConnectionChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void GlobalSignals::networkError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
