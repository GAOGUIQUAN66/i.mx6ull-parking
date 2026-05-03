#include "querywindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

// Colors
#define COLOR_BG_DARK "#1a1a2e"
#define COLOR_BG_PANEL "#16213e"
#define COLOR_ACCENT "#e94560"
#define COLOR_SUCCESS "#4ecca3"
#define COLOR_WARNING "#ffd93d"
#define COLOR_TEXT_GRAY "#a0a0a0"

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
    setWindowTitle("车辆记录查询");
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
    loadPage(1);
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

    // Header
    QWidget *header = new QWidget();
    header->setFixedHeight(50);
    header->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #0f3460); border-bottom: 2px solid %2;").arg(COLOR_BG_PANEL, COLOR_ACCENT));

    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 20, 0);

    QLabel *titleLabel = new QLabel("车辆记录查询");
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
    searchPanel->setFixedHeight(70);

    QHBoxLayout *searchLayout = new QHBoxLayout(searchPanel);
    searchLayout->setContentsMargins(20, 15, 20, 15);
    searchLayout->setSpacing(15);

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

    QPushButton *resetBtn = new QPushButton("重置");
    resetBtn->setStyleSheet("background: #333; border: 1px solid #555; color: white;");
    connect(resetBtn, &QPushButton::clicked, this, &QueryWindow::onResetClicked);

    searchLayout->addWidget(plateLabel);
    searchLayout->addWidget(m_plateEdit);
    searchLayout->addWidget(dateLabel);
    searchLayout->addWidget(m_dateEdit);
    searchLayout->addWidget(statusLabel);
    searchLayout->addWidget(m_statusCombo);
    searchLayout->addWidget(searchBtn);
    searchLayout->addWidget(resetBtn);
    searchLayout->addStretch();

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
        connect(btn, &QPushButton::clicked, [this, i]() { onPageClicked(i + 1); });
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

    m_currentPage = 1;
    loadPage(1);
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

void QueryWindow::loadPage(int page)
{
    if (!m_db || !m_db->isOpen()) return;

    m_currentPage = page;

    // 计算日期范围
    QDate startDate = m_queryDate;
    QDate endDate = m_queryDate;

    // 查询数据
    QList<ParkingRecord> allRecords = m_db->queryHistory(startDate, endDate, m_queryPlate);

    // 根据状态筛选
    QList<ParkingRecord> filteredRecords;
    for (const ParkingRecord &record : allRecords) {
        if (m_queryStatus == -1) {
            filteredRecords.append(record);
        } else if (m_queryStatus == 0 && record.exitTime.isNull()) {
            filteredRecords.append(record); // 停车中
        } else if (m_queryStatus == 1 && !record.exitTime.isNull()) {
            filteredRecords.append(record); // 已出库
        }
    }

    m_totalRecords = filteredRecords.size();
    m_totalPages = (m_totalRecords + m_pageSize - 1) / m_pageSize;
    if (m_totalPages < 1) m_totalPages = 1;

    // 计算当前页数据
    int startIdx = (page - 1) * m_pageSize;
    int endIdx = qMin(startIdx + m_pageSize, m_totalRecords);

    QList<ParkingRecord> pageRecords;
    for (int i = startIdx; i < endIdx; i++) {
        pageRecords.append(filteredRecords[i]);
    }

    updateTable(pageRecords);
    updatePagination();
}

void QueryWindow::updateTable(const QList<ParkingRecord> &records)
{
    m_table->setRowCount(records.size());

    for (int i = 0; i < records.size(); i++) {
        const ParkingRecord &record = records[i];

        // 序号
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number((m_currentPage - 1) * m_pageSize + i + 1));
        idItem->setTextAlignment(Qt::AlignCenter);

        // 车牌号
        QTableWidgetItem *plateItem = new QTableWidgetItem(record.plateNumber);
        plateItem->setForeground(QColor(COLOR_SUCCESS));
        plateItem->setFont(QFont("Microsoft YaHei", 12, QFont::Bold));

        // 入场时间
        QTableWidgetItem *entryItem = new QTableWidgetItem(record.entryTime.toString("MM-dd hh:mm"));

        // 出场时间
        QTableWidgetItem *exitItem = new QTableWidgetItem(record.exitTime.isNull() ? "--" : record.exitTime.toString("MM-dd hh:mm"));

        // 停车时长
        int hours = record.duration / 60;
        int mins = record.duration % 60;
        QTableWidgetItem *durationItem = new QTableWidgetItem(QString("%1小时%2分").arg(hours).arg(mins));

        // 费用
        QTableWidgetItem *feeItem = new QTableWidgetItem(record.fee > 0 ? QString("¥%1").arg(record.fee, 0, 'f', 2) : "--");
        feeItem->setForeground(QColor(COLOR_WARNING));
        feeItem->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));

        // 状态
        QTableWidgetItem *statusItem = new QTableWidgetItem(record.exitTime.isNull() ? "停车中" : "已出库");
        statusItem->setBackground(record.exitTime.isNull() ? QColor(COLOR_WARNING) : QColor(COLOR_SUCCESS));
        statusItem->setForeground(QColor("#000000"));
        statusItem->setTextAlignment(Qt::AlignCenter);

        m_table->setItem(i, 0, idItem);
        m_table->setItem(i, 1, plateItem);
        m_table->setItem(i, 2, entryItem);
        m_table->setItem(i, 3, exitItem);
        m_table->setItem(i, 4, durationItem);
        m_table->setItem(i, 5, feeItem);
        m_table->setItem(i, 6, statusItem);
    }
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
