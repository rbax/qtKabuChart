/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#include "backgroundthread.h"
#include "datafile.h"

backgroundThread::backgroundThread(){
    maxDownload = 60; //約3ヶ月
}

backgroundThread::~backgroundThread(){

}

void backgroundThread::run(){
    quint32 last = getLastDate(101, dataDir, isOmega);
    QDateTime now = QDateTime::currentDateTime();
    QDateTime now_offset = now.addSecs(- 60 * 60 * 16);
    quint32 idate = now_offset.date().year() * 10000 + now_offset.date().month() * 100 + now_offset.date().day();
    int offset = idate - last;
    //qDebug() << "last:" << last << "target:" << idate << "offset:" << offset;

    if(offset <= 1){
        downloadDay(td, isUpdate, dataDir, isOmega);
        //最終日の翌日の場合（16時以降に限定）
        QSqlQuery query = QSqlQuery(td->db);
        if(!td->db.isOpen())
            td->db.open();
        query.exec("replace into brands (code, name, market, marketsymbol) "
                   "select kdb.code, kdb.name, kdb.smarket, kdb.market from kdb where kdb.date = (select max(kdb.date) from kdb)");
        qDebug() << "replace brand from kdb" << query.lastError();
    }
    else if(offset > 1){
        QList<quint32> list = downloadBrand(td, 101, dataDir, isOmega);
        //まず日経平均１年分を更新　掲載日で未更新日のindicesとstocksを更新
        int i = 0;
        if(list.count() > 0){
            emit sendMessage(QString::fromLocal8Bit("日経平均を更新しました　銘柄ダウンロード中"));
            qSort(list);
            QListIterator<quint32> it(list);
            it.toBack();
            while(it.hasPrevious()){
                quint32 dt = it.previous();
                if(dt <= last)
                    break;
                int ret = downloadDay(td, true, dataDir, isOmega, dt);
                if(ret != 0){
                    if(ret > 0)
                        emit sendMessage(tr("file update error on %1").arg(dt));
                    else if(ret < 0)
                        emit sendMessage(tr("k-db web error on %1").arg(dt));
                    break;
                }
                else
                    emit sendMessage(tr("download success %1").arg(dt));
                qDebug() << dt;
                if(i==0){
                    QSqlQuery query = QSqlQuery(td->db);
                    if(!td->db.isOpen())
                        td->db.open();
                    query.exec("replace into brands (code, name, market, marketsymbol) "
                               "select kdb.code, kdb.name, kdb.smarket, kdb.market from kdb where kdb.date = (select max(kdb.date) from kdb)");
                    qDebug() << "replace brand from kdb" << query.lastError();
                    if(last == 0)
                        break;      //初回の場合には、１日分のみダウンロード
                }
                i++;
                if(i > maxDownload)
                    break;
            }
        }
        else
            emit sendMessage("daunload error");
    }
    else
        emit sendMessage("no new data");

    emit finishThread();

}

void backgroundThread::set(tradeData *tradedata, QString directory, bool isomega, bool isupdate){
    td = tradedata;
    dataDir = directory;
    isOmega = isomega;
    isUpdate = isupdate;
}

void backgroundThread::setMaxdownload(int countDays){
    maxDownload = countDays;
}
