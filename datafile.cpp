/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#include "datafile.h"

#include <fstream>
#include <QtNetwork>
#include <QRegExp>
#include <QMap>

struct structArray{
    quint32 data[8];
    qlonglong volume;
};

QString getSource(QString url){
    QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(response,SIGNAL(finished()),&event,SLOT(quit()));
    event.exec();
    return response->readAll();
}

QList< QStringList > readSjCsv(QString fn){
    QTextCodec* codec = QTextCodec::codecForName("Shift-JIS");
    QList< QStringList > list;
    QFile file( fn );
    if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
        QTextStream stream(&file);
        stream.setCodec( codec );
        while (!stream.atEnd())
        {
            QString line = stream.readLine(); //read one line at a time
            QStringList lstLine = line.split(",");
            list.append(lstLine);
        }
    }
    return list;
}


QMap<quint32, structArray> datafileToMap(int code, QString directory, bool isOmega = false){

    if(directory.right(1) != "/")
        directory.append("/");
    //QString filename = QString("%1/%2").arg(directory).arg(code, 4, 10, QChar('0'));
    char fn[255];
    sprintf(fn, "%s%04d", directory.toLocal8Bit().constData(), code);
    QMap<quint32, structArray> map;
    std::ifstream ifs( fn, std::ios::in | std::ios::binary );
    if (ifs){
        quint32 buf;
        int i = 0;
        structArray rec;
        while (ifs.read(reinterpret_cast<char *>(&buf), sizeof(quint32))) {
            int j = i % (isOmega ? 8 : 6);
            switch(j){
                //qDebug() << buf;
                case 0:
                    if(i > 0){
                        rec.volume = rec.data[5]; //qlonglongで保持
                        map.insert(rec.data[0], rec);
                    }
                    rec.data[0] = buf;
                    for(int k = 1 ; k < 8; k++)
                        rec.data[k] = 0;
                    break;
                default:
                    rec.data[j] = (j == 5 && !isOmega) ? buf * 100 : buf; //出来高 1/100 (omegaはそのまま)
                    break;
            }
        i++;
        }
        ifs.close();
        map.insert(rec.data[0], rec);
        bool debug = false;
        if(debug){
            QMap<quint32, structArray>::const_iterator it = map.constBegin();
            while (it != map.constEnd()) {
                qDebug() << it.key();
                ++it;
            }
        }
    }
    else{
        //qDebug() << "new brand add" << code;
        map.clear();
    }
    return map;
}

quint32 getLastDate(int code, QString directory, bool isOmega){
    QMap<quint32, structArray> map = datafileToMap(code, directory, isOmega);
    if(map.count() == 0)
        return 0;
    else
        return map.keys().last();
}

int writeDatafile(int code, QString directory, QMap<quint32, structArray> map, bool isOmega){
    std::ofstream ofs;

    char fn[255];
    if(directory.right(1) != "/")
        directory.append("/");
    QByteArray ba_dir = directory.toLocal8Bit();
    const char *dir;
    dir = ba_dir.constData();
    sprintf(fn, "%s%04d", dir, code);

    ofs.open(fn, std::ios::out|std::ios::binary|std::ios::trunc);

    if (ofs) {
        QMap<quint32, structArray>::const_iterator it = map.constBegin();
        while (it != map.constEnd()) {
            if(it.key() > 20070101 && it.key() < 21000000){
                for(int j = 0; j < 5 ; j++){
                        ofs.write(( char * ) &(it.value().data[j]), sizeof( quint32 ) );   //!!!!! &(*it)
                }

                quint32 vol = (quint32)(it.value().volume / (isOmega ? 1 : 100));
                //qDebug() << it.value().data[0] << vol;
                ofs.write(( char * ) &vol, sizeof( quint32 ) );

                if(isOmega){
                    int sh = 0;
                    int lg = 0;
                    ofs.write(( char * ) &(sh), sizeof( quint32 ) );
                    ofs.write(( char * ) &(lg), sizeof( quint32 ) );
                }
            }
            ++it;
        }
        ofs.close();
        return 0;
    }
    else{
        qDebug() << "ファイル が開けません" << fn;
        return 1;
    }
}

int writeDatafile(int code, QString directory, structArray val, bool isOmega){
    QMap<quint32, structArray> map = datafileToMap(code, directory, isOmega);
    map.insert(val.data[0], val);
    return writeDatafile(code, directory, map, isOmega);
}

