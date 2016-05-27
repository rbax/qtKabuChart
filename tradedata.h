/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#ifndef TRADEDATA_H
#define TRADEDATA_H

#include <QtCore>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

struct brandInfo{
    int code;
    QString name;
    QString market;
    QString marketsymbol;
    QString indust;
    int order;
};
extern struct brandInfo BrandInfo;

class tradeData
{

public:
    tradeData();
    ~tradeData();

    QSqlDatabase db;
    int initializeDb(QString datadirectory, QString indexfile, bool iscredit);
    void setCredit(bool flag);
    bool isCredit();
    qint32 lastDateN225();
    void setIndextext(QString path);
    void setDatadir(QString dir);
    bool ReadIndexToDb();
    QString getIndextext();
    QString getDatadir();
    void closeDb();
    bool getBrandInfo(int code);
    brandInfo info;

    QList<QDate> list_date[3];
    QList<double> list_open[3];
    QList<double> list_high[3];
    QList<double> list_low[3];
    QList<double> list_close[3];
    QList<qlonglong> list_volume[3];

    int load(int code, int ashi = 0);
    void convert(int ashi);

private:
    QString pathIndex;
    QString dirData;
    bool containsCredit;
    bool isDebug;
    bool checkDb();

};

#endif // TRADEDATA_H
