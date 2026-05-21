#include "querywindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>

// 颜色定义
#define COLOR_BG_DARK "#1a1a2e"
#define COLOR_BG_PANEL "#16213e"
#define COLOR_ACCENT "#e94560"
#define COLOR_SUCCESS "#4ecca3"
#define COLOR_WARNING "#ffd93d"
#define COLOR_TEXT_GRAY "#a0a0a0"

namespace {

void showDarkMessageBox(QWidget *parent, QMessageBox::Icon icon, const QString &title, const QString &text)
{
    QMessageBox msg(parent);
    msg.setIcon(icon);
    msg.setWindowTitle(title);
    msg.setText(text);
    msg.setStyleSheet(
        "QLabel { color: #111111; }"
        "QPushButton { color: #111111; background: #e0e0e0; border: 1px solid #b0b0b0; "
        "border-radius: 4px; padding: 6px 16px; min-width: 68px; }"
    );
    msg.exec();
}

QString toDisplayTime(const QString &isoText)
{
    QDateTime dt = QDateTime::fromString(isoText, Qt::ISODate);
    if (!dt.isValid()) {
        dt = QDateTime::fromString(isoText, "yyyy-MM-dd HH:mm:ss");
    }
    return dt.isValid() ? dt.toString("MM-dd hh:mm:ss") : "--";
}

QString toCsvField(const QString &value)
{
    QString escaped = value;
    escaped.replace("\"", "\"\"");
    return QString("\"%1\"").arg(escaped);
}

}

QueryWindow::QueryWindow(Database *db, QWidget *parent)
    : QMainWindow(parent)
    , m_db(db)
    , m_currentPage(1)
    , m_totalPages(1)
    , m_pageSize(10)
    , m_totalRecords(0)
    , m_queryDate(QDate::currentDate())
    , m_queryStatus(-1)
{
    setWindowTitle("数据查询中心");
    setFixedSize(1024, 600);
    setStyleSheet(QString(
        "QMainWindow { background-color: %1; }"
        "QLabel { color: white; }"
        "QPushButton { "
        "   border: none; "
        "   border-radius: 5px; "
        "   padding: 8px 25px; "
        "   font-size: 14px; "
        "   font-weight: bold; "
        "}"
        "QPushButton:hover { opacity: 0.9; }"
        "QLineEdit, QComboBox, QDateEdit { "
        "   padding: 8px 12px; "
        "   background: #0f0f23; "
        "   border: 1px solid #333; "
        "   border-radius: 5px; "
        "   color: white; "
        "   font-size: 14px; "
        "}"
        "QLineEdit:focus, QComboBox:focus, QDateEdit:focus { border-color: %2; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox::down-arrow { image: none; border: none; }"
        "QTableWidget { "
        "   background: rgba(255,255,255,0.02); "
        "   border: none; "
        "   gridline-color: #333; "
        "}"
        "QTableWidget::item { padding: 8px; }"
        "QTableWidget::item:selected { background: rgba(78, 204, 163, 0.3); }"
        "QHeaderView::section { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %3, stop:1 #0f3460); "
        "   color: %2; "
        "   font-weight: bold; "
        "   font-size: 14px; "
        "   padding: 12px; "
        "   border: none; "
        "   border-bottom: 2px solid %4; "
        "}"
    ).arg(COLOR_BG_DARK, COLOR_SUCCESS, COLOR_BG_PANEL, COLOR_ACCENT));

    setupUI();
    onTableChanged(m_tableCombo ? m_tableCombo->currentIndex() : 0);
}

QueryWindow::~QueryWindow()
{
}

void QueryWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 顶部栏
    QWidget *header = new QWidget();
    header->setFixedHeight(50);
    header->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #0f3460); border-bottom: 2px solid %2;").arg(COLOR_BG_PANEL, COLOR_ACCENT));

    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 20, 0);

    QLabel *titleLabel = new QLabel("数据查询中心");
    titleLabel->setStyleSheet(QString("font-size: 20px; font-weight: bold; color: %1;").arg(COLOR_ACCENT));

    QPushButton *backBtn = new QPushButton("← 返回主界面");
    backBtn->setStyleSheet(QString("background: %1; color: white;").arg(COLOR_ACCENT));
    connect(backBtn, &QPushButton::clicked, this, &QueryWindow::onBackClicked);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(backBtn);
    mainLayout->addWidget(header);

    // 查询条件区
    QWidget *searchPanel = new QWidget();
    searchPanel->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #0f3460); border-bottom: 1px solid #333;").arg(COLOR_BG_PANEL));
    searchPanel->setFixedHeight(130);

    QVBoxLayout *searchLayout = new QVBoxLayout(searchPanel);
    searchLayout->setContentsMargins(20, 10, 20, 10);
    searchLayout->setSpacing(8);

    QHBoxLayout *filterRow = new QHBoxLayout();
    filterRow->setSpacing(12);
    QHBoxLayout *actionRow = new QHBoxLayout();
    actionRow->setSpacing(10);

    // 数据表
    QLabel *tableLabel = new QLabel("数据表:");
    tableLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_tableCombo = new QComboBox();
    m_tableCombo->addItem("车辆表(vehicle)", "vehicle");
    m_tableCombo->addItem("历史表(history)", "history");
    m_tableCombo->addItem("RFID账户(rfid_account)", "rfid_account");
    m_tableCombo->addItem("配置表(parking_config)", "parking_config");
    m_tableCombo->addItem("黑名单(blacklist)", "blacklist");
    m_tableCombo->setFixedWidth(170);
    connect(m_tableCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onTableChanged(int)));

    // 车牌号
    QLabel *plateLabel = new QLabel("车牌号:");
    plateLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_plateEdit = new QLineEdit();
    m_plateEdit->setPlaceholderText("输入车牌号");
    m_plateEdit->setFixedWidth(150);

    // 日期
    QLabel *dateLabel = new QLabel("日期:");
    dateLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_dateEdit = new QDateEdit(QDate::currentDate());
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setFixedWidth(150);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");

    // 状态
    QLabel *statusLabel = new QLabel("状态:");
    statusLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_statusCombo = new QComboBox();
    m_statusCombo->addItem("全部", -1);
    m_statusCombo->addItem("停车中", 0);
    m_statusCombo->addItem("已出库", 1);
    m_statusCombo->setFixedWidth(120);

    // 按钮
    QPushButton *searchBtn = new QPushButton("查询");
    searchBtn->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #3db892); color: white;").arg(COLOR_SUCCESS));
    connect(searchBtn, &QPushButton::clicked, this, &QueryWindow::onSearchClicked);
    searchBtn->setMinimumWidth(96);

    QPushButton *resetBtn = new QPushButton("重置");
    resetBtn->setStyleSheet("background: #333; border: 1px solid #555; color: white;");
    connect(resetBtn, &QPushButton::clicked, this, &QueryWindow::onResetClicked);
    resetBtn->setMinimumWidth(90);

    QPushButton *exportBtn = new QPushButton("导出当前表CSV");
    exportBtn->setStyleSheet("background: #1f4068; border: 1px solid #4c6b91; color: white;");
    connect(exportBtn, &QPushButton::clicked, this, &QueryWindow::onExportCsvClicked);
    exportBtn->setMinimumWidth(150);

    QPushButton *deleteBtn = new QPushButton("删除选中记录");
    deleteBtn->setStyleSheet("background: #7d2f2f; border: 1px solid #b34b4b; color: white;");
    connect(deleteBtn, &QPushButton::clicked, this, &QueryWindow::onDeleteRecordClicked);
    deleteBtn->setMinimumWidth(138);

    QPushButton *addBlacklistBtn = new QPushButton("加入黑名单");
    addBlacklistBtn->setStyleSheet(QString("background: %1; color: white;").arg(COLOR_ACCENT));
    connect(addBlacklistBtn, &QPushButton::clicked, this, &QueryWindow::onAddBlacklistClicked);
    addBlacklistBtn->setMinimumWidth(118);

    QPushButton *removeBlacklistBtn = new QPushButton("移出黑名单");
    removeBlacklistBtn->setStyleSheet("background: #333; border: 1px solid #555; color: white;");
    connect(removeBlacklistBtn, &QPushButton::clicked, this, &QueryWindow::onRemoveBlacklistClicked);
    removeBlacklistBtn->setMinimumWidth(118);

    QPushButton *viewBlacklistBtn = new QPushButton("查看黑名单");
    viewBlacklistBtn->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #3db892); color: white;").arg(COLOR_SUCCESS));
    connect(viewBlacklistBtn, &QPushButton::clicked, this, &QueryWindow::onViewBlacklistClicked);
    viewBlacklistBtn->setMinimumWidth(118);

    filterRow->addWidget(tableLabel);
    filterRow->addWidget(m_tableCombo);
    filterRow->addWidget(plateLabel);
    filterRow->addWidget(m_plateEdit);
    filterRow->addWidget(dateLabel);
    filterRow->addWidget(m_dateEdit);
    filterRow->addWidget(statusLabel);
    filterRow->addWidget(m_statusCombo);
    filterRow->addWidget(searchBtn);
    filterRow->addWidget(resetBtn);
    filterRow->addStretch();

    actionRow->addWidget(exportBtn);
    actionRow->addWidget(deleteBtn);
    actionRow->addWidget(addBlacklistBtn);
    actionRow->addWidget(removeBlacklistBtn);
    actionRow->addWidget(viewBlacklistBtn);
    actionRow->addStretch();

    searchLayout->addLayout(filterRow);
    searchLayout->addLayout(actionRow);

    mainLayout->addWidget(searchPanel);

    // 表格区
    m_table = new QTableWidget();
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels(QStringList() << "序号" << "车牌号" << "入场时间" << "出场时间" << "停车时长" << "费用" << "状态");
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet("alternate-background-color: rgba(255,255,255,0.02);");

    // 设置列宽
    m_table->setColumnWidth(0, 60);   // 序号
    m_table->setColumnWidth(1, 100);  // 车牌号
    m_table->setColumnWidth(2, 150);  // 入场时间
    m_table->setColumnWidth(3, 150);  // 出场时间
    m_table->setColumnWidth(4, 100);  // 停车时长
    m_table->setColumnWidth(5, 80);   // 费用
    m_table->setColumnWidth(6, 80);   // 状态

    mainLayout->addWidget(m_table, 1);

    // 分页区
    QWidget *pagination = new QWidget();
    pagination->setFixedHeight(50);
    pagination->setStyleSheet(QString("background: %1; border-top: 1px solid #333;").arg(COLOR_BG_PANEL));

    QHBoxLayout *pageLayout = new QHBoxLayout(pagination);
    pageLayout->setContentsMargins(20, 0, 20, 0);

    m_totalLabel = new QLabel("共 0 条记录");
    m_totalLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));

    QHBoxLayout *btnsLayout = new QHBoxLayout();
    btnsLayout->setSpacing(5);

    m_prevBtn = new QPushButton("上一页");
    m_prevBtn->setStyleSheet("background: #333; border: 1px solid #555; color: white;");
    connect(m_prevBtn, &QPushButton::clicked, this, &QueryWindow::onPrevPage);

    m_nextBtn = new QPushButton("下一页");
    m_nextBtn->setStyleSheet("background: #333; border: 1px solid #555; color: white;");
    connect(m_nextBtn, &QPushButton::clicked, this, &QueryWindow::onNextPage);

    m_pageButtons = new QWidget();
    QHBoxLayout *pageBtnsLayout = new QHBoxLayout(m_pageButtons);
    pageBtnsLayout->setSpacing(5);

    // 创建页码按钮
    for (int i = 0; i < 5; i++) {
        QPushButton *btn = new QPushButton(QString::number(i + 1));
        btn->setStyleSheet("background: #333; border: 1px solid #555; color: white;");
        btn->setFixedWidth(40);
        connect(btn, &QPushButton::clicked, [this, btn]() { onPageClicked(btn->text().toInt()); });
        m_pageBtns.append(btn);
        pageBtnsLayout->addWidget(btn);
    }

    btnsLayout->addWidget(m_prevBtn);
    btnsLayout->addWidget(m_pageButtons);
    btnsLayout->addWidget(m_nextBtn);

    pageLayout->addWidget(m_totalLabel);
    pageLayout->addStretch();
    pageLayout->addLayout(btnsLayout);

    mainLayout->addWidget(pagination);

    setCentralWidget(centralWidget);
}