QList<quint32> downloadBrand(tradeData *td, int code, QString directory, bool isOmega, bool fAll){
    QList<quint32> list;
        QMap<quint32, structArray> map = datafileToMap(code, directory, isOmega);
        QString market = code > 1300 ? "-T" : "";
        if(td->info.market.contains(QString::fromLocal8Bit("札")))
            market = "-S";
        else if(td->info.market.contains(QString::fromLocal8Bit("福")))
            market = "-F";
        else if(td->info.market.contains(QString::fromLocal8Bit("名")))
            market = "-N";

        QString url = QString("http://k-db.com/%1/%2%3%4").arg(code < 1000 ? "indices" : "stocks").arg(code < 1000 ? "I" : "").arg(code).arg(market);
        qDebug() << url;
        QString source = getSource(url);
        if(source.contains("Error 403")){
            qDebug() << source;
            return list;
        }
        QRegExp rx("<tr><td.*>([0-9]{4})-([0-9]{1,2})-([0-9]{1,2})</td><td></td><td>(.*)</td><td>(.*)</td><td>(.*)</td><td.*>(.*)</td><td>(.*)</td>");
        rx.setMinimal(true);
        int pos = 0;
        int ratio = code < 1000 ? 100: 10;
        int count = 0;
        while ((pos = rx.indexIn(source, pos)) != -1) {
            quint32 idate = rx.cap(1).toInt() * 10000 + rx.cap(2).toInt() * 100 + rx.cap(3).toInt();
            structArray arr;
            //arr.data={};
            arr.volume=0;
            arr.data[0] = idate;
            for(int i = 1; i < 5 ; i++)
                arr.data[i] = (int)(rx.cap(i + 3).toDouble() * ratio);
            if(code > 1300){
                arr.volume = rx.cap(8).toLongLong();
            }
            map.insert(idate, arr);
            list.append(idate);
            count++;
            pos += rx.matchedLength();
        }
        if(count == 0){
            qDebug() << source;
            return list;
        }

        if(fAll){
            QRegExp rx_link("href='(.*)'>(.*)</a>");
            rx_link.setMinimal(true);
            int pos1 = 0;
            int j = 0;
            while ((pos1 = rx_link.indexIn(source, pos1)) != -1) {
                if(rx_link.cap(1).contains("/1d/")){
                    //qDebug() << rx_link.cap(1) << rx_link.cap(2);
                    if(j > 0){
                        url = QString("http://k-db.com%1").arg(rx_link.cap(1));
                        //qDebug() << url;
                        QString source2 = getSource(url);
                        pos = 0;
                        count = 0;
                        while ((pos = rx.indexIn(source2, pos)) != -1) {
                            quint32 idate = rx.cap(1).toInt() * 10000 + rx.cap(2).toInt() * 100 + rx.cap(3).toInt();
                            //qDebug() << idate;
                            structArray arr;
                            arr.data[0] = idate;
                            for(int i = 1; i < 5 ; i++)
                                arr.data[i] = (int)(rx.cap(i + 3).toDouble() * ratio);
                            if(code > 1300)
                                arr.volume = rx.cap(8).toLongLong();
                            map.insert(idate, arr);
                            list.append(idate);
                            count++;
                            pos += rx.matchedLength();
                        }
                        if(count == 0){
                            qDebug() << source2;
                            return list;
                        }
                    }
                    j++;
                }
                //qDebug() << rx_link.cap(1) << rx_link.cap(2);
                pos1 += rx_link.matchedLength();
            }
        }
        bool isDebug = false;
        if(isDebug){
            QMap<quint32, structArray>::const_iterator it = map.constBegin();
            while (it != map.constEnd()) {
                qDebug() << it.key() << it.value().data[0] << it.value().data[1] << it.value().data[2] << it.value().data[3] << it.value().data[4] << it.value().volume;
                ++it;
            }
        }

        writeDatafile(code, directory, map, isOmega);
        qDebug() << "data writed" << code;
        return list;
}

