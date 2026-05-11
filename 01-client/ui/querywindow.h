#ifndef QUERYWINDOW_H
#define QUERYWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QTableWidget>
#include <QGroupBox>
#include <QDateTime>

#include "../database/database.h"

/**
 * @brief 车辆记录查询窗口
 *
 * 支持按车牌号、日期、状态筛选
 * 显示表格和分页功能
 */
class QueryWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit QueryWindow(Database *db, QWidget *parent = nullptr);
    ~QueryWindow();

signals:
    /**
     * @brief 返回主界面信号
     */
    void backToMain();

private slots:
    void onSearchClicked();
    void onResetClicked();
    void onBackClicked();
    void onPrevPage();
    void onNextPage();
    void onPageClicked(int page);

private:
    void setupUI();
    void loadPage(int page);
    void updatePagination();
    void updateTable(const QList<ParkingRecord> &records);

    // 查询条件
    QLineEdit *m_plateEdit;
    QDateEdit *m_dateEdit;
    QComboBox *m_statusCombo;

    // 表格
    QTableWidget *m_table;

    // 分页
    QLabel *m_totalLabel;
    QWidget *m_pageButtons;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QList<QPushButton*> m_pageBtns;

    // 数据
    Database *m_db;
    int m_currentPage;
    int m_totalPages;
    int m_pageSize;
    int m_totalRecords;

    // 当前查询条件
    QString m_queryPlate;
    QDate m_queryDate;
    int m_queryStatus; // -1:全部, 0:停车中, 1:已出库
};

#endif // QUERYWINDOW_H
