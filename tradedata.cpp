/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#include "tradedata.h"
#include <iostream>
#include <fstream>
#include <QDebug>

struct brandInfo BrandInfo;

tradeData::tradeData(){

}

tradeData::~tradeData(){
}

void tradeData::setCredit(bool flag){
    containsCredit = flag;
}
bool tradeData::isCredit(){
    return containsCredit;
}

/*
銘柄一覧と追加のお気に入り銘柄一覧はsqlite3でlocalに保存され、実行終了時にメモリ上の同名テーブルに読み込み上書きされる
銘柄一覧は、k-dbからデータをダウンロードして初めて取得保存される。
以降、ダウンロードごとに更新
splitsテーブルはindex.txtから分割した日付と倍率を取得。load()で銘柄を読み込む時に修正値を計算して、seriesテーブルに保存。
*/
int tradeData::initializeDb(QString datadirectory, QString indexfile, bool iscredit){
    dirData = datadirectory;
    pathIndex = indexfile;
    containsCredit = iscredit;
    db=QSqlDatabase::addDatabase("QSQLITE","trade");
    db.setDatabaseName(":memory:");
    db.setHostName("ub");
    db.setUserName("n");
    if(db.open()){
        QSqlQuery query=QSqlQuery(db);

        if(!query.exec("CREATE TABLE splits (code integer, date integer, split real, primary key(code, date))"))
            qDebug() << "sqlite error splits table not created";
        //indicatorsテーブルはもう使わない
        if(!query.exec("create table indicators (item text, ashi integer, value integer, color text, width integer, flag inetger, primary key(item, ashi, value))"))
            qDebug() << "sqlite error setting table not created";

        qDebug() << "split table" << query.lastError();
        QString file_db_name = "sqlite.db";
        QFile file(file_db_name);
        bool isExdb = file.exists();
        QString sqlAttach = QString("ATTACH DATABASE '%1' AS fileDB").arg(QDir::toNativeSeparators(file_db_name));
        QSqlQuery q_attach = db.exec(sqlAttach);
        qDebug() << "attach: " << q_attach.lastError();

        query.exec("CREATE TABLE favorite (code integer, folder text, primary key(code, folder))");
        query.exec("CREATE TABLE brands (code integer primary key, name text, market text, marketsymbol text, unit integer, obs integer)");
        if(!isExdb){
            //local保存用のテーブル
            bool f = query.exec("CREATE TABLE fileDB.favorite (code integer, folder text, primary key(code, folder))");
            f = query.exec("CREATE TABLE fileDB.brands (code integer primary key, name text, market text, marketsymbol text, unit integer, obs integer)");
            if(f)
             qDebug() << "local db created";
            else
                qDebug() << "local db create error";
            qDebug() << "local db" << query.lastError();
        }
        query.exec("replace into favorite select * from fileDB.favorite");
        query.exec("replace into brands select * from fileDB.brands");
qDebug() << "favorite copy ";
qDebug() << query.lastError();
QString sql =QString::fromLocal8Bit( "replace into brands(code, name, market, marketsymbol) values (101, '日経平均', '指標', 'I')");
if(!query.exec(sql))
    qDebug() << query.lastError();

query.exec(QString::fromLocal8Bit("update brands set marketsymbol = 'T1' where market = '東証1部'"));
query.exec(QString::fromLocal8Bit("update brands set marketsymbol = 'T2' where market = '東証2部'"));
query.exec(QString::fromLocal8Bit("update brands set marketsymbol = 'M' where market = '東証マザーズ'"));
query.exec(QString::fromLocal8Bit("update brands set marketsymbol = 'JS' where market = 'JQスタンダード'"));
query.exec(QString::fromLocal8Bit("update brands set marketsymbol = 'JG' where market = 'JQグロース'"));
//銘柄変更ごとに修正株価を保持するseriesテーブル
//ashi 0:日足 1:週足 2:月足
query.exec("create table series "
                "(code integer, offset integer, date integer, ashi integer"
                ", open real, high real, low real, close real, volume integer"
                ", ma1 real, ma2 real, ma3 real, ma4 real"
                ", t real, k real, c real, s1 real, s2 real"
                ", bb_m real, bb real, pb real"
                ", MACD real null default null, MACD_sig real null default null"
                ", RSI real, MOMENTUM real, ATR real, primary key(offset, ashi))");
qDebug() << "create series" << query.lastError();
query.exec("create table talib (name text primary key, begin integer, elem integer"
           ", period1 integer, period2 integer, period3 integer)");
qDebug() << "create talib" << query.lastError();
query.exec("insert into talib(name, period1, period2, period3) "
           "values ('MA1', 5, NULL, NULL), ('MA2', 25, NULL, NULL), ('MA3', 75, NULL, NULL), ('MA4', 200, NULL, NULL)"
           ", ('BB', 20, 1, 1), ('PB', 0.02, 0.2, NULL)"
           ", ('MACD', 12, 26, 9), ('RSI', 14, NULL, NULL)"
           ", ('MOMENTUM', 10, NULL, NULL), ('ATR', 14, NULL, NULL);");
qDebug() << "insert talib" << query.lastError();


//query.exec("create index offset on series(offset)");
//query.exec("create index ashi on series(ashi)");
//k-dbからのダウンロード時に一時的にデータを保存するテーブル
query.exec("create table kdb(code integer, name text, market text, smarket text, date integer"
       ", open real, high real, low real, close real, volume integer, primary key(code, date));");
//query = db.exec("create index offset on series(offset)");
qDebug() << "create kdb" << query.lastError();


qDebug() << "qsqlite initialized in tradeData::initializedDb";

        query.exec("select count(*) from brands where code > 1300;");
        query.next();
        int count = query.value(0).toInt();
        if(count == 0)
            return 1;
        else
            return 0;
   }

    return -1;

}