// 0:success -1:web error 1:file error
int downloadDay(tradeData *td, bool isUpdate, QString directory, bool isOmega, quint32 targetdate){
    QString sdate = "";
    if(targetdate > 20070101 && targetdate < 21000000){
        int yy = targetdate / 10000;
        int mm = (targetdate - yy * 10000) / 100;
        int dd = targetdate - yy * 10000 - mm * 100;
        sdate = QString("%1-%2-%3").arg(yy).arg(mm, 2, 10, QChar('0')).arg(dd, 2, 10, QChar('0'));
    }
    QString url = QString("http://k-db.com/indices/%1").arg(sdate);
    QString source = getSource(url);
    if(source.contains("Error 403")){
        qDebug() << source;
        return -1;
    }
    if(!td->db.isOpen())
        td->db.open();
    QSqlQuery query = QSqlQuery(td->db);
    query.prepare("INSERT INTO kdb (code, name, market, smarket, date, open, high, low, close, volume) VALUES (:code, :name, :market, :smarket, :date, :open, :high, :low, :close, :volume)");
    QRegExp rx_date(QString::fromLocal8Bit("<div.*tablecaption.>([0-9]{4})年([0-9]{1,2})月([0-9]{1,2})日.*データ</div>"));
    rx_date.setMinimal(true);
    rx_date.indexIn(source);
    quint32 year = rx_date.cap(1).toInt();
    quint32 month = rx_date.cap(2).toInt();
    quint32 day = rx_date.cap(3).toInt();
    quint32 idate = year * 10000 + month * 100 + day;
    //qDebug() << "idate" << idate;
    QRegExp rx("<tr><td.*><a.*/indices/I(.*)'>(.*)</a></td><td>(.*)</td><td>(.*)</td><td>(.*)</td><td.*>(.*)</td><td></td></tr>");
    rx.setMinimal(true);
    int pos = 0;
    int count = 0;
    bool fError = false;
    while ((pos = rx.indexIn(source, pos)) != -1) {
        //quint32 idate = rx.cap(1).toInt() * 10000 + rx.cap(2).toInt() * 100 + rx.cap(3).toInt();
        QString scode = rx.cap(1);
        QString name = rx.cap(2);
        //QString market = rx.cap(3);
        //QString smarket = rx.cap(4);
        int code = scode.toInt();
        int ratio = code < 1000 ? 100: 10;
        structArray arr;

        arr.data[0] = idate;
        for(int i = 1; i < 5 ; i++)
            arr.data[i] = (int)(rx.cap(i + 2).toDouble() * ratio);
        if(code > 1300)
            arr.volume = rx.cap(5 + 2).toLongLong();
        query.bindValue(":code", code);
        query.bindValue(":name", name);
        query.bindValue(":market", "I");
        query.bindValue(":smarket", QString::fromLocal8Bit("指標"));
        query.bindValue(":date", idate);
        query.bindValue(":open", arr.data[1]);
        query.bindValue(":high", arr.data[2]);
        query.bindValue(":low", arr.data[3]);
        query.bindValue(":close", arr.data[4]);
        query.bindValue(":volume", arr.volume);
        bool f = query.exec();
        if(!f)
            qDebug() << "kdb to db" << f << scode << name << arr.data[0] << arr.data[1] << arr.data[2] << arr.data[3] << arr.data[4] << arr.data[5];
        if(isUpdate && code > 100){
            int ret = writeDatafile(scode.toInt(), directory, arr, isOmega);
            if(ret != 0)
                fError = true;
        }
        //map.insert(idate, arr);
        count++;
        pos += rx.matchedLength();
    }
    if(count == 0)
        qDebug() << source;
    url = QString("http://k-db.com/stocks/%1").arg(sdate);
    source = getSource(url);
    if(source.contains("Error 403")){
        qDebug() << source;
        return -1;
    }

    rx.setPattern("<tr><td.*><a.*stocks/(.*).>([0-9]{4,5})-(.).(.*)</a></td><td.*>(.*)</td><td>(.*)</td><td>(.*)</td><td>(.*)</td><td.*>(.*)</td><td>(.*)</td><td>.*</td><td></td></tr>");
    rx.setMinimal(true);
    pos = 0;
    count = 0;
    int prev_code = 0;
    while ((pos = rx.indexIn(source, pos)) != -1) {
        int code = rx.cap(2).toInt();
        if(code != prev_code){
            QString scode = rx.cap(1);
            QString market = rx.cap(3);
            QString name = rx.cap(4);
            QString smarket = rx.cap(5);
            int ratio = code < 1000 ? 100: 10;
            structArray arr;

            arr.data[0] = idate;
            for(int i = 1; i < 5 ; i++)
                arr.data[i] = (int)(rx.cap(i + 5).toDouble() * ratio);
            arr.volume = rx.cap(5 + 5).toLongLong();
            query.bindValue(":code", code);
            query.bindValue(":name", name);
            if(smarket == QString::fromLocal8Bit("東証1部"))
                market = "T1";
            else if(smarket == QString::fromLocal8Bit("東証2部"))
                market = "T2";
            else if(smarket == QString::fromLocal8Bit("東証マザーズ"))
                market = "M";
            else if(smarket == QString::fromLocal8Bit("JQスタンダード"))
                market = "JS";
            else if(smarket == QString::fromLocal8Bit("JQグロース"))
                market = "JG";

            else if(smarket == QString::fromLocal8Bit("東証"))
                market = "T";
            else if(smarket.contains(QString::fromLocal8Bit("札")))
                market = "S";
            else if(smarket.contains(QString::fromLocal8Bit("名")))
                market = "N";
            else if(smarket.contains(QString::fromLocal8Bit("福")))
                market = "F";

            query.bindValue(":market", market);
            query.bindValue(":smarket", smarket);
            query.bindValue(":date", idate);
            query.bindValue(":open", arr.data[1]);
            query.bindValue(":high", arr.data[2]);
            query.bindValue(":low", arr.data[3]);
            query.bindValue(":close", arr.data[4]);
            query.bindValue(":volume", arr.volume);
            bool f = query.exec();
            if(!f)
                qDebug() << "kdb to db " << f << code << scode << name << arr.data[0] << arr.data[1] << arr.data[2] << arr.data[3] << arr.data[4] << arr.volume;
            if(isUpdate && code > 100){
                int ret = writeDatafile(code, directory, arr, isOmega);
                if(ret != 0)
                    fError = true;
            }
            count++;
        }
        prev_code = code;
        pos += rx.matchedLength();
    }
    return (fError ? 1 : 0);
}