void QueryWindow::onSearchClicked()
{
    m_queryPlate = m_plateEdit->text().trimmed();
    m_queryDate = m_dateEdit->date();
    m_queryStatus = m_statusCombo->currentData().toInt();

    m_currentPage = 1;
    loadPage(1);
}

void QueryWindow::onResetClicked()
{
    m_plateEdit->clear();
    m_dateEdit->setDate(QDate::currentDate());
    m_statusCombo->setCurrentIndex(0);

    m_queryPlate.clear();
    m_queryDate = QDate::currentDate();
    m_queryStatus = -1;

    onTableChanged(m_tableCombo ? m_tableCombo->currentIndex() : 0);
}

void QueryWindow::onBackClicked()
{
    emit backToMain();
    hide();
}

void QueryWindow::onPrevPage()
{
    if (m_currentPage > 1) {
        loadPage(m_currentPage - 1);
    }
}

void QueryWindow::onNextPage()
{
    if (m_currentPage < m_totalPages) {
        loadPage(m_currentPage + 1);
    }
}

void QueryWindow::onPageClicked(int page)
{
    loadPage(page);
}

void QueryWindow::onTableChanged(int index)
{
    Q_UNUSED(index);
    const bool enablePlate = tableNeedsPlateFilter();
    const bool enableDate = tableNeedsDateFilter();
    const bool enableStatus = tableNeedsStatusFilter();
    m_plateEdit->setEnabled(enablePlate);
    m_dateEdit->setEnabled(enableDate);
    m_statusCombo->setEnabled(enableStatus);
    m_currentPage = 1;
    loadPage(1);
}

