/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 *
 * This source need open-source(BSD licence) version of
 *  TA-Lib(Technical Analysis Library)   see http://ta-lib.org
 * *******************************************************************/


#include "fortalib.h"

#ifdef TA_LIBC_H

bool calcIndicators(tradeData *td, int ashi, int from, QMap<QString, QString> setting){
    int n= td->list_date[ashi].count();
    double * close = new double[n];
    double * high = new double[n];
    double * low = new double[n];
    for(int i = 0; i < td->list_close[ashi].count() ; i++){
        close[i] = td->list_close[ashi].at(i);
        if(setting["pb"] == "1" || setting["oscillator"] == "ATR"){
            high[i] = td->list_high[ashi].at(i);
            low[i] = td->list_low[ashi].at(i);
        }
    }
    int period_macd[3] = {12, 26, 9};
    int period_bb[3] = {20, 1, 1};
    int period_rsi = 14;
    int period_atr = 14;
    int period_momentum = (ashi == 0 ? 10 : (ashi == 1 ? 9 : 3));

    QSqlQuery query=QSqlQuery(td->db);
    if(!td->db.isOpen()) td->db.open();
    query.exec("select * from talib;");
    while(query.next()){
        QString name = query.value("name").toString();
        if(name == "MACD"){
            if(!query.value("period1").isNull() && query.value("period1").toInt() > 0)
                period_macd[0] = query.value("period1").toInt();
            if(!query.value("period2").isNull() && query.value("period2").toInt() > 0)
                period_macd[1] = query.value("period2").toInt();
            if(!query.value("period3").isNull() && query.value("period3").toInt() > 0)
                period_macd[2] = query.value("period3").toInt();
        }
        else if(name == "BB"){
            if(!query.value("period1").isNull() && query.value("period1").toInt() > 0)
                period_bb[0] = query.value("period1").toInt();
            if(!query.value("period2").isNull() && query.value("period2").toInt() > 0)
                period_bb[1] = query.value("period2").toInt();
            if(!query.value("period3").isNull() && query.value("period3").toInt() > 0)
                period_bb[2] = query.value("period3").toInt();
        }
        else if(name == "PB"){
        }
        else if(name == "RSI"){
            if(!query.value("period1").isNull() && query.value("period1").toInt() > 0)
                period_rsi = query.value("period1").toInt();
        }
        else if(name == "ATR"){
            if(!query.value("period1").isNull() && query.value("period1").toInt() > 0)
                period_atr = query.value("period1").toInt();
        }
        else if(name == "MOMENTUM"){
            if(!query.value("period1").isNull() && query.value("period1").toInt() > 0)
                period_momentum = query.value("period1").toInt();
        }
    }

    // ma
    if(setting["ma1"].toInt() > 0 || setting["ma2"].toInt() > 0 || setting["ma3"].toInt() > 0 || setting["ma4"].toInt() > 0){
        double ** ma = new double*[4];
        for( int i=0; i < 4; i++ ) {
            ma[i] = new double[n];
        }
        int *begin = new int[4];
        int *elem = new int[4];

        for(int i = 0 ; i < 4 ; i++){
            if(setting[QString("ma%1").arg(i + 1)].toInt() > 0 && setting[QString("ma%1").arg(i + 1)].toInt() < 500){
                TA_MA(0, n - 1, &close[0], setting[QString("ma%1").arg(i + 1)].toInt(), TA_MAType_SMA, &begin[i], &elem[i], &ma[i][0]);
                query.exec(QString("update talib set begin = %1, elem = %2, period1 = %3 where name = 'MA%4'")
                           .arg(begin[i]).arg(elem[i]).arg(setting[QString("ma%1").arg(i + 1)]).arg(i + 1));
            }
        }
        query.prepare("update series set ma1 = :ma1, ma2 = :ma2, ma3 = :ma3, ma4 = :ma4 where ashi = :ashi and offset = :offset;");
        for(int i = from ; i < td->list_date[ashi].count() ; i++){
            query.bindValue(":ashi", ashi);
            query.bindValue(":offset", i);
            query.bindValue(":ma1", (setting["ma1"].toInt() > 0 && i >= begin[0]) ? ma[0][i - begin[0]] : 0);
            query.bindValue(":ma2", (setting["ma2"].toInt() > 0 && i >= begin[1]) ? ma[1][i - begin[1]] : 0);
            query.bindValue(":ma3", (setting["ma3"].toInt() > 0 && i >= begin[2]) ? ma[2][i - begin[2]] : 0);
            query.bindValue(":ma4", (setting["ma4"].toInt() > 0 && i >= begin[3]) ? ma[3][i - begin[3]] : 0);
            if(!query.exec())
              qDebug() << "update ma " << i <<query.lastError();
        }
        //------ma delete[]
        delete[] begin;
        delete[] elem;
        for( int i=0; i<4; i++ ) {
            delete[] ma[i];
        }
        delete[] ma;
    }

    if(setting["oscillator"] == "MACD"){
        int begin_macd;
        int elem_macd;
        double *macd = new double[n];
        double *macd_sig = new double[n];
        double *macd_hist = new double[n];
        TA_MACD(0, n - 1, &close[0], period_macd[0], period_macd[1], period_macd[2], &begin_macd, &elem_macd, &macd[0], &macd_sig[0], &macd_hist[0]);
        query.exec(QString("update talib set begin = %1, elem = %2 where name = 'MACD'")
                   .arg(begin_macd).arg(elem_macd));
        query.prepare("update series set MACD = :macd, MACD_sig = :macd_sig where ashi = :ashi and offset = :offset;");
        for(int i = from ; i < td->list_date[ashi].count() ; i++){
            query.bindValue(":ashi", ashi);
            query.bindValue(":offset", i);
            query.bindValue(":macd", i >= begin_macd ? macd[i - begin_macd] : QVariant(QVariant::Double));
            query.bindValue(":macd_sig", (i >= begin_macd) ? macd_sig[i - begin_macd] : QVariant(QVariant::Double));
            query.exec();
        }
        //------macd
        delete[] macd;
        delete[] macd_sig;
        delete[] macd_hist;
    }
    else if (setting["oscillator"] == "RSI"){
        int begin_rsi;
        int elem_rsi;
        double *rsi = new double[n];
        TA_RSI(0, n - 1, &close[0], period_rsi, &begin_rsi, &elem_rsi, &rsi[0]);
        query.exec(QString("update talib set begin = %1, elem = %2 where name = 'RSI'")
                   .arg(begin_rsi).arg(elem_rsi));
        query.prepare("update series set RSI = :rsi where ashi = :ashi and offset = :offset;");
        for(int i = from ; i < td->list_date[ashi].count() ; i++){
            query.bindValue(":ashi", ashi);
            query.bindValue(":offset", i);
            query.bindValue(":rsi", (i >= begin_rsi) ? rsi[i - begin_rsi] : 0);
            if(!query.exec())
                qDebug() << "update rsi " << i <<query.lastError();
        }
        delete[] rsi;
    }
    else if (setting["oscillator"] == QString::fromLocal8Bit("モメンタム")){
        int begin, elem;
        double *moment = new double[n];
        TA_MOM(0, n - 1, &close[0], period_momentum, &begin, &elem, &moment[0]);
        query.exec(QString("update talib set begin = %1, elem = %2 where name = 'MOMENTUM'")
                   .arg(begin).arg(elem));
        query.prepare("update series set MOMENTUM = :momentum where ashi = :ashi and offset = :offset;");
        for(int i = from ; i < td->list_date[ashi].count() ; i++){
            query.bindValue(":ashi", ashi);
            query.bindValue(":offset", i);
            query.bindValue(":momentum", (i >= begin) ? moment[i - begin] : 0);
            if(!query.exec())
                qDebug() << "update momentum " << i <<query.lastError();
        }
        delete[] moment;

    }
    else if (setting["oscillator"] == "ATR"){
        int begin, elem;
        double *atr = new double[n];;
        TA_ATR(0, n - 1, &high[0], &low[0], &close[0], period_atr, &begin, &elem, &atr[0]);
        query.exec(QString("update talib set begin = %1, elem = %2 where name = 'ATR'")
                   .arg(begin).arg(elem));
        query.prepare("update series set ATR = :atr where ashi = :ashi and offset = :offset;");
        for(int i = from ; i < td->list_date[ashi].count() ; i++){
            query.bindValue(":ashi", ashi);
            query.bindValue(":offset", i);
            query.bindValue(":atr", (i >= begin) ? atr[i - begin] : 0);
            if(!query.exec())
                qDebug() << "update atr " << i <<query.lastError();
        }
        delete[] atr;
    }

    if(setting["bb"] == "1"){
        double *bb_u = new double[n];
        double *bb_m = new double[n];
        double *bb_l = new double[n];
        int begin_bb;
        int elem_bb;
        TA_BBANDS(0, n - 1, &close[0], period_bb[0], period_bb[1], period_bb[2], TA_MAType_SMA, &begin_bb, &elem_bb, &bb_u[0], &bb_m[0], &bb_l[0]);
        query.exec(QString("update talib set begin = %1, elem = %2 where name = 'BB'")
                   .arg(begin_bb).arg(elem_bb));
        query.prepare("update series set bb_m = :bb_m, bb = :bb where ashi = :ashi and offset = :offset;");
        for(int i = from ; i < td->list_date[ashi].count() ; i++){
            query.bindValue(":ashi", ashi);
            query.bindValue(":offset", i);
            query.bindValue(":bb_m", (i >= begin_bb) ? bb_m[i - begin_bb] : 0);
            query.bindValue(":bb", (i >= begin_bb) ? (bb_u[i - begin_bb] - bb_m[i - begin_bb]) : 0);
            if(!query.exec())
                qDebug() << "update bb " << i <<query.lastError();
        }
        delete[] bb_u;
        delete[] bb_m;
        delete[] bb_l;
    }
    if(setting["pb"] == "1"){
        int begin_pb;
        int elem_pb;
        double *pb = new double[n];
        TA_SAR(0, n - 1, &high[0], &low[0], 0.02, 0.2,&begin_pb, &elem_pb, &pb[0]);
        query.exec(QString("update talib set begin = %1, elem = %2 where name = 'PB'")
                   .arg(begin_pb).arg(elem_pb));
        query.prepare("update series set pb = :pb where ashi = :ashi and offset = :offset;");
        for(int i = from ; i < td->list_date[ashi].count() ; i++){
            query.bindValue(":ashi", ashi);
            query.bindValue(":offset", i);
            query.bindValue(":pb", (i >= begin_pb) ? pb[i - begin_pb] : 0);
            if(!query.exec())
                qDebug() << "update pb " << i <<query.lastError();
        }
        delete[] pb;
    }
    delete[] high;
    delete[] low;
    delete[] close;
    return true;
}
#endif