void tradeData::closeDb(){

    QString connection;
    {
        qDebug() <<  __FILE__ << __FUNCTION__ << "line:" << __LINE__ << "db.connectionName()" << db.connectionName();
        //QSqlDatabase db = QSqlDatabase::database();
        connection = db.connectionName();
        db.close();
        //db.removeDatabase(connection);
    }
    QSqlDatabase::removeDatabase(connection);
}

bool tradeData::checkDb(){
        bool f = db.isOpen();
        if(!f){
            qDebug() << "db is not open  try reopen db:" << f;
            return db.open();
        }
    return true;
}

//株価データ読み込み本体
int tradeData::load(int code, int ashi){
    double ratio = code > 1300 ? 0.1 : 0.01;
    QByteArray dir = dirData.toLocal8Bit();
    char fn[255];
    sprintf(fn, "%s/%04d", dir.constData(), code);
    std::ifstream ifs( fn, std::ios::in | std::ios::binary );
    if (ifs){

        info.code=code;
        info.indust = "";
        info.market = "";
        info.marketsymbol = "";
        info.name = "";
        QSqlQuery query = QSqlQuery(db);
        if(query.exec(QString("select name, market, marketsymbol from brands where code = %1").arg(code))){
            if(query.size() > 0){
                query.first();
                info.name = query.value(0).toString();
                info.market = query.value(1).toString();
                info.marketsymbol = query.value(2).toString();
            }
        }
        //ta-libがデータを配列で読み込む仕様なので、変換しやすいように日付ごとにまとめずに、終値等種類ごとのリストにした
        for(int i =0 ; i < 3 ; i++){
            list_date[i].clear();
            list_open[i].clear();
            list_high[i].clear();
            list_low[i].clear();
            list_close[i].clear();
            list_volume[i].clear();
        }

        QMap<quint32, double> splits;
        query = db.exec("delete from series");
        //qDebug() << "series delete" << query.lastError();
        double split = 1.0;
        quint32 buf;
        int i = 0;
        quint32 idate;
        QDate date;
        double open = 0;
        double high = 0;
        double low = 0;
        double close = 0;
        int volume = 0;
        //containsCredit = true;
        int fieldsCount = containsCredit ? 8 : 6;
        //qDebug() << "td is omega (fields count) " << containsCredit << fieldsCount;
        bool checkDate = true;
        int count = 0;
        //int count_vol = 0;
        //バイナリデータを32bit整数ごとに読み込む
        while (ifs.read(reinterpret_cast<char *>(&buf), sizeof(quint32))) {
            int year = 0;
            int month = 0;
            int day = 0;
            switch(i % fieldsCount){
                case 0:
                    idate = buf;
                    if(idate < 19000000 || idate > 21000000)
                        checkDate = false;
                    year = buf / 10000;
                    month = (buf - year * 10000) / 100;
                    day = buf - year * 10000 - month * 100;
                    date = QDate(year, month, day);
                    if(i == 0){
                        query.exec(QString("select date, split from splits where code = %1 and date >= %2 order by date").arg(code).arg(buf));
                        while(query.next()){
                            splits.insert(query.value(0).toInt(), query.value(1).toDouble());
                            qDebug() << code << buf << "split:" << query.value(0).toInt() << query.value(1).toDouble();
                        }
                    }
                    for(int j = 0 ; j < splits.count(); j++){
                        //int yy = splits.keys().at(j) / 10000;
                        //int mm = (splits.keys().at(j) - yy * 10000) / 100;
                        //int dd = splits.keys().at(j) - year * 10000 - month * 100;
                        if(splits.keys().at(j) > buf){
                            split = splits.values().at(j);
                            break;
                        }
                        else if(j == splits.count() - 1 && buf >= splits.keys().at(j))
                            split = 1.0;
                        //else
                        //    split = splits.values().at(j);
                    }
                    break;
                case 1:
                    open =(double)buf * ratio / split;
                    break;
                case 2:
                    high = (double)buf * ratio / split;
                    break;
                case 3:
                    low = (double)buf * ratio / split;
                    break;
                case 4:
                    close = (double)buf * ratio / split;
                    break;
                case 5:
                    volume= buf * split * (isCredit() ? 1 : 100);
                    if(close > 0){
                        list_date[0].append(date);
                        list_open[0].append(open);
                        list_high[0].append(high);
                        list_low[0].append(low);
                        list_close[0].append(close);
                        list_volume[0].append(volume);
                        bool f = query.exec(QString("insert into series (code, offset, date, ashi, open, high, low, close, volume)"
                                           " values (%1, %2, %3, %4, %5, %6, %7, %8, %9)")
                                   .arg(code).arg(count).arg(idate).arg(0).arg(open).arg(high).arg(low).arg(close).arg(volume));
                        if(!f)
                            qDebug() << "insert error on td";
                        count++;
                    }
                    break;
            }
            i++;
        }
        info.code = code;
        query.exec(QString("select * from brands where code = %1").arg(code));
        //if(query.size() > 0){
            query.first();
            info.name = query.value(1).toString();
            info.market = query.value(2).toString();
        if(!checkDate){
            for(int i =  0; i < list_date[0].count() ; i++){
                qDebug() << "tradeData loaded ashi;" << ashi << list_date[0][i] << list_open[0][i] << list_high[0][i] << list_low[0][i]<< list_close[0][i] << list_volume[0][i];
            }
            return -1; //not date
        }

    //isDebug = true;
        if(isDebug){
            for(int i =   qMax(list_date[0].count() - 3, 0) ; i < list_date[0].count() ; i++){
                qDebug() << "tradeData loaded ashi;" << ashi << list_date[0][i] << list_open[0][i] << list_high[0][i] << list_low[0][i]<< list_close[0][i] << list_volume[0][i];
            }
        }
        if(ashi > 0)
            convert(ashi);
        return 0;
    }
    else
        return 1;   //file not found
}