QString QueryWindow::resolveExportDir() const
{
    // 板端优先落盘到TF卡，便于后续由Ubuntu拉取。
    const QString boardDir = "/run/media/mmcblk1p1/exports";
    if (QDir("/run/media/mmcblk1p1").exists()) {
        return boardDir;
    }

    const QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!docDir.isEmpty()) {
        return docDir + "/parking_exports";
    }
    return QDir::currentPath() + "/parking_exports";
}

QString QueryWindow::currentTableName() const
{
    return m_tableCombo ? m_tableCombo->currentData().toString() : QString("vehicle");
}

bool QueryWindow::tableNeedsPlateFilter() const
{
    const QString table = currentTableName();
    return table == "vehicle" || table == "history" || table == "blacklist";
}

bool QueryWindow::tableNeedsDateFilter() const
{
    const QString table = currentTableName();
    return table == "vehicle" || table == "history";
}

bool QueryWindow::tableNeedsStatusFilter() const
{
    return currentTableName() == "vehicle";
}

void QueryWindow::onExportCsvClicked()
{
    if (!m_db || !m_db->isOpen()) {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "数据库未打开");
        return;
    }

    const QString tableName = currentTableName();
    QStringList whereParts;
    QList<QVariant> whereBindValues;
    if (tableNeedsPlateFilter() && !m_queryPlate.trimmed().isEmpty()) {
        whereParts << "plate_number = ?";
        whereBindValues << m_queryPlate.trimmed().toUpper();
    }
    if (tableNeedsDateFilter()) {
        whereParts << "DATE(entry_time) = ?";
        whereBindValues << m_queryDate.toString(Qt::ISODate);
    }
    if (tableNeedsStatusFilter() && m_queryStatus != -1) {
        whereParts << "status = ?";
        whereBindValues << m_queryStatus;
    }

    QString whereSql;
    if (!whereParts.isEmpty()) {
        whereSql = " WHERE " + whereParts.join(" AND ");
    }

    QStringList headers;
    QString selectSql;
    if (tableName == "vehicle") {
        headers << "id" << "plate_number" << "rfid_card" << "entry_time" << "exit_time" << "status" << "total_fee";
        selectSql = QString(
            "SELECT id, plate_number, rfid_card, entry_time, exit_time, status, total_fee "
            "FROM vehicle%1 ORDER BY id DESC"
        ).arg(whereSql);
    } else if (tableName == "history") {
        headers << "id" << "plate_number" << "rfid_card" << "entry_time" << "exit_time" << "duration" << "fee";
        selectSql = QString(
            "SELECT id, plate_number, rfid_card, entry_time, exit_time, duration, fee "
            "FROM history%1 ORDER BY id DESC"
        ).arg(whereSql);
    } else if (tableName == "rfid_account") {
        headers << "id" << "rfid_card" << "balance" << "created_time";
        selectSql = QString(
            "SELECT id, rfid_card, balance, created_time FROM rfid_account%1 ORDER BY id DESC"
        ).arg(whereSql);
    } else if (tableName == "parking_config") {
        headers << "id" << "total_spaces" << "hourly_rate";
        selectSql = QString(
            "SELECT id, total_spaces, hourly_rate FROM parking_config%1 ORDER BY id ASC"
        ).arg(whereSql);
    } else {
        headers << "id" << "plate_number" << "reason" << "added_time";
        selectSql = QString(
            "SELECT id, plate_number, reason, added_time FROM blacklist%1 ORDER BY id DESC"
        ).arg(whereSql);
    }

    QSqlQuery query(QSqlDatabase::database());
    query.prepare(selectSql);
    for (int i = 0; i < whereBindValues.size(); ++i) {
        query.addBindValue(whereBindValues.at(i));
    }
    if (!query.exec()) {
        showDarkMessageBox(this, QMessageBox::Warning, "导出失败", QString("SQL执行失败: %1").arg(query.lastError().text()));
        return;
    }

    const QString exportDir = resolveExportDir();
    QDir dir(exportDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        showDarkMessageBox(this, QMessageBox::Warning, "导出失败", QString("无法创建目录: %1").arg(exportDir));
        return;
    }

    const QString fileName = QString("%1_%2.csv").arg(tableName, QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    const QString filePath = dir.filePath(fileName);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        showDarkMessageBox(this, QMessageBox::Warning, "导出失败", QString("无法写入文件: %1").arg(filePath));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "\xEF\xBB\xBF";
    out << headers.join(",") << "\n";
    while (query.next()) {
        QStringList fields;
        for (int i = 0; i < headers.size(); ++i) {
            fields << toCsvField(query.value(i).toString());
        }
        out << fields.join(",") << "\n";
    }
    file.close();

    showDarkMessageBox(this, QMessageBox::Information, "导出成功",
        QString("CSV已导出:\n%1\n\n可由Ubuntu脚本继续同步到云服务器。").arg(filePath));
}

