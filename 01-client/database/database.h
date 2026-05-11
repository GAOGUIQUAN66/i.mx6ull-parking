#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantList>
#include <QDateTime>

/**
 * @brief 车辆信息结构体
 */
struct VehicleInfo {
    int id;                 // 记录ID
    QString plateNumber;    // 车牌号
    QString rfidCard;       // RFID卡号
    QDateTime entryTime;    // 入库时间
    QDateTime exitTime;     // 出库时间
    int status;             // 状态：0-在场 1-已离场
    double totalFee;        // 累计费用

    VehicleInfo() : id(0), status(0), totalFee(0.0) {}
};

/**
 * @brief 停车记录结构体
 */
struct ParkingRecord {
    int id;                 // 记录ID
    QString plateNumber;    // 车牌号
    QString rfidCard;       // RFID卡号
    QDateTime entryTime;    // 入库时间
    QDateTime exitTime;     // 出库时间
    int duration;           // 停车时长（分钟）
    double fee;             // 费用

    ParkingRecord() : id(0), duration(0), fee(0.0) {}
};

/**
 * @brief 停车场配置结构体
 */
struct ParkingConfig {
    int totalSpaces;        // 总车位数
    double hourlyRate;      // 每小时费率（元）

    ParkingConfig() : totalSpaces(50), hourlyRate(5.0) {}
};

/**
 * @brief 数据库管理类
 *
 * 封装SQLite数据库操作，提供车辆信息管理和计费功能
 */
class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    /**
     * @brief 打开数据库
     * @param path 数据库文件路径，默认 /run/media/mmcblk1p1/parking.db
     * @return 成功返回true
     */
    bool open(const QString &path = "/run/media/mmcblk1p1/parking.db");

    /**
     * @brief 关闭数据库
     */
    void close();

    /**
     * @brief 检查数据库是否已打开
     */
    bool isOpen() const;

    // ========== 车辆信息操作 ==========

    /**
     * @brief 添加车辆入库记录
     * @param plateNumber 车牌号
     * @param rfidCard RFID卡号（可选）
     * @return 成功返回记录ID，失败返回-1
     */
    int addVehicle(const QString &plateNumber, const QString &rfidCard = QString());

    /**
     * @brief 车辆出库结算
     * @param plateNumber 车牌号
     * @param rfidCard 支付RFID卡号
     * @return 成功返回true
     */
    bool checkoutVehicle(const QString &plateNumber, const QString &rfidCard);

    /**
     * @brief 根据车牌号查询车辆信息
     * @param plateNumber 车牌号
     * @return 车辆信息
     */
    VehicleInfo queryVehicle(const QString &plateNumber);

    /**
     * @brief 根据RFID卡号查询车辆信息
     * @param rfidCard RFID卡号
     * @return 车辆信息
     */
    VehicleInfo queryVehicleByRfid(const QString &rfidCard);

    /**
     * @brief 获取当前在场车辆列表
     * @return 车辆信息列表
     */
    QList<VehicleInfo> getParkedVehicles();

    /**
     * @brief 获取当前在场车辆数量
     */
    int getParkedCount();

    /**
     * @brief 查询某车牌是否有在场记录
     * @param plateNumber 车牌号
     * @return 在场记录信息，不存在则id为0
     */
    VehicleInfo queryActiveVehicle(const QString &plateNumber);

    // ========== 配置操作 ==========

    /**
     * @brief 获取停车场配置
     * @return 配置信息
     */
    ParkingConfig getConfig();

    /**
     * @brief 更新停车场配置
     * @param config 配置信息
     * @return 成功返回true
     */
    bool updateConfig(const ParkingConfig &config);

    /**
     * @brief 计算停车费用
     * @param entryTime 入库时间
     * @param exitTime 出库时间
     * @return 费用金额
     */
    double calculateFee(const QDateTime &entryTime, const QDateTime &exitTime);

    /**
     * @brief 确保RFID账户存在，不存在时按初始金额创建
     */
    bool ensureCardAccount(const QString &rfidCard, double initialBalance = 100.0);

    /**
     * @brief 查询RFID卡余额
     * @return 余额，失败返回-1
     */
    double getCardBalance(const QString &rfidCard);

    // ========== 历史记录操作 ==========

    /**
     * @brief 查询历史记录
     * @param startDate 开始日期
     * @param endDate 结束日期
     * @param plateNumber 车牌号（可选，用于筛选）
     * @return 记录列表
     */
    QList<ParkingRecord> queryHistory(const QDate &startDate, const QDate &endDate,
                                      const QString &plateNumber = QString());

    /**
     * @brief 获取最近N条记录（按最近发生的入场/出场时间排序）
     * @param count 记录数量
     * @return 记录列表
     */
    QList<ParkingRecord> getRecentRecords(int count = 10);

    // ========== 车位统计 ==========

    /**
     * @brief 获取空闲车位数
     */
    int getAvailableSpaces();

    /**
     * @brief 获取车位占用率（百分比）
     */
    double getOccupancyRate();

    /**
     * @brief 检查车牌是否为非法车辆
     * @param plateNumber 车牌号
     * @return 是非法车辆返回true
     */
    bool isIllegalVehicle(const QString &plateNumber);

    /**
     * @brief 获取最后错误信息
     */
    QString lastError() const;

private:
    QSqlDatabase m_db;
    QString m_lastError;

    /**
     * @brief 创建数据库表
     */
    bool createTables();

    /**
     * @brief 迁移vehicle表，移除旧的RFID唯一约束
     */
    bool migrateVehicleTableIfNeeded();

    /**
     * @brief 初始化默认配置
     */
    void initDefaultConfig();
};

#endif // DATABASE_H