//週足、月足への変換
void tradeData::convert(int ashi){
    double open = 0;
    double high = 0;
    double low = 0;
    qlonglong volume = 0;
    int count = 0;
    if(ashi > 0 && ashi < 3 && list_date[ashi].count() == 0){
        QSqlQuery query = QSqlQuery(db);
        //QDate date;
        for(int i = 0; i < list_date[0].count(); i++){
            if(open == 0){
                open = list_open[0].at(i);
                high = list_high[0].at(i);
                low = list_low[0].at(i);
            }
            else{
                if(list_high[0].at(i) > high)
                    high = list_high[0].at(i);
                if(list_low[0].at(i) < low)
                    low = list_low[0].at(i);
            }
            volume += list_volume[0].at(i);
            if(i == list_date[0].count() - 1
                    || ((ashi  == 1 && list_date[0].at(i).weekNumber() != list_date[0].at(i + 1).weekNumber() ) |
                        (ashi == 2 && list_date[0].at(i).month() != list_date[0].at(i + 1).month() ) ) ){
                list_date[ashi].append(list_date[0].at(i));
                list_close[ashi].append(list_close[0].at(i));
                list_high[ashi].append(high);
                list_low[ashi].append(low);
                list_open[ashi].append(open);
                list_volume[ashi].append(volume);
                quint32 idate = list_date[0].at(i).year() * 10000 + list_date[0].at(i).month() * 100 + list_date[0].at(i).day();
                query.exec(QString("insert into series (code, offset, date, ashi, open, high, low, close, volume)"
                                   " values (%1, %2, %3, %4, %5, %6, %7, %8, %9)")
                           .arg(info.code).arg(count).arg(idate).arg(ashi).arg(open).arg(high).arg(low).arg(list_close[0].at(i)).arg(volume));
                open = 0;
                high = 0;
                low = 0;
                volume = 0;
                count++;
            }

        }
/*
        for(int i =  list_date[ashi].count() - 3; i < list_date[ashi].count() ; i++){
            qDebug() << "ashi;" << ashi << list_date[ashi][i] << list_open[ashi][i] << list_high[ashi][i] << list_low[ashi][i] << fixed << list_close[ashi][i] << list_volume[ashi][i];
        }
*/
    }
}

