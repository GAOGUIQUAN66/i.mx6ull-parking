#include "database.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

Database::Database(QObject *parent)
    : QObject(parent)
{
}

Database::~Database()
{
    close();
}

bool Database::open(const QString &path)
{
    if (m_db.isOpen()) {
        close();
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        m_lastError = QString("无法打开数据库: %1").arg(m_db.lastError().text());
        return false;
    }

    if (!createTables()) {
        return false;
    }

    if (!migrateVehicleTableIfNeeded()) {
        return false;
    }

    initDefaultConfig();

    return true;
}

void Database::close()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool Database::isOpen() const
{
    return m_db.isOpen();
}

bool Database::createTables()
{
    QSqlQuery query(m_db);

    // 创建车辆信息表
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS vehicle ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "plate_number TEXT NOT NULL, "
        "rfid_card TEXT, "
        "entry_time DATETIME, "
        "exit_time DATETIME, "
        "status INTEGER DEFAULT 0, "
        "total_fee REAL DEFAULT 0.0"
        ")")) {
        m_lastError = QString("创建vehicle表失败: %1").arg(query.lastError().text());
        return false;
    }

    // 创建停车位配置表
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS parking_config ("
        "id INTEGER PRIMARY KEY, "
        "total_spaces INTEGER DEFAULT 50, "
        "hourly_rate REAL DEFAULT 5.0"
        ")")) {
        m_lastError = QString("创建parking_config表失败: %1").arg(query.lastError().text());
        return false;
    }

    // 创建历史记录表
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "plate_number TEXT NOT NULL, "
        "rfid_card TEXT, "
        "entry_time DATETIME, "
        "exit_time DATETIME, "
        "duration INTEGER, "
        "fee REAL"
        ")")) {
        m_lastError = QString("创建history表失败: %1").arg(query.lastError().text());
        return false;
    }

    // 创建黑名单表（非法车辆）
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS blacklist ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "plate_number TEXT NOT NULL UNIQUE, "
        "reason TEXT, "
        "added_time DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")")) {
        m_lastError = QString("创建blacklist表失败: %1").arg(query.lastError().text());
        return false;
    }

    // 创建RFID账户表
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS rfid_account ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "rfid_card TEXT NOT NULL UNIQUE, "
        "balance REAL DEFAULT 100.0, "
        "created_time DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")")) {
        m_lastError = QString("创建rfid_account表失败: %1").arg(query.lastError().text());
        return false;
    }

    return true;
}

void Database::initDefaultConfig()
{
    QSqlQuery query(m_db);
    query.exec("SELECT COUNT(*) FROM parking_config");
    if (query.next() && query.value(0).toInt() == 0) {
        query.exec("INSERT INTO parking_config (id, total_spaces, hourly_rate) VALUES (1, 50, 5.0)");
    }
}

int Database::addVehicle(const QString &plateNumber, const QString &rfidCard)
{
    VehicleInfo activeVehicle = queryActiveVehicle(plateNumber);
    if (activeVehicle.id != 0) {
        m_lastError = QString("车辆 %1 已在库中").arg(plateNumber);
        return -1;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO vehicle (plate_number, rfid_card, entry_time, status) "
                  "VALUES (?, ?, ?, 0)");
    query.addBindValue(plateNumber);
    query.addBindValue(rfidCard.isEmpty() ? QVariant() : rfidCard);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        m_lastError = QString("添加车辆失败: %1").arg(query.lastError().text());
        return -1;
    }

    return query.lastInsertId().toInt();
}