void QueryWindow::onDeleteRecordClicked()
{
    if (!m_db || !m_db->isOpen()) {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "数据库未打开");
        return;
    }

    const QString tableName = currentTableName();
    if (tableName == "parking_config") {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "配置表不支持删除，请在系统设置中修改配置");
        return;
    }

    const int row = m_table ? m_table->currentRow() : -1;
    if (row < 0 || row >= m_currentRowIds.size()) {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "请先在表格中选中一条记录");
        return;
    }

    const int recordId = m_currentRowIds.at(row);
    QMessageBox confirm(this);
    confirm.setIcon(QMessageBox::Question);
    confirm.setWindowTitle("确认删除");
    confirm.setText(QString("确认删除当前记录？\n表: %1\n记录ID: %2").arg(tableName).arg(recordId));
    confirm.setStyleSheet(
        "QLabel { color: #111111; }"
        "QPushButton { color: #111111; background: #e0e0e0; border: 1px solid #b0b0b0; "
        "border-radius: 4px; padding: 6px 16px; min-width: 68px; }"
    );
    QPushButton *yesBtn = confirm.addButton("删除", QMessageBox::AcceptRole);
    confirm.addButton("取消", QMessageBox::RejectRole);
    confirm.exec();
    if (confirm.clickedButton() != yesBtn) {
        return;
    }

    QSqlQuery query(QSqlDatabase::database());
    QString deleteSql;
    if (tableName == "vehicle") {
        deleteSql = "DELETE FROM vehicle WHERE id = ?";
    } else if (tableName == "history") {
        deleteSql = "DELETE FROM history WHERE id = ?";
    } else if (tableName == "rfid_account") {
        deleteSql = "DELETE FROM rfid_account WHERE id = ?";
    } else if (tableName == "blacklist") {
        deleteSql = "DELETE FROM blacklist WHERE id = ?";
    } else {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "当前数据表不支持删除");
        return;
    }

    query.prepare(deleteSql);
    query.addBindValue(recordId);
    if (!query.exec()) {
        showDarkMessageBox(this, QMessageBox::Warning, "删除失败", query.lastError().text());
        return;
    }

    showDarkMessageBox(this, QMessageBox::Information, "删除成功", "记录已删除");
    loadPage(m_currentPage);
}

