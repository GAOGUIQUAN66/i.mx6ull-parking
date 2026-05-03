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
 * @brief History browser
 *
 * Filter by plate/date/state
 * Table + pager
 */
class QueryWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit QueryWindow(Database *db, QWidget *parent = nullptr);
    ~QueryWindow();

signals:
    /**
     * @brief Back to main
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

    // Filters
    QLineEdit *m_plateEdit;
    QDateEdit *m_dateEdit;
    QComboBox *m_statusCombo;

    // Table
    QTableWidget *m_table;

    // Pagination
    QLabel *m_totalLabel;
    QWidget *m_pageButtons;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QList<QPushButton*> m_pageBtns;

    // State
    Database *m_db;
    int m_currentPage;
    int m_totalPages;
    int m_pageSize;
    int m_totalRecords;

    // Active filters
    QString m_queryPlate;
    QDate m_queryDate;
    int m_queryStatus; // -1 all, 0 parked, 1 exited
};

#endif // QUERYWINDOW_H