bool Database::checkoutVehicle(const QString &plateNumber, const QString &rfidCard)
{
    if (rfidCard.isEmpty()) {
        m_lastError = "出库必须刷卡";
        return false;
    }

    if (!ensureCardAccount(rfidCard)) {
        return false;
    }

    QSqlQuery query(m_db);

    // 获取入库时间
    query.prepare("SELECT id, rfid_card, entry_time FROM vehicle "
                  "WHERE plate_number = ? AND status = 0 ORDER BY id DESC LIMIT 1");
    query.addBindValue(plateNumber);
    if (!query.exec() || !query.next()) {
        m_lastError = QString("未找到车辆记录: %1").arg(plateNumber);
        return false;
    }

    int vehicleId = query.value(0).toInt();
    QDateTime entryTime = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
    QDateTime exitTime = QDateTime::currentDateTime();
    double fee = calculateFee(entryTime, exitTime);
    qint64 durationMinutes = (entryTime.secsTo(exitTime) + 59) / 60;
    int duration = qMax(1, static_cast<int>(durationMinutes));  // 分钟，向上取整

    double balance = getCardBalance(rfidCard);
    if (balance < 0) {
        return false;
    }
    if (balance < fee) {
        m_lastError = QString("RFID卡 %1 余额不足，当前余额 %2 元，需支付 %3 元")
            .arg(rfidCard)
            .arg(balance, 0, 'f', 2)
            .arg(fee, 0, 'f', 2);
        return false;
    }

    if (!m_db.transaction()) {
        m_lastError = QString("出库事务开启失败: %1").arg(m_db.lastError().text());
        return false;
    }

    query.prepare("UPDATE rfid_account SET balance = balance - ? WHERE rfid_card = ?");
    query.addBindValue(fee);
    query.addBindValue(rfidCard);
    if (!query.exec()) {
        m_db.rollback();
        m_lastError = QString("扣费失败: %1").arg(query.lastError().text());
        return false;
    }

    // 更新车辆记录
    query.prepare("UPDATE vehicle SET rfid_card = ?, exit_time = ?, status = 1, total_fee = ? "
                  "WHERE id = ?");
    query.addBindValue(rfidCard);
    query.addBindValue(exitTime.toString(Qt::ISODate));
    query.addBindValue(fee);
    query.addBindValue(vehicleId);

    if (!query.exec()) {
        m_db.rollback();
        m_lastError = QString("更新车辆状态失败: %1").arg(query.lastError().text());
        return false;
    }

    // 添加历史记录
    query.prepare("INSERT INTO history (plate_number, rfid_card, entry_time, exit_time, duration, fee) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(plateNumber);
    query.addBindValue(rfidCard.isEmpty() ? QVariant() : rfidCard);
    query.addBindValue(entryTime.toString(Qt::ISODate));
    query.addBindValue(exitTime.toString(Qt::ISODate));
    query.addBindValue(duration);
    query.addBindValue(fee);
    if (!query.exec()) {
        m_db.rollback();
        m_lastError = QString("写入历史记录失败: %1").arg(query.lastError().text());
        return false;
    }

    if (!m_db.commit()) {
        m_lastError = QString("出库事务提交失败: %1").arg(m_db.lastError().text());
        return false;
    }

    return true;
}

VehicleInfo Database::queryVehicle(const QString &plateNumber)
{
    VehicleInfo info;
    QSqlQuery query(m_db);

    query.prepare("SELECT id, plate_number, rfid_card, entry_time, exit_time, status, total_fee "
                  "FROM vehicle WHERE plate_number = ? ORDER BY id DESC LIMIT 1");
    query.addBindValue(plateNumber);

    if (query.exec() && query.next()) {
        info.id = query.value(0).toInt();
        info.plateNumber = query.value(1).toString();
        info.rfidCard = query.value(2).toString();
        info.entryTime = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
        info.exitTime = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        info.status = query.value(5).toInt();
        info.totalFee = query.value(6).toDouble();
    }

    return info;
}

VehicleInfo Database::queryVehicleByRfid(const QString &rfidCard)
{
    VehicleInfo info;
    QSqlQuery query(m_db);

    query.prepare("SELECT id, plate_number, rfid_card, entry_time, exit_time, status, total_fee "
                  "FROM vehicle WHERE rfid_card = ? ORDER BY id DESC LIMIT 1");
    query.addBindValue(rfidCard);

    if (query.exec() && query.next()) {
        info.id = query.value(0).toInt();
        info.plateNumber = query.value(1).toString();
        info.rfidCard = query.value(2).toString();
        info.entryTime = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
        info.exitTime = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        info.status = query.value(5).toInt();
        info.totalFee = query.value(6).toDouble();
    }

    return info;
}

QList<VehicleInfo> Database::getParkedVehicles()
{
    QList<VehicleInfo> list;
    QSqlQuery query(m_db);

    query.exec("SELECT id, plate_number, rfid_card, entry_time, status, total_fee "
               "FROM vehicle WHERE status = 0 ORDER BY entry_time DESC");

    while (query.next()) {
        VehicleInfo info;
        info.id = query.value(0).toInt();
        info.plateNumber = query.value(1).toString();
        info.rfidCard = query.value(2).toString();
        info.entryTime = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
        info.status = query.value(4).toInt();
        info.totalFee = query.value(5).toDouble();
        list.append(info);
    }

    return list;
}

int Database::getParkedCount()
{
    QSqlQuery query(m_db);
    query.exec("SELECT COUNT(*) FROM vehicle WHERE status = 0");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

VehicleInfo Database::queryActiveVehicle(const QString &plateNumber)
{
    VehicleInfo info;
    QSqlQuery query(m_db);

    query.prepare("SELECT id, plate_number, rfid_card, entry_time, exit_time, status, total_fee "
                  "FROM vehicle WHERE plate_number = ? AND status = 0 ORDER BY id DESC LIMIT 1");
    query.addBindValue(plateNumber);

    if (query.exec() && query.next()) {
        info.id = query.value(0).toInt();
        info.plateNumber = query.value(1).toString();
        info.rfidCard = query.value(2).toString();
        info.entryTime = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
        info.exitTime = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        info.status = query.value(5).toInt();
        info.totalFee = query.value(6).toDouble();
    }

    return info;
}

ParkingConfig Database::getConfig()
{
    ParkingConfig config;
    QSqlQuery query(m_db);

    query.exec("SELECT total_spaces, hourly_rate FROM parking_config WHERE id = 1");
    if (query.next()) {
        config.totalSpaces = query.value(0).toInt();
        config.hourlyRate = query.value(1).toDouble();
    }

    return config;
}

bool Database::updateConfig(const ParkingConfig &config)
{
    QSqlQuery query(m_db);
    query.prepare("UPDATE parking_config SET total_spaces = ?, hourly_rate = ? WHERE id = 1");
    query.addBindValue(config.totalSpaces);
    query.addBindValue(config.hourlyRate);

    if (!query.exec()) {
        m_lastError = QString("更新配置失败: %1").arg(query.lastError().text());
        return false;
    }
    return true;
}

double Database::calculateFee(const QDateTime &entryTime, const QDateTime &exitTime)
{
    int seconds = entryTime.secsTo(exitTime);
    if (seconds <= 0) {
        return 0.0;
    }

    // 计费规则: 1分钟0.1元
    int minutes = (seconds + 59) / 60; // 向上取整
    return minutes * 0.1;
}

bool Database::ensureCardAccount(const QString &rfidCard, double initialBalance)
{
    if (rfidCard.isEmpty()) {
        m_lastError = "RFID卡号不能为空";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT OR IGNORE INTO rfid_account (rfid_card, balance) VALUES (?, ?)");
    query.addBindValue(rfidCard);
    query.addBindValue(initialBalance);

    if (!query.exec()) {
        m_lastError = QString("创建RFID账户失败: %1").arg(query.lastError().text());
        return false;
    }

    return true;
}

double Database::getCardBalance(const QString &rfidCard)
{
    if (rfidCard.isEmpty()) {
        m_lastError = "RFID卡号不能为空";
        return -1.0;
    }

    if (!ensureCardAccount(rfidCard)) {
        return -1.0;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT balance FROM rfid_account WHERE rfid_card = ?");
    query.addBindValue(rfidCard);
    if (!query.exec() || !query.next()) {
        m_lastError = QString("查询RFID卡余额失败: %1").arg(query.lastError().text());
        return -1.0;
    }

    return query.value(0).toDouble();
}

QList<ParkingRecord> Database::queryHistory(const QDate &startDate, const QDate &endDate,
                                            const QString &plateNumber)
{
    QList<ParkingRecord> list;
    QSqlQuery query(m_db);

    QString sql = "SELECT id, plate_number, rfid_card, entry_time, exit_time, status, total_fee "
                  "FROM vehicle WHERE DATE(entry_time) >= ? AND DATE(entry_time) <= ?";

    if (!plateNumber.isEmpty()) {
        sql += " AND plate_number = ?";
    }
    sql += " ORDER BY entry_time DESC";

    query.prepare(sql);
    query.addBindValue(startDate.toString(Qt::ISODate));
    query.addBindValue(endDate.toString(Qt::ISODate));
    if (!plateNumber.isEmpty()) {
        query.addBindValue(plateNumber);
    }

    if (!query.exec()) {
        m_lastError = QString("查询历史记录失败: %1").arg(query.lastError().text());
        return list;
    }

    while (query.next()) {
        ParkingRecord record;
        record.id = query.value(0).toInt();
        record.plateNumber = query.value(1).toString();
        record.rfidCard = query.value(2).toString();
        record.entryTime = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
        record.exitTime = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);

        int status = query.value(5).toInt();
        if (status == 0 || !record.exitTime.isValid()) {
            record.exitTime = QDateTime();
            record.duration = record.entryTime.isValid()
                ? record.entryTime.secsTo(QDateTime::currentDateTime()) / 60
                : 0;
        } else {
            record.duration = record.entryTime.isValid() && record.exitTime.isValid()
                ? record.entryTime.secsTo(record.exitTime) / 60
                : 0;
        }

        record.fee = query.value(6).toDouble();
        list.append(record);
    }

    return list;
}

QList<ParkingRecord> Database::getRecentRecords(int count)
{
    QList<ParkingRecord> list;
    QSqlQuery query(m_db);

    query.prepare("SELECT id, plate_number, rfid_card, entry_time, exit_time, total_fee "
                  "FROM vehicle "
                  "ORDER BY CASE "
                  "WHEN exit_time IS NOT NULL AND exit_time != '' THEN exit_time "
                  "ELSE entry_time "
                  "END DESC "
                  "LIMIT ?");
    query.addBindValue(count);

    if (query.exec()) {
        while (query.next()) {
            ParkingRecord record;
            record.id = query.value(0).toInt();
            record.plateNumber = query.value(1).toString();
            record.rfidCard = query.value(2).toString();
            record.entryTime = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
            record.exitTime = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
            if (record.exitTime.isValid()) {
                record.duration = record.entryTime.secsTo(record.exitTime) / 60;
            } else {
                record.duration = 0;
            }
            record.fee = query.value(5).toDouble();
            list.append(record);
        }
    }

    return list;
}

int Database::getAvailableSpaces()
{
    ParkingConfig config = getConfig();
    int parked = getParkedCount();
    return qMax(0, config.totalSpaces - parked);
}

double Database::getOccupancyRate()
{
    ParkingConfig config = getConfig();
    if (config.totalSpaces <= 0) return 0.0;

    int parked = getParkedCount();
    return (double)parked / config.totalSpaces * 100.0;
}

bool Database::isIllegalVehicle(const QString &plateNumber)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM blacklist WHERE plate_number = ?");
    query.addBindValue(plateNumber);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

QString Database::lastError() const
{
    return m_lastError;
}

bool Database::migrateVehicleTableIfNeeded()
{
    QSqlQuery query(m_db);
    query.prepare("SELECT sql FROM sqlite_master WHERE type='table' AND name='vehicle'");
    if (!query.exec() || !query.next()) {
        return true;
    }

    QString createSql = query.value(0).toString().toUpper();
    if (!createSql.contains("RFID_CARD TEXT UNIQUE")) {
        return true;
    }

    qDebug() << "Database: 迁移vehicle表，移除RFID唯一约束";

    if (!m_db.transaction()) {
        m_lastError = QString("开启数据库迁移事务失败: %1").arg(m_db.lastError().text());
        return false;
    }

    QStringList sqlList;
    sqlList << "ALTER TABLE vehicle RENAME TO vehicle_old"
            << "CREATE TABLE vehicle ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "plate_number TEXT NOT NULL, "
               "rfid_card TEXT, "
               "entry_time DATETIME, "
               "exit_time DATETIME, "
               "status INTEGER DEFAULT 0, "
               "total_fee REAL DEFAULT 0.0"
               ")"
            << "INSERT INTO vehicle (id, plate_number, rfid_card, entry_time, exit_time, status, total_fee) "
               "SELECT id, plate_number, rfid_card, entry_time, exit_time, status, total_fee FROM vehicle_old"
            << "DROP TABLE vehicle_old";

    for (int i = 0; i < sqlList.size(); ++i) {
        if (!query.exec(sqlList[i])) {
            m_db.rollback();
            m_lastError = QString("迁移vehicle表失败: %1").arg(query.lastError().text());
            return false;
        }
    }

    if (!m_db.commit()) {
        m_lastError = QString("提交vehicle表迁移失败: %1").arg(m_db.lastError().text());
        return false;
    }

    return true;
}