void QueryWindow::onAddBlacklistClicked()
{
    if (!m_db || !m_db->isOpen()) {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "数据库未打开");
        return;
    }

    const QString plate = m_plateEdit->text().trimmed().toUpper();
    if (plate.isEmpty()) {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "请先在车牌号输入框填写车牌");
        return;
    }

    QInputDialog dialog(this);
    dialog.setWindowTitle("加入黑名单");
    dialog.setLabelText("请输入拉黑原因（可选）:");
    dialog.setTextValue("人工加入黑名单");
    dialog.setStyleSheet(
        "QLabel { color: #111111; }"
        "QLineEdit { color: #111111; background: #ffffff; border: 1px solid #999; border-radius: 4px; padding: 4px; }"
        "QPushButton { color: #111111; background: #e0e0e0; border: 1px solid #b0b0b0; border-radius: 4px; padding: 4px 14px; }"
    );
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QString reason = dialog.textValue();

    if (!m_db->addBlacklistPlate(plate, reason)) {
        showDarkMessageBox(this, QMessageBox::Warning, "操作失败", m_db->lastError());
        return;
    }

    showDarkMessageBox(this, QMessageBox::Information, "操作成功", QString("已加入黑名单: %1").arg(plate));
}

void QueryWindow::onRemoveBlacklistClicked()
{
    if (!m_db || !m_db->isOpen()) {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "数据库未打开");
        return;
    }

    const QString plate = m_plateEdit->text().trimmed().toUpper();
    if (plate.isEmpty()) {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "请先在车牌号输入框填写车牌");
        return;
    }

    if (!m_db->removeBlacklistPlate(plate)) {
        showDarkMessageBox(this, QMessageBox::Warning, "操作失败", m_db->lastError());
        return;
    }

    showDarkMessageBox(this, QMessageBox::Information, "操作成功", QString("已移出黑名单: %1").arg(plate));
}

void QueryWindow::onViewBlacklistClicked()
{
    showBlacklistDialog();
}

