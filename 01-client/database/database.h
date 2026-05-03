#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantList>
#include <QDateTime>

/**
 * @brief Vehicle row
 */
struct VehicleInfo {
    int id;                 // pk
    QString plateNumber;    // plate
    QString rfidCard;       // rfid
    QDateTime entryTime;    // entry
    QDateTime exitTime;     // exit
    int status;             // 0 in 1 out
    double totalFee;        // charged

    VehicleInfo() : id(0), status(0), totalFee(0.0) {}
};

/**
 * @brief History projection
 */
struct ParkingRecord {
    int id;                 // pk
    QString plateNumber;    // plate
    QString rfidCard;       // rfid
    QDateTime entryTime;    // entry
    QDateTime exitTime;     // exit
    int duration;           // minutes
    double fee;             // CNY

    ParkingRecord() : id(0), duration(0), fee(0.0) {}
};

/**
 * @brief Lot policy
 */
struct ParkingConfig {
    int totalSpaces;        // capacity
    double hourlyRate;      // CNY/hour (unused in fee fn)

    ParkingConfig() : totalSpaces(50), hourlyRate(5.0) {}
};

/**
 * @brief SQLite facade
 *
 * Parking persistence + billing
 */
class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    /**
     * @brief open sqlite
     * @param path db file
     * @return true on success
     */
    bool open(const QString &path = "/run/media/mmcblk1p1/parking.db");

    /**
     * @brief close
     */
    void close();

    /**
     * @brief isOpen
     */
    bool isOpen() const;

    // --- Vehicles ---

    /**
     * @brief check-in
     * @param plateNumber
     * @param rfidCard（可选）
     * @return row id or -1
     */
    int addVehicle(const QString &plateNumber, const QString &rfidCard = QString());

    /**
     * @brief checkout + charge
     * @param plateNumber
     * @param rfidCard payer
     * @return true on success
     */
    bool checkoutVehicle(const QString &plateNumber, const QString &rfidCard);

    /**
     * @brief lookup by plate
     * @param plateNumber
     * @return 车辆信息
     */
    VehicleInfo queryVehicle(const QString &plateNumber);

    /**
     * @brief lookup by rfid
     * @param rfidCard
     * @return 车辆信息
     */
    VehicleInfo queryVehicleByRfid(const QString &rfidCard);

    /**
     * @brief parked rows
     * @return 车辆信息列表
     */
    QList<VehicleInfo> getParkedVehicles();

    /**
     * @brief parked count
     */
    int getParkedCount();

    /**
     * @brief active session
     * @param plateNumber
     * @return VehicleInfo
     */
    VehicleInfo queryActiveVehicle(const QString &plateNumber);

    // --- Config ---

    /**
     * @brief lot config
     * @return ParkingConfig
     */
    ParkingConfig getConfig();

    /**
     * @brief update config
     * @param config
     * @return true on success
     */
    bool updateConfig(const ParkingConfig &config);

    /**
     * @brief tariff
     * @param entryTime
     * @param exitTime
     * @return CNY
     */
    double calculateFee(const QDateTime &entryTime, const QDateTime &exitTime);

    /**
     * @brief ensure RFID wallet
     */
    bool ensureCardAccount(const QString &rfidCard, double initialBalance = 100.0);

    /**
     * @brief balance
     * @return CNY or -1
     */
    double getCardBalance(const QString &rfidCard);

    // --- History ---

    /**
     * @brief queryHistory
     * @param startDate
     * @param endDate
     * @param plateNumber（可选，用于筛选）
     * @return rows
     */
    QList<ParkingRecord> queryHistory(const QDate &startDate, const QDate &endDate,
                                      const QString &plateNumber = QString());

    /**
     * @brief recent audit
     * @param count
     * @return rows
     */
    QList<ParkingRecord> getRecentRecords(int count = 10);

    // --- Occupancy ---

    /**
     * @brief free slots
     */
    int getAvailableSpaces();

    /**
     * @brief occupancy %
     */
    double getOccupancyRate();

    /**
     * @brief blacklist
     * @param plateNumber
     * @return bool
     */
    bool isIllegalVehicle(const QString &plateNumber);

    /**
     * @brief Last error string
     */
    QString lastError() const;

private:
    QSqlDatabase m_db;
    QString m_lastError;

    /**
     * @brief DDL
     */
    bool createTables();

    /**
     * @brief migrate schema
     */
    bool migrateVehicleTableIfNeeded();

    /**
     * @brief seed defaults
     */
    void initDefaultConfig();
};

#endif // DATABASE_H