bool tradeData::getBrandInfo(int code){
    checkDb();
    QSqlQuery query(QString("select name, market, indust from brands where code = %1").arg(code));
    if(query.first()){
        info.code = code;
        info.name = query.value(0).toString();
        info.market = query.value(1).toString();
        info.indust = query.value(2).toString();
        return true;
    }
    else
        return false;
}

//index.txtの読み込み
bool tradeData::ReadIndexToDb(){

    //index.txtはshift-jis
    QTextCodec* codec = QTextCodec::codecForName("Shift-JIS");
    QFile file( pathIndex );
    if(file.exists()){
        if(!db.isOpen()) db.open();
        QSqlQuery query=QSqlQuery(db);
        if (file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
            query.prepare("insert INTO brands (code, name, market, unit, obs) VALUES (:code, :name, :market, :unit, :obs)");
            QTextStream stream(&file);
            stream.setCodec( codec );
            QSqlQuery query2=QSqlQuery(db);
            query2.exec("DROP TABLE IF EXISTS splits");
            query2.clear();
            if(!query2.exec("CREATE TABLE splits (code integer, date integer, split real, primary key(code, date))"))
                qDebug() << "sqlite error splits table not created";
            query2.prepare("insert into splits (code, date, split) values (:code, :date, :split);");
            while (!stream.atEnd())
            {
                QString line = stream.readLine(); //read one line at a time
                QStringList lstLine = line.split(",");
                if(lstLine.count() > 3){
                    QSqlQuery query3=QSqlQuery(db);
                    query3.exec("select count(*) from brands where code = " + lstLine.at(0));
                    if(query3.size() == 0 && !line.contains(",OBS")){
                        query.bindValue(":code", lstLine.at(0));
                        query.bindValue(":name", lstLine.at(1));
                        query.bindValue(":market", lstLine.at(2));
                        query.bindValue(":unit", lstLine.at(3));
                        if(line.contains(",OBS"))   //obs:上場廃止銘柄
                            query.bindValue(":obs", 1);
                        else
                            query.bindValue(":obs", 0);
                        query.exec();
                    }

                    if(line.contains("S:") && !line.contains(",OBS")){
                        double ratio = 1.0;
                        //qDebug() << line;
                        //QMap<quint32, double> splits;
                        std::map<quint32, double> splits;
                        for(int i = 0; i < lstLine.count(); i++){
                            if(lstLine.at(i).indexOf("S:") == 0){
                                //qDebug() << lstLine.at(i);
                                QStringList ss = lstLine.at(i).split(":");
                                int date = ss[1].toLong();
                                if(date > 19900000 && date < 21000000)
                                    splits.insert( std::map<quint32, double>::value_type(date, ss[2].toDouble()));
                            }
                        }
                        //QMap<quint32, double>::iterator it;
                        //std::map<quint32, double>::iterator it;
                        /*
                        std::map<quint32, double>::iterator it = splits.begin();
                        while( it != splits.end() )
                        {
                            qDebug() << lstLine.at(0) << (*it).first << ":" << (*it).second;
                            ++it;
                        }
                        */
                        std::map<quint32, double>::reverse_iterator it = splits.rbegin();
                        while( it != splits.rend() )
                        {
                            ratio *= (*it).second;
                            query2.bindValue(":code", lstLine.at(0));
                            query2.bindValue(":date", (*it).first);
                            query2.bindValue(":split", ratio);
                            query2.exec();
                            //qDebug() << lstLine.at(0) << (*it).first << ":" << ratio << f;
                            ++it;
                        }
                        /*
                        for(it = splits.end(); it != splits.begin(); --it){
                            qDebug() << lstLine.at(0) << it.key() << it.value();
                        }
                        */
                    }

                }
             }
            query2.clear();
            query.clear();
            //qDebug() << "index.txt to sqlite brands" << query.lastError() << "splits" << query2.lastError();
        }
        qDebug() << "index.txt readed:" << file.exists() << pathIndex;
        return true;
    }
    qDebug() << "index.txt not exists:" << pathIndex;
    return false;
}



void tradeData::setIndextext(QString path){
    pathIndex = path;
}
QString tradeData::getIndextext(){
    return pathIndex;
}

void tradeData::setDatadir(QString dir){
    dirData = dir;
}
QString tradeData::getDatadir(){
    return dirData;
}