void QueryWindow::loadPage(int page)
{
    if (!m_db || !m_db->isOpen()) {
        return;
    }

    const QString tableName = currentTableName();
    QStringList whereParts;
    QList<QVariant> whereBindValues;

    if (tableNeedsPlateFilter() && !m_queryPlate.trimmed().isEmpty()) {
        whereParts << "plate_number = ?";
        whereBindValues << m_queryPlate.trimmed().toUpper();
    }
    if (tableNeedsDateFilter()) {
        whereParts << "DATE(entry_time) = ?";
        whereBindValues << m_queryDate.toString(Qt::ISODate);
    }
    if (tableNeedsStatusFilter() && m_queryStatus != -1) {
        whereParts << "status = ?";
        whereBindValues << m_queryStatus;
    }

    QString whereSql;
    if (!whereParts.isEmpty()) {
        whereSql = " WHERE " + whereParts.join(" AND ");
    }

    QSqlQuery countQuery(QSqlDatabase::database());
    QString countSql = QString("SELECT COUNT(*) FROM %1%2").arg(tableName, whereSql);
    countQuery.prepare(countSql);
    for (int i = 0; i < whereBindValues.size(); ++i) {
        countQuery.addBindValue(whereBindValues.at(i));
    }
    if (!countQuery.exec() || !countQuery.next()) {
        m_totalRecords = 0;
    } else {
        m_totalRecords = countQuery.value(0).toInt();
    }

    m_totalPages = (m_totalRecords + m_pageSize - 1) / m_pageSize;
    if (m_totalPages < 1) m_totalPages = 1;
    m_currentPage = qBound(1, page, m_totalPages);
    const int offset = (m_currentPage - 1) * m_pageSize;

    QStringList headers;
    QString selectSql;
    if (tableName == "vehicle") {
        headers << "序号" << "车牌号" << "RFID卡号" << "入场时间" << "出场时间" << "停车时长" << "费用" << "状态";
        selectSql = QString(
            "SELECT id, plate_number, rfid_card, entry_time, exit_time, status, total_fee "
            "FROM vehicle%1 ORDER BY id DESC LIMIT ? OFFSET ?"
        ).arg(whereSql);
    } else if (tableName == "history") {
        headers << "序号" << "车牌号" << "RFID卡号" << "入场时间" << "出场时间" << "停车时长" << "费用";
        selectSql = QString(
            "SELECT id, plate_number, rfid_card, entry_time, exit_time, duration, fee "
            "FROM history%1 ORDER BY id DESC LIMIT ? OFFSET ?"
        ).arg(whereSql);
    } else if (tableName == "rfid_account") {
        headers << "序号" << "RFID卡号" << "余额(元)" << "创建时间";
        selectSql = QString(
            "SELECT id, rfid_card, balance, created_time "
            "FROM rfid_account%1 ORDER BY id DESC LIMIT ? OFFSET ?"
        ).arg(whereSql);
    } else if (tableName == "parking_config") {
        headers << "序号" << "总车位" << "每小时费率(元)";
        selectSql = QString(
            "SELECT id, total_spaces, hourly_rate "
            "FROM parking_config%1 ORDER BY id ASC LIMIT ? OFFSET ?"
        ).arg(whereSql);
    } else {
        headers << "序号" << "车牌号" << "原因" << "加入时间";
        selectSql = QString(
            "SELECT id, plate_number, reason, added_time "
            "FROM blacklist%1 ORDER BY id DESC LIMIT ? OFFSET ?"
        ).arg(whereSql);
    }

    QSqlQuery query(QSqlDatabase::database());
    query.prepare(selectSql);
    for (int i = 0; i < whereBindValues.size(); ++i) {
        query.addBindValue(whereBindValues.at(i));
    }
    query.addBindValue(m_pageSize);
    query.addBindValue(offset);

    QList<QStringList> rows;
    m_currentRowIds.clear();
    if (query.exec()) {
        int seq = offset + 1;
        while (query.next()) {
            const int recordId = query.value(0).toInt();
            QStringList row;
            if (tableName == "vehicle") {
                const QString entryTime = query.value(3).toString();
                const QString exitTime = query.value(4).toString();
                const QDateTime entryDt = QDateTime::fromString(entryTime, Qt::ISODate);
                const QDateTime exitDt = QDateTime::fromString(exitTime, Qt::ISODate);
                int duration = 0;
                if (exitDt.isValid() && entryDt.isValid()) {
                    duration = entryDt.secsTo(exitDt) / 60;
                } else if (entryDt.isValid()) {
                    duration = entryDt.secsTo(QDateTime::currentDateTime()) / 60;
                }
                const QString durationText = QString("%1小时%2分").arg(duration / 60).arg(duration % 60);
                const int status = query.value(5).toInt();
                row << QString::number(seq++)
                    << query.value(1).toString()
                    << (query.value(2).toString().isEmpty() ? "--" : query.value(2).toString())
                    << toDisplayTime(entryTime)
                    << (exitDt.isValid() ? toDisplayTime(exitTime) : "--")
                    << durationText
                    << QString("¥%1").arg(query.value(6).toDouble(), 0, 'f', 2)
                    << (status == 0 ? "停车中" : "已出库");
            } else if (tableName == "history") {
                const int duration = query.value(5).toInt();
                row << QString::number(seq++)
                    << query.value(1).toString()
                    << (query.value(2).toString().isEmpty() ? "--" : query.value(2).toString())
                    << toDisplayTime(query.value(3).toString())
                    << toDisplayTime(query.value(4).toString())
                    << QString("%1小时%2分").arg(duration / 60).arg(duration % 60)
                    << QString("¥%1").arg(query.value(6).toDouble(), 0, 'f', 2);
            } else if (tableName == "rfid_account") {
                row << QString::number(seq++)
                    << query.value(1).toString()
                    << QString("¥%1").arg(query.value(2).toDouble(), 0, 'f', 2)
                    << toDisplayTime(query.value(3).toString());
            } else if (tableName == "parking_config") {
                row << QString::number(seq++)
                    << query.value(1).toString()
                    << QString::number(query.value(2).toDouble(), 'f', 2);
            } else {
                row << QString::number(seq++)
                    << query.value(1).toString()
                    << (query.value(2).toString().isEmpty() ? "--" : query.value(2).toString())
                    << toDisplayTime(query.value(3).toString());
            }
            rows.append(row);
            m_currentRowIds.append(recordId);
        }
    }

    updateTable(headers, rows);
    updatePagination();
}

void QueryWindow::updateTable(const QStringList &headers, const QList<QStringList> &rows)
{
    m_table->clear();
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setRowCount(rows.size());

    for (int i = 0; i < rows.size(); ++i) {
        const QStringList &row = rows.at(i);
        for (int j = 0; j < row.size() && j < headers.size(); ++j) {
            QTableWidgetItem *item = new QTableWidgetItem(row.at(j));
            if (j == 0) {
                item->setTextAlignment(Qt::AlignCenter);
            }
            if (headers.value(j).contains("费用") || headers.value(j).contains("余额")) {
                item->setForeground(QColor(COLOR_WARNING));
            } else if (headers.value(j).contains("车牌号") || headers.value(j).contains("RFID")) {
                item->setForeground(QColor(COLOR_SUCCESS));
            }
            if (headers.value(j) == "状态") {
                const bool parking = row.at(j) == "停车中";
                item->setBackground(parking ? QColor(COLOR_WARNING) : QColor(COLOR_SUCCESS));
                item->setForeground(QColor("#000000"));
                item->setTextAlignment(Qt::AlignCenter);
            }
            m_table->setItem(i, j, item);
        }
    }

    m_table->horizontalHeader()->setStretchLastSection(true);
}

void QueryWindow::updatePagination()
{
    m_totalLabel->setText(QString("共 %1 条记录").arg(m_totalRecords));

    // 更新页码按钮
    int startPage = qMax(1, m_currentPage - 2);
    int endPage = qMin(m_totalPages, startPage + 4);

    for (int i = 0; i < m_pageBtns.size(); i++) {
        int pageNum = startPage + i;
        if (pageNum <= endPage) {
            m_pageBtns[i]->setText(QString::number(pageNum));
            m_pageBtns[i]->setVisible(true);

            if (pageNum == m_currentPage) {
                m_pageBtns[i]->setStyleSheet(QString("background: %1; border-color: %1; color: #000;").arg(COLOR_SUCCESS));
            } else {
                m_pageBtns[i]->setStyleSheet("background: #333; border: 1px solid #555; color: white;");
            }
        } else {
            m_pageBtns[i]->setVisible(false);
        }
    }

    m_prevBtn->setEnabled(m_currentPage > 1);
    m_nextBtn->setEnabled(m_currentPage < m_totalPages);
}

void QueryWindow::showBlacklistDialog()
{
    if (!m_db || !m_db->isOpen()) {
        showDarkMessageBox(this, QMessageBox::Warning, "提示", "数据库未打开");
        return;
    }

    const QList<BlacklistEntry> entries = m_db->getBlacklistEntries();

    QDialog dialog(this);
    dialog.setWindowTitle("黑名单列表");
    dialog.resize(720, 420);
    dialog.setStyleSheet(QString(
        "QDialog { background-color: %1; }"
        "QLabel, QTableWidget { color: white; }"
        "QTableWidget { background: rgba(255,255,255,0.03); border: none; gridline-color: #333; }"
        "QHeaderView::section { background: %2; color: %3; border: none; padding: 6px; }"
        "QPushButton { background: %4; color: white; border: none; border-radius: 5px; padding: 8px 14px; }"
    ).arg(COLOR_BG_DARK, COLOR_BG_PANEL, COLOR_SUCCESS, COLOR_ACCENT));

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *title = new QLabel(QString("共 %1 条黑名单记录").arg(entries.size()));
    layout->addWidget(title);

    QTableWidget *table = new QTableWidget(entries.size(), 4);
    table->setHorizontalHeaderLabels(QStringList() << "ID" << "车牌号" << "原因" << "加入时间");
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setColumnWidth(0, 60);
    table->setColumnWidth(1, 130);
    table->setColumnWidth(2, 220);

    for (int i = 0; i < entries.size(); ++i) {
        const BlacklistEntry &entry = entries.at(i);
        table->setItem(i, 0, new QTableWidgetItem(QString::number(entry.id)));
        table->setItem(i, 1, new QTableWidgetItem(entry.plateNumber));
        table->setItem(i, 2, new QTableWidgetItem(entry.reason.isEmpty() ? "--" : entry.reason));
        table->setItem(i, 3, new QTableWidgetItem(
            entry.addedTime.isValid() ? entry.addedTime.toString("yyyy-MM-dd hh:mm:ss") : "--"));
    }
    layout->addWidget(table);

    QPushButton *closeBtn = new QPushButton("关闭");
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dialog.exec();
}
