/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#include "mygraphicsview.h"
#include <QDockWidget>
#include <QTreeWidget>
#include <QLineEdit>

#   include "fortalib.h"        //.proファイルでta-libとヘッダが見つかった時のみinclude

const qreal MAIN_MARGIN_WIDTH = 20;
const qreal MAIN_MARGIN_HEIGHT = 120; //ベースウィンドウからコントロールなどのマージン
const qreal MARGIN_LEFT = 10;
const qreal MARGIN_RIGHT = 60;
const qreal MARGIN_TOP = 20;
const qreal MARGIN_BOTTOM = 20;
const qreal VOLUME_RATIO = 0.1;
const qreal INDICATOR_RATIO = 0.2;

MyGraphicsView::MyGraphicsView(QWidget *parent) :
    QGraphicsView(parent)
{
    v_scene = new MyGraphicsScene();
    this->setScene(v_scene);
    this->parentWidth = parent->width();
    this->parentHeight = parent->height();
    isVolume = false;
    isOscillator = false;
    isCross=false;

    settingsLoad();
    this->scene()->setBackgroundBrush(QBrush(QColor(mapSettings["canvasColor"])));

    if(isDebug)
        qDebug() << "color set";
    //baseRect = this->scene()->addRect(0, 0, 10,  10, QPen(QColor(mapSettings["defaultPenColor"])), QBrush(QColor(mapSettings["canvasColor"])));
    baseRect = new QGraphicsRectItem();

}

MyGraphicsView::~MyGraphicsView(){
    qDebug() <<  __FILE__ << __FUNCTION__ << "line:" << __LINE__;
    settingsSave();
}

void MyGraphicsView::resizeEvent(QResizeEvent *event){
    //qDebug() <<  __FILE__ << __FUNCTION__ << "line:" << __LINE__;
    QDockWidget *dock = this->parent()->findChild<QDockWidget *>("dockWidget");
    //qDebug() << "dock width" << dock->width();
    dockWidth = dock->width();
    updateChart();
//    this->resizeEvent(event);
}

QStringList MyGraphicsView::getKeysList(){
    return sListKeys;
}

QString MyGraphicsView::getLabel(QString field){
    int index = sListKeys.indexOf(field);
    return sListLabel.value(index);
}

void MyGraphicsView::settingsLoad(){
    sListKeys << "canvasColor" << "textPenColor" << "yosenColor" << "yosenPenColor" << "insenColor" << "insenPenColor"
           << "volumeColor" << "scalePenColor"
          << "maColor1" << "maColor2" << "maColor3" << "maColor4"
          << "ichiKColor" << "ichiTColor" << "ichiCColor" << "ichiSaColor" << "ichiSbColor"
          << "bb0Color" << "bb1Color" << "bb2Color" << "bb3Color"
          << "pbPosColor" << "pbNegColor"
          << "macdColor" << "macdSigColor" << "rsiColor" << "oscBaseColor"
          << "lineColor" << "freeLineColor" << "selectColor";
    sListLabel << QString::fromLocal8Bit("背景") << QString::fromLocal8Bit("文字色")
               << QString::fromLocal8Bit("陽線") << QString::fromLocal8Bit("陽線枠")
               << QString::fromLocal8Bit("陰線") << QString::fromLocal8Bit("陰線枠")
               << QString::fromLocal8Bit("出来高") << QString::fromLocal8Bit("枠線")
               << "MA1" << "MA2" << "MA3" << "MA4"
               << QString::fromLocal8Bit("基準線") << QString::fromLocal8Bit("転換線") << QString::fromLocal8Bit("遅行線")
               << QString::fromLocal8Bit("先行A") << QString::fromLocal8Bit("先行B")
               << "BBm" << "BB1" << "BB2" << "BB3"
               << QString::fromLocal8Bit("PB陽") << QString::fromLocal8Bit("PB陰")
               << "MACD" << "signal" << "RSI"
               << QString::fromLocal8Bit("基線") << QString::fromLocal8Bit("ライン")
               << QString::fromLocal8Bit("自由直線") << QString::fromLocal8Bit("日付スケール");
    sListDefaultValues << "#000022" << "white" << "#ff3300" << "coral" << "#99ff99" << "green"
                     << "powderblue" << "navy"
                     << "red" << "lime" << "gold" << "blue"
                     << "blueviolet" << "orange" << "yellow" << "lightsteelblue" << "mistyrose"
                     << "lime" << "greenyellow" << "mintcream" << "blueviolet"
                     << "pink" << "aqua"
                      << "orangered" << "mediumspringgreen" << "deepskyblue" << "beige"
                     << "beige" << "white" << "yellow";

    sListKeys << "maWidth1" << "maWidth2" << "maWidth3" << "maWidth4";
    sListDefaultValues << "2" << "2" << "2" << "2";
    //MainWindowでView初期化前に一度読み込むが、初期化後に再度読み込む
    QSettings settings("myChart.ini", QSettings::IniFormat);
    settings.beginGroup("Chart");
    for(int i = 0; i < sListKeys.count() ; i++){
        if(sListKeys.at(i) != ""){
            mapSettings.insert(sListKeys.at(i), settings.value(sListKeys.at(i), sListDefaultValues.at(i)).toString());
            if(sListKeys.at(i) == "freeLineColor")
                v_scene->setLineColor(mapSettings["freeLineColor"]);
        }
    }
    settings.endGroup();
}

void MyGraphicsView::settingsSave(){
    QSettings settings("myChart.ini", QSettings::IniFormat);
    settings.beginGroup("Chart");
    for(int i = 0 ; i < sListKeys.count() ; i++){
        if(sListKeys.at(i) != "")
            settings.setValue(sListKeys.at(i), mapSettings[sListKeys.at(i)]);
    }
    settings.endGroup();
    qDebug() <<  __FILE__ << __FUNCTION__ << "line:" << __LINE__;
}

void MyGraphicsView::setDockWidth(qreal value){
    dockWidth = value;
}

//priceのY座標を取得
qreal MyGraphicsView::getPriceYposition(qreal price){
    qreal relative_percentage = (price - priceScaleBottom) / (priceScaleTop - priceScaleBottom);
    qreal pos = posPriceTop + (posPriceBottom - posPriceTop) * (1 - relative_percentage);
    return pos;
}
qreal MyGraphicsView::getVolumeYposition(qreal volume, qreal maxVolume){
    qreal relative_percentage = volume / maxVolume;
    qreal pos = posPriceBottom + (posVolumeBottom - posPriceBottom) * (1 - relative_percentage);
    return pos;
}
qreal MyGraphicsView::getIndiYposition(qreal value, qreal max, qreal min){
    qreal relative_percentage = (value - min) / (max - min);
    qreal pos = posVolumeBottom + (posIndicatorBottom - posVolumeBottom) * (1 - relative_percentage);
    return pos;
}

void MyGraphicsView::drawScaleDate(QDate d, QDate prev, int i,int x){
    if(i > 0 && d.month() != prev.month()){
        bool f = false;
        switch (month_interval) {
        case 12:
            if(d.month() == 1)
                f = true;
            break;
        default:
            if((d.month() - 1 ) % month_interval == 0)
                f = true;
            break;
        }
        if(f){
            baseRect->childItems().append(this->scene()->addLine(x, posPriceTop, x, posIndicatorBottom - 12, QPen(QColor(mapSettings["scalePenColor"])) ) );
            QString str;
            if(month_interval == 12)
                str = d.toString("yyyy");
            else if(d.month() == 1)
                str = d.toString("yyyy.M");
            else
                str = d.toString("M");
            QGraphicsTextItem * io = new QGraphicsTextItem;
            io->setPos(x,posIndicatorBottom);
            io->setPlainText(str);
            io->setDefaultTextColor(QColor(mapSettings["textPenColor"]));
            this->scene()->addItem(io);
            baseRect->childItems().append(io);
            //group->addToGroup(io);
        }
    }
}

void MyGraphicsView::showBrand(int code, int candleashi, int monthFrom){
    td->load(code, candleashi);
    ashi = candleashi;
    month = monthFrom;
    updateChart();
}
void MyGraphicsView::updateChart(int candleashi){
    ashi = candleashi;
    updateChart();
}

void MyGraphicsView::setMonth(int monthFrom, bool update){
    month = monthFrom;
    if(update)
        updateChart();
}

void MyGraphicsView::updateChart(QString name, QString value){
    setSetting(name, value);
    if(name == "freeLineColor"){
        v_scene->setLineColor(value);
    }
    else
        updateChart();
}

void MyGraphicsView::updateChart(QString name, int value){
    setSetting(name, tr("%1").arg(value));
    if(name == "ashi")
        ashi = value;
    else if(name == "volume")
        isVolume = value;

    updateChart();
}

/*  draw chart main  */
void MyGraphicsView::updateChart(){
   // isDebug = true;
    //this->scene()->clear();
    if(isDebug)
        qDebug() << "base rect" << baseRect->childItems().count();

    if(baseRect->childItems().count() > 0){
            for(int i = 0; i < baseRect->childItems().count(); i++)
                baseRect->childItems().at(i)->~QGraphicsItem();
            baseRect->childItems().clear();
        }
        //baseRect->~QGraphicsRectItem();
        this->scene()->clear();

    //-----------

    baseRect->setRect(0, 0, parentWidth - (dockWidth + MAIN_MARGIN_WIDTH),  parentHeight - MAIN_MARGIN_HEIGHT);
    //baseRect->setBrush(QBrush(QColor(mapSettings["canvasColor"])));
    //baseRect = this->scene()->addRect(0, 0, parentWidth - (dockWidth + MAIN_MARGIN_WIDTH),  parentHeight - MAIN_MARGIN_HEIGHT, QPen(QColor(mapSettings["defaultPenColor"])), QBrush(QColor(mapSettings["canvasColor"])));
    canvasHeight = parentHeight - MAIN_MARGIN_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM;
    canvasWidth = parentWidth - (dockWidth + MAIN_MARGIN_WIDTH) - MARGIN_LEFT - MARGIN_RIGHT;
    qreal volHeight = isVolume ? canvasHeight * VOLUME_RATIO : 0;
    qreal indiHeight = isOscillator ? canvasHeight * INDICATOR_RATIO : 0;
    qreal priceHeight = canvasHeight - volHeight - indiHeight;

    posPriceTop = MARGIN_TOP;
    posPriceBottom = MARGIN_TOP + priceHeight;
    posVolumeBottom = posPriceBottom + volHeight;
    posIndicatorBottom = posVolumeBottom + indiHeight;
    posCanvasLeft = MARGIN_LEFT;
    posCanvasRight = MARGIN_RIGHT + canvasWidth;
    if(isDebug)
        qDebug() << "canvas height:" << canvasHeight << "pos price" << posPriceBottom << "vol bottom:" << posVolumeBottom;
    //-------------------------------
    candleCount = 0;

    //qDebug() << "ashi:" << ashi;
    if(ashi > 3 || ashi < 0)
        ashi = 0;
    //if(td->info.code < 0)
      //  return;
    if(isDebug){
        qDebug() << td->info.code;
        qDebug() << "td->list_date[ashi].count()" << td->list_date[ashi].count();
    }
    if(td->list_date[ashi].count() < 1){
        return;
    }

    to = td->list_date[ashi].count() - 1;
    if(isDebug)
        qDebug() << "to:" << to;
    toDate = td->list_date[ashi].at(to);
    //qDebug() << "toDate:" << to;
    QDate offsetDate = toDate.addMonths(- month);
    if(isDebug)
        qDebug() << "ashi:" << ashi << "month:" << month << offsetDate << to << toDate;
    maxPrice = 0;
    minPrice = 0;
    maxVolume = 0;
    for(int i = to ; i >= 0 ; i--){
        if(td->list_date[ashi].at(i) < offsetDate && td->list_date[ashi].at(i).month() != offsetDate.month())
            break;
        candleCount++;
        if(maxPrice == 0){
            maxPrice = td->list_high[ashi].at(i);
            minPrice = td->list_low[ashi].at(i);
            maxVolume = td->list_volume[ashi].at(i);
            //qDebug() << maxPrice << minPrice << maxVolume;
        }
        else{
            if(td->list_high[ashi].at(i) > maxPrice)
                maxPrice = td->list_high[ashi].at(i);
            if(td->list_low[ashi].at(i) < minPrice)
                minPrice = td->list_low[ashi].at(i);
            if(td->list_volume[ashi].at(i) > maxVolume)
                maxVolume = td->list_volume[ashi].at(i);
        }
        from = i;
    }
    fromDate = td->list_date[ashi].at(from);
    //------------↑ max and mini

    //----------- calc indicators using Ta-lib and save data to sqlite3

#ifdef TA_LIBC_H
        calcIndicators(td, ashi, from, mapSettings);
#endif
    //this->setBaseRect();
    //高値安値差から４?８本程度のpricescale
    qreal diff = maxPrice - minPrice;
    int e = (int)log10(diff);
    qreal p = ceil(diff / pow(10.0, e) * 10) / 10;
    int interval = 10;
    if(p < 1.5)
        interval = 0.2 * pow(10.0, e);
    else if(p < 5)
        interval = 0.5 * pow(10.0, e);
    else
        interval = 1 * pow(10.0, e);
    priceScaleTop = ceil(maxPrice / interval) * interval;       //max of price area
    priceScaleBottom = floor(minPrice / interval) * interval;   //min
    //qreal r = 100.0 / (priceScaleTop - priceScaleBottom);
    day_size = canvasWidth / (candleCount + 2 + (mapSettings["ichi"] == "1" ? 25 : 0));
    candle_width = day_size * 3 / 5;
    //qDebug() << "max:" << maxPrice << "min:" << minPrice << "from:" << from << fromDate << "to:" << to << toDate << "maxvol:" << maxVolume;
    //qDebug() << "priceScaleTop:" << priceScaleTop << "priceScaleBottom:" << priceScaleBottom << "posPriceTop:" << posPriceTop << "posPriceBottom:" << posPriceBottom;
    if(isDebug)
        qDebug() << "top:" << priceScaleTop << posPriceTop << "bottom:" << priceScaleBottom << posPriceBottom << "width:" << canvasWidth << parentWidth << "height:" << canvasHeight << parentHeight;

    QSqlQuery query=QSqlQuery(td->db);
    if(!td->db.isOpen()) td->db.open();
    int begin_ma[4] = {};
    int begin_bb = 0, begin_pb = 0, begin_macd = 0, begin_rsi = 0, begin_momentum = 0, begin_atr = 0;
    query.exec("select * from talib;");
    while(query.next()){
        QString name = query.value("name").toString();
        if(name == "MA1" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_ma[0] = query.value("begin").toInt();
        else if(name == "MA2" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_ma[1] = query.value("begin").toInt();
        else if(name == "MA3" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_ma[2] = query.value("begin").toInt();
        else if(name == "MA4" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_ma[3] = query.value("begin").toInt();
        else if(name == "MACD" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_macd = query.value("begin").toInt();
        else if(name == "RSI" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_rsi = query.value("begin").toInt();
        else if(name == "BB" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_bb = query.value("begin").toInt();
         else if(name == "PB" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_pb = query.value("begin").toInt();
        else if(name == "MOMENTUM" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_momentum = query.value("begin").toInt();
        else if(name == "ATR" && !query.value("begin").isNull() && query.value("begin").toInt() > 0)
                begin_atr = query.value("begin").toInt();
     }

    double osc_max, osc_min;
   // if(mapSettings["oscillator"] == "MACD" || mapSettings["oscillator"] == "RSI" || mapSettings["oscillator"] == QString::fromLocal8Bit("モメンタム") || mapSettings["oscillator"] == "ATR"){
    if(mapSettings.contains("oscillator") &&  mapSettings["oscillator"] !="none") {
        query.exec(tr("select max(%1), min(%1) from series where ashi = %2 and offset >= %3")
                   .arg(mapSettings["oscillator"] == QString::fromLocal8Bit("モメンタム") ? "MOMENTUM" : mapSettings["oscillator"]).arg(ashi).arg(from));
        query.next();
        osc_max = query.value(0).toDouble();
        osc_min = query.value(1).toDouble();
        if(osc_max < 0)
            osc_max = 0;
        if(osc_min > 0)
            osc_min = 0;
        //qDebug() << "osc max" << osc_max << osc_min;
        if(mapSettings["oscillator"] == "MACD" || mapSettings["oscillator"] == QString::fromLocal8Bit("モメンタム")){
            osc_max *= 1.1;   //maxがvolumeと重なるので10%up
            qreal y0 = getIndiYposition(0, osc_max, osc_min);
            QPen pen;
            pen.setColor(mapSettings["oscBaseColor"]);
            baseRect->childItems().append(this->scene()->addLine(posCanvasLeft + day_size, y0, posCanvasLeft + day_size * (to - from + 1), y0, pen) );
        }
        else if(mapSettings["oscillator"] == "RSI"){
            osc_max = 100;
            osc_min = 0;
            qreal y7 = getIndiYposition(70, 100, 0);
            qreal y3 = getIndiYposition(30, 100, 0);
            QPen pen;
            pen.setColor(mapSettings["oscBaseColor"]);
            baseRect->childItems().append(this->scene()->addLine(posCanvasLeft + day_size, y7, posCanvasLeft + day_size * (to - from + 1), y7, pen) );
            baseRect->childItems().append(this->scene()->addLine(posCanvasLeft + day_size, y3, posCanvasLeft + day_size * (to - from + 1), y3, pen) );
        }
    }


    //------------- price scale line
    int price_scale = priceScaleBottom + interval;
    do{
        baseRect->childItems().append(this->scene()->addLine(posCanvasLeft, getPriceYposition(price_scale), posCanvasRight, getPriceYposition(price_scale), QPen(QColor(mapSettings["scalePenColor"]))) );
        if(price_scale < priceScaleTop){
            QString str;
            str.setNum(price_scale);
            QGraphicsTextItem * io = new QGraphicsTextItem;
            io->setPos(posCanvasRight - 50, getPriceYposition(price_scale));
            io->setPlainText(str);
            io->setDefaultTextColor(QColor(mapSettings["textPenColor"]));
            this->scene()->addItem(io);
            baseRect->childItems().append(io);
        }
        price_scale += interval;
    } while(price_scale <= priceScaleTop);
    //-------------- day scale line
    month_interval = 1;
    if(month < 7)
        month_interval = 1;
    else if(month < 12)
            month_interval = 2;
    else if(month < 24)
        month_interval = 3;
    else if(month > 48)
        month_interval = 12;
    else if(month > 60)
        month_interval = 24;
    else
        month_interval = 6;
//---------------------------------

    double t_prev = 0;
    double k_prev = 0;
    double c_prev = 0;
    double s1_prev = 0;
    double s2_prev = 0;
/*
    if(isIchimoku){
        for(int i = from - 25 ; i < from ; i++){
            double t = 0;
            double k = 0;
            double s1 = 0;
            double s2 = 0;
            //double c = 0;
            if(i >= 51){
                query.exec(tr("select max(high) as h, min(low) as l from series where offset > %1 and offset < %2").arg(i - 9).arg(i + 1));
                query.first();
                t = (query.value(0).toDouble() + query.value(1).toDouble()) / 2;
                query.exec(tr("select max(high) as h, min(low) as l from series where offset > %1 and offset < %2").arg(i - 26).arg(i + 1));
                query.first();
                k = (query.value(0).toDouble() + query.value(1).toDouble()) / 2;
                s1 = (t + k) / 2;
                query.exec(tr("select max(high) as h, min(low) as l from series where offset > %1 and offset < %2").arg(i - 51).arg(i + 1));
                query.first();
                s2 = (query.value(0).toDouble() + query.value(1).toDouble()) / 2;
                if(!query.exec(tr("update series set t = %1, k = %2 where ashi = %3 and offset = %4").arg(t).arg(k).arg(ashi).arg(i)) )
                    qDebug() << "sql update error ichi " << i;
                if(!query.exec(tr("update series set s1 = %1, s2 = %2 where ashi = %3 and offset = %4").arg(s1).arg(s2).arg(ashi).arg(i + 25)) )
                    qDebug() << "sql update error ichi " << i;
                if(s1 > 0 && s2 > 0 && s1_prev > 0 && s2_prev > 0){
                    qreal x = posCanvasLeft + day_size * (i + 25);
                    QPen pen;
                    pen.setColor(QColor(mapSettings["ichiSaColor"]));
                    this->scene()->addLine(x - day_size, getPriceYposition(s1_prev), x, getPriceYposition(s1), pen);
                    pen.setColor(QColor(mapSettings["ichiSbColor"]));
                    this->scene()->addLine(x - day_size, getPriceYposition(s2_prev), x, getPriceYposition(s2), pen);
                    pen.setStyle(Qt::DashLine);
                    this->scene()->addLine(x, getPriceYposition(s1), x, getPriceYposition(s2), pen);
                }
                s1_prev = s1;
                s2_prev = s2;
            }
        }
        if(isDebug)
            qDebug() << "calced ichi";
    }
*/
    if(isDebug)
        qDebug() << "start candle";

    //isDebug = true;
    double pre_heikin;
    if(mapSettings["heikin"] == "1"){
        if(from > 1)
            pre_heikin = (td->list_open[ashi].at(from - 2) + td->list_close[ashi].at(from - 2)) / 4
                + (td->list_open[ashi].at(from - 1) + td->list_high[ashi].at(from - 1) + td->list_low[ashi].at(from - 1) + td->list_close[ashi].at(from - 1)) / 8;
        else if(from > 0)
            pre_heikin = (td->list_open[ashi].at(from - 1) + td->list_close[ashi].at(from - 1)) / 2;
        else
            pre_heikin = (td->list_open[ashi].at(from) + td->list_high[ashi].at(from) + td->list_low[ashi].at(from) + td->list_close[ashi].at(from)) / 4;
    }





    double prev_ma[4] = {};
    double prev_bb_m = 0, prev_bb = 0;
    double prev_macd = 0, prev_macd_signal = 0, prev_rsi = 0, prev_momentum = 0, prev_atr = 0;

    QSqlQuery query2=QSqlQuery(td->db);
    if(!td->db.isOpen()) td->db.open();
    query2.exec(tr("select * from series where ashi = %1 and offset >= %2 order by offset").arg(ashi).arg(from));
    for(int i = from; i <= to ; i++ ){
        query2.next();
        qreal x = posCanvasLeft + day_size * (i - from + 1);
        if(i > 0)
        drawScaleDate(td->list_date[ashi].at(i), td->list_date[ashi].at(i - 1), i, x);    //--------日付スケール

        //-------ma
        if(mapSettings["ma1"].toInt() > 0 || mapSettings["ma2"].toInt() > 0 || mapSettings["ma3"].toInt() > 0 || mapSettings["ma4"].toInt() > 0){
            double ma[4] = {};
            for(int j = 0 ; j < 4 ; j++)
                ma[j] = query2.value(tr("MA%1").arg(j + 1)).toDouble();
            for(int j = 3; j >= 0; j--){
                if(mapSettings[tr("ma%1").arg(j + 1)].toInt() > 0
                        && prev_ma[j] >= priceScaleBottom && prev_ma[j] <= priceScaleTop
                        && ma[j] >= priceScaleBottom && ma[j] <= priceScaleTop
                        && i > from && i - 1 >= begin_ma[j]){
                    qreal pre = getPriceYposition(prev_ma[j]);
                    qreal y1 = getPriceYposition(ma[j]);
                    QPen mapen;
                    mapen.setColor(mapSettings[tr("maColor%1").arg(j + 1)]);
                    mapen.setWidth(mapSettings[tr("maWidth%1").arg(j + 1)].toInt());
                    if(i > begin_ma[j])
                    baseRect->childItems().append(this->scene()->addLine(x - day_size, pre, x, y1, mapen) );
                }
                prev_ma[j] = ma[j];
            }

        }
       //-------↑ ma
        if(mapSettings["bb"] == "1") {
            double bb_m = query2.value("bb_m").toDouble();
            double bb = query2.value("bb").toDouble();

            if(prev_bb_m >0 && prev_bb >0 && bb_m >0 && bb >0){
                qreal y0 = getPriceYposition(bb_m);
                qreal sd = getPriceYposition(bb_m + bb) - y0;
                qreal pre0 = getPriceYposition(prev_bb_m);
                qreal sd_pre = getPriceYposition(prev_bb_m + prev_bb) - pre0;
                QPen penBb;
                penBb.setColor(QColor(mapSettings["bb0Color"]));
                if(prev_bb_m >= priceScaleBottom && bb_m >= priceScaleBottom && prev_bb_m <= priceScaleTop && bb_m <= priceScaleTop)
                    baseRect->childItems().append(this->scene()->addLine(x - day_size, pre0, x, y0, penBb) );
                penBb.setColor(QColor(mapSettings["bb1Color"]));
                if(prev_bb_m + bb >= priceScaleBottom && bb_m + bb >= priceScaleBottom
                        && prev_bb_m + bb <= priceScaleTop && bb_m + bb <= priceScaleTop)
                    baseRect->childItems().append(this->scene()->addLine(x - day_size, pre0 + sd_pre, x, y0 + sd, penBb) );
                if(prev_bb_m - bb >= priceScaleBottom && bb_m - bb >= priceScaleBottom
                        && prev_bb_m - bb <= priceScaleTop && bb_m - bb <= priceScaleTop)
                    baseRect->childItems().append(this->scene()->addLine(x - day_size, pre0 - sd_pre, x, y0 - sd, penBb) );
                penBb.setColor(QColor(mapSettings["bb2Color"]));
                if(prev_bb_m + bb * 2 >= priceScaleBottom && bb_m + bb * 2 >= priceScaleBottom
                        && prev_bb_m + bb * 2 <= priceScaleTop && bb_m + bb * 2 <= priceScaleTop)
                    baseRect->childItems().append(this->scene()->addLine(x - day_size, pre0 + sd_pre * 2, x, y0 + sd * 2, penBb) );
                if(prev_bb_m - bb * 2 >= priceScaleBottom && bb_m - bb * 2 >= priceScaleBottom
                        && prev_bb_m - bb * 2 <= priceScaleTop && bb_m - bb * 2 <= priceScaleTop)
                    baseRect->childItems().append(this->scene()->addLine(x - day_size, pre0 - sd_pre * 2, x, y0 - sd * 2, penBb) );
                penBb.setColor(QColor(mapSettings["bb3Color"]));
                if(prev_bb_m + bb * 3 >= priceScaleBottom && bb_m + bb * 3 >= priceScaleBottom
                        && prev_bb_m + bb * 3 <= priceScaleTop && bb_m + bb * 3 <= priceScaleTop)
                    baseRect->childItems().append(this->scene()->addLine(x - day_size, pre0 + sd_pre * 3, x, y0 + sd * 3, penBb) );
                if(prev_bb_m - bb * 3 >= priceScaleBottom && bb_m - bb * 3 >= priceScaleBottom
                        && prev_bb_m - bb * 3 <= priceScaleTop && bb_m - bb * 3 <= priceScaleTop)
                    baseRect->childItems().append(this->scene()->addLine(x - day_size, pre0 - sd_pre * 3, x, y0 - sd * 3, penBb) );
            }
            prev_bb_m = bb_m;
            prev_bb = bb;
        }
        //-----↑ bb
        if(mapSettings["pb"] == "1"){
            double sar = query2.value("pb").toDouble();
            if(sar >= priceScaleBottom && sar <= priceScaleTop){
                qreal y = getPriceYposition(sar);
                QPen pen;
                if(sar > td->list_close[ashi].at(i)){
                    baseRect->childItems().append(this->scene()->addEllipse(x, y, 3, 3, QPen(QColor(mapSettings["pbNegColor"])), QBrush(QColor(mapSettings["pbNegColor"]))) );
                }
                else{
                    baseRect->childItems().append(this->scene()->addEllipse(x, y, 3, 3, QPen(QColor(mapSettings["pbPosColor"])), QBrush(QColor(mapSettings["pbPosColor"]))) );
                }
            }
        }
        if(mapSettings["ichi"] == "1"){
            double t = 0;
            double k = 0;
            double c = 0;
            double s1 = 0;
            double s2 = 0;
            if(i >= 9){
                query.exec(tr("select max(high) as h, min(low) as l from series where offset > %1 and offset < %2").arg(i - 9).arg(i + 1));
                query.first();
                t = (query.value(0).toDouble() + query.value(1).toDouble()) / 2;
            }
            if(i >= 26){
                query.exec(tr("select max(high) as h, min(low) as l from series where offset > %1 and offset < %2").arg(i - 26).arg(i + 1));
                query.first();
                k = (query.value(0).toDouble() + query.value(1).toDouble()) / 2;
            }
            if(i + 25 <= to){
                c = td->list_close[ashi].at(i + 25);
            }
            if(t > 0 && t_prev > 0){
                QPen pen;
                pen.setColor(QColor(mapSettings["ichiTColor"]));
                pen.setWidth(2);
                baseRect->childItems().append(this->scene()->addLine(x - day_size, getPriceYposition(t_prev), x, getPriceYposition(t), pen) );
            }
            if(k > 0 && k_prev > 0){
                QPen pen;
                pen.setColor(QColor(mapSettings["ichiKColor"]));
                pen.setWidth(2);
                baseRect->childItems().append(this->scene()->addLine(x - day_size, getPriceYposition(k_prev), x, getPriceYposition(k), pen) );
            }
            if(c > 0 && c_prev > 0){
                QPen pen;
                pen.setColor(QColor(mapSettings["ichiCColor"]));
                pen.setWidth(1);
                baseRect->childItems().append(this->scene()->addLine(x - day_size, getPriceYposition(c_prev), x, getPriceYposition(c), pen) );
            }
            if(i >= 52){
                s1 = (t + k) / 2;
                query.exec(tr("select max(high) as h, min(low) as l from series where offset > %1 and offset < %2").arg(i - 52).arg(i + 1));
                query.first();
                s2 = (query.value(0).toDouble() + query.value(1).toDouble()) / 2;

            }
            if(!query.exec(tr("update series set t = %1, k = %2, c = %3 where ashi = %4 and offset = %5").arg(t).arg(k).arg(c).arg(ashi).arg(i)) )
                qDebug() << "sql update error ichi " << i;
            if(i + 25 > to)
                query.exec(tr("replace into series (offset, ashi, s1, s2) values (%1, %2, %3, %4)").arg(i + 25).arg(ashi).arg(s1).arg(s2));
            else{
            if(!query.exec(tr("update series set s1 = %1, s2 = %2 where ashi = %3 and offset = %4").arg(s1).arg(s2).arg(ashi).arg(i + 25)) )
                qDebug() << "sql update error ichi " << i;
            }
            if(s1 > 0 && s2 > 0 && s1_prev > 0 && s2_prev > 0){
                qreal x2 = posCanvasLeft + day_size * (i - from + 1 + 25);
                QPen pen1;

                pen1.setColor(QColor(mapSettings["ichiSaColor"]));
                //pen1.setColor(QColor(Qt::transparent));
                baseRect->childItems().append(this->scene()->addLine(x2 - day_size, getPriceYposition(s1_prev), x2, getPriceYposition(s1), pen1) );
                pen1.setColor(QColor(mapSettings["ichiSbColor"]));
                baseRect->childItems().append(this->scene()->addLine(x2 - day_size, getPriceYposition(s2_prev), x2, getPriceYposition(s2), pen1) );
                if(s1 > s2)
                    pen1.setColor(QColor(mapSettings["ichiSbColor"]));
                else
                    pen1.setColor(QColor(mapSettings["ichiSaColor"]));
                pen1.setStyle(Qt::DashLine);
                baseRect->childItems().append(this->scene()->addLine(x2, getPriceYposition(s1), x2, getPriceYposition(s2), pen1) );
            }

            t_prev = t;
            k_prev = k;
            c_prev = c;
            s1_prev = s1;
            s2_prev = s2;
        }
        if( mapSettings["line"] == "1"){
            if(i > 0){
                QPen pen;
                pen.setColor(QColor(mapSettings["lineColor"]));
                pen.setWidth(3);
                baseRect->childItems().append(this->scene()->addLine(x - day_size, getPriceYposition(td->list_close[ashi][i - 1]), x, getPriceYposition(td->list_close[ashi][i]), pen) );
            }
        }
        else{
            qreal h = getPriceYposition(td->list_high[ashi].at(i));
            qreal l = getPriceYposition(td->list_low[ashi].at(i));
            if(mapSettings["heikin"] == "1"){
                if(i > 0){
                    //qreal ho = (td->list_open[ashi].at(i - 1) + td->list_close[ashi].at(i - 1)) / 2;
                    qreal ho = pre_heikin;
                    qreal hc = (td->list_open[ashi].at(i) + td->list_high[ashi].at(i)
                               + td->list_low[ashi].at(i) + td->list_close[ashi].at(i)) / 4;
                    bool fUp = (hc >= ho) ? true : false;
                    qreal b1 = getPriceYposition(fUp ? hc : ho);
                    qreal b2 = getPriceYposition(fUp ? ho : hc);
                    QBrush brush(fUp ? QColor(mapSettings["yosenColor"]) : QColor(mapSettings["insenColor"]));
                    QPen pen(fUp ? QColor(mapSettings["yosenPenColor"]) : QColor(mapSettings["insenPenColor"]));
                    baseRect->childItems().append(this->scene()->addRect(x - candle_width / 2, b1, candle_width, b2 - b1, pen, brush) );
                    pen.setColor(fUp ? QColor(mapSettings["yosenColor"]) : QColor(mapSettings["insenColor"]));
                    baseRect->childItems().append(this->scene()->addLine( x, h, x, b1 - 1, pen) );
                    baseRect->childItems().append(this->scene()->addLine( x, l, x, b2 + 1, pen) );
                    pre_heikin = (ho + hc) / 2;
                }
            }
            else{
                bool fUp = td->list_close[ashi].at(i) >= td->list_open[ashi].at(i) ? true : false;
                qreal b1 = getPriceYposition(fUp ? td->list_close[ashi].at(i) : td->list_open[ashi].at(i));
                qreal b2 = getPriceYposition(fUp ? td->list_open[ashi].at(i) : td->list_close[ashi].at(i));
                if(b1 == b2)
                    b1 += 1;
                QBrush brush(fUp ? QColor(mapSettings["yosenColor"]) : QColor(mapSettings["insenColor"]));
                QPen pen(fUp ? QColor(mapSettings["yosenPenColor"]) : QColor(mapSettings["insenPenColor"]));
                baseRect->childItems().append(this->scene()->addRect(x - candle_width / 2, b1, candle_width, b2 - b1, pen, brush) );
                pen.setColor(fUp ? QColor(mapSettings["yosenColor"]) : QColor(mapSettings["insenColor"]));
                //pen;
                baseRect->childItems().append(this->scene()->addLine( x, h, x, b1 - 1, pen) );
                baseRect->childItems().append(this->scene()->addLine( x, l, x, b2 + 1, pen) );
            }
        }
        if(isVolume && td->info.code > 1300){
            qreal vol = getVolumeYposition(td->list_volume[ashi].at(i), maxVolume);
            if(vol < 0)
                vol = 0;    //三井住友建設10桁
            baseRect->childItems().append(this->scene()->addRect(x - candle_width / 2, vol, candle_width, posVolumeBottom - vol, QPen(QColor(mapSettings["volumeColor"])), QBrush(QColor(mapSettings["volumeColor"]))) );
        }
        if(mapSettings["oscillator"] == "MACD"){
            //qDebug() << i << begin_macd << query2.value("MACD").toDouble() << query2.value("MACD_sig").toDouble();
            if(i >= begin_macd){
                double value = query2.value("MACD").toDouble();
                double signal = query2.value("MACD_sig").toDouble();
                if(i > from && i > begin_macd){
                    qreal val = getIndiYposition(value, osc_max, osc_min);
                    qreal sig = getIndiYposition(signal, osc_max, osc_min);
                    qreal val0 = getIndiYposition(prev_macd, osc_max, osc_min);
                    qreal sig0 = getIndiYposition(prev_macd_signal, osc_max, osc_min);
                    QPen pen;
                    pen.setColor(QColor(mapSettings["macdColor"]));
                    if(value >= osc_min && value <= osc_max && prev_macd >= osc_min && prev_macd <=osc_max)
                        baseRect->childItems().append(this->scene()->addLine(x - day_size, val0, x, val, pen) );
                    pen.setColor(QColor(mapSettings["macdSigColor"]));
                    if(signal >= osc_min && signal <= osc_max && prev_macd_signal >= osc_min && prev_macd_signal <=osc_max)
                        baseRect->childItems().append(this->scene()->addLine(x - day_size, sig0, x, sig, pen) );
                }
                prev_macd = value;
                prev_macd_signal = signal;
            }
        }
        else{
            if( mapSettings.contains("oscillator") &&  mapSettings["oscillator"] != "none") {
                QString name = mapSettings["oscillator"] == QString::fromLocal8Bit("モメンタム") ? "MOMENTUM" : mapSettings["oscillator"];
                int begin = 0;
                double pr;
                if(mapSettings["oscillator"] == QString::fromLocal8Bit("モメンタム")){
                    begin = begin_momentum;
                    pr = prev_momentum;
                }
                else if(mapSettings["oscillator"] == "RSI"){
                    begin = begin_rsi;
                    pr = prev_rsi;
                }
                else if(mapSettings["oscillator"] == "ATR"){
                    begin = begin_atr;
                    pr = prev_atr;
                }
                if(i >= begin){
                    double value = query2.value(name).toDouble();
                    qreal prev = getIndiYposition(pr, osc_max, osc_min);
                    if(i > from && i > begin){
                        qreal val = getIndiYposition(value, osc_max, osc_min);
                        QPen pen;
                        pen.setColor(QColor(mapSettings["rsiColor"]));
                        baseRect->childItems().append(this->scene()->addLine(x - day_size, prev, x, val, pen) );
                    }
                    if(mapSettings["oscillator"] == QString::fromLocal8Bit("モメンタム"))
                        prev_momentum = value;
                    else if(mapSettings["oscillator"] == "RSI")
                        prev_rsi = value;
                    else if(mapSettings["oscillator"] == "ATR")
                        prev_atr = value;
                }
            }
        }
    }
    if(isDebug)
        qDebug() << "candle end";

    crossLine = this->scene()->addLine(posCanvasLeft + day_size * (to - from + 1), posPriceTop
            , posCanvasLeft + day_size * (to - from + 1), posIndicatorBottom - 12, QPen(QColor(mapSettings["selectColor"])) );
    crossLine->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
    crossLine->setVisible(isCross);

    if(mapSettings["showValue"].toInt() == 1){
    QString info = tr("%1 %2 %3").arg(td->info.code).arg(td->info.name).arg(td->info.market);
    QGraphicsSimpleTextItem * infoitem = this->scene()->addSimpleText(info);
    infoitem->setBrush(QColor(mapSettings["textPenColor"]));


    infoPrice = new QGraphicsTextItem;
    infoPrice->setPos(30,80);
    infoPrice->setDefaultTextColor(QColor(mapSettings["textPenColor"]));
    infoPrice->setFlag(QGraphicsItem::ItemIsMovable);
    updatePriceInfo(0);
    this->scene()->addItem(infoPrice);
    baseRect->childItems().append(infoPrice);
    }
}

void MyGraphicsView::updatePriceInfo(int offset){
    QStringList str_price;
    str_price << td->list_date[ashi].at(to - offset).toString("yyyy-M-d ddd");
    str_price << tr("O: %L1").arg(td->list_open[ashi].at(to - offset));
    str_price << tr("H: %L1").arg(td->list_high[ashi].at(to - offset));
    str_price << tr("L: %L1").arg(td->list_low[ashi].at(to - offset));
    if(to - offset - 1 >= 0)
        str_price << tr("C: %L1 (%2%L3)").arg(td->list_close[ashi].at(to - offset))
                 .arg(td->list_close[ashi].at(to - offset) - td->list_close[ashi].at(to - offset - 1) > 0 ? "+" : "")
                 .arg(td->list_close[ashi].at(to - offset) - td->list_close[ashi].at(to - offset - 1) );
    else
        str_price << tr("C: %L1").arg(td->list_close[ashi].at(to - offset));
    if(isVolume && td->info.code > 1300)
        str_price << tr("V: %L1").arg(td->list_volume[ashi].at(to - offset));
    QSqlQuery query=QSqlQuery(td->db);
    query.exec(tr("select * from series where ashi = %1 and offset > %2 and offset < %3 order by date")
               .arg(ashi).arg(to - offset - 2).arg(to - offset + 1));
    query.first();
    double ma[4] = {};
    double ma_pre[4] = {};
    double macd[2] = {}, macd_sig[2] = {};
    double rsi[2] = {};

    for(int i = 0; i < 4 ;i++)
        ma_pre[i] = query.value(tr("ma%1").arg(i + 1)).toDouble();
    macd[0] = query.value("macd").toDouble();
    macd_sig[0] = query.value("macd_sig").toDouble();
    rsi[0] = query.value("rsi").toDouble();
    query.next();
    double close = query.value("close").toDouble();
    for(int i = 0; i < 4 ;i++){
        ma[i] = query.value(tr("ma%1").arg(i + 1)).toDouble();
        int map = mapSettings[tr("ma%1").arg(i + 1)].toInt();
        double pos = close - ma[i];
        double dir = ma[i] - ma_pre[i];
        if(map > 0 && ma[i] > 0 && pos < 100000 && pos > -100000)
            str_price << tr("%1MA: %2%L3(%L4 %5)").arg(map).arg(pos > 0 ? "+" : "").arg(pos, 2, 'f', 1).arg(ma[i], 2, 'f', 1).arg(dir > 0 ? "+" : (dir < 0 ? "-" : ""));
    }
    macd[1] = query.value("macd").toDouble();
    macd_sig[1] = query.value("macd_sig").toDouble();
    rsi[1] = query.value("rsi").toDouble();
    //bool isBB = mapSettings["bb"] == "1" ? true : false;
    if(mapSettings["bb"] == "1"){
        double bb_m = query.value("bb_m").toDouble();
        double bb = query.value("bb").toDouble();
        double sd = (close - bb_m) / bb;
        str_price << tr("BB(20):%L1(sd:%2").arg(bb_m, 2, 'f', 1).arg(sd, 2, 'f', 1);
    }
    if(mapSettings["oscillator"] == "MACD"){
        if(macd[1] > -1000 && macd[0] > -1000 && macd_sig[1] > -1000 && macd_sig[0] > -1000
                && macd[1] < 1000 && macd[0] < 1000 && macd_sig[1] < 1000 && macd_sig[0] < 1000)
        str_price << tr("MACD:%1%2 %3(macd-sig: %4%5 %6)").arg(macd[1] > 0 ? "+" : "").arg(macd[1], 2, 'f', 1)
                .arg(macd[1] > macd[0] ? "+" : (macd[1] < macd[0] ? "-" : ""))
                .arg(macd[1] > macd_sig[1] ? "+" : "").arg(macd[1] - macd_sig[1], 2, 'f', 1)
                .arg(macd_sig[1] > macd_sig[0] ? "+" : (macd_sig[1] < macd_sig[0] ? "-" : ""));
    }
    else if(mapSettings["oscillator"] == "RSI"){
        str_price << tr("RSI:%1(%2%3)").arg(rsi[1], 2, 'f', 1).arg(rsi[1] > rsi[0] ? "+" : "").arg(rsi[1] - rsi[0], 2, 'f', 1);
    }
    else if(mapSettings["oscillator"] == "ATR"){
        str_price << tr("ATR: %1").arg(query.value("ATR").toDouble(), 5, 'f', 1);
    }
    else if(mapSettings["oscillator"] == QString::fromLocal8Bit("モメンタム")){
        str_price << QString::fromLocal8Bit("モメンタム: %1").arg(query.value("MOMENTUM").toDouble(), 5, 'f', 1);;
    }
    if(mapSettings["ichi"] == "1"){
        query.last();
        double t = query.value("t").toDouble();
        double k = query.value("k").toDouble();
        //double c = query.value("c").toDouble();
        double s1 = query.value("s1").toDouble();
        double s2 = query.value("s2").toDouble();
        str_price << QString::fromLocal8Bit("転換線:%1(%2%3)").arg(t).arg(close > t ? "+" : "").arg(close - t);
        str_price << QString::fromLocal8Bit("基準線:%1(%2%3)").arg(k).arg(close > k ? "+" : "").arg(close - k);
        str_price << QString::fromLocal8Bit("先行スパンA:%1(%2%3)").arg(s1).arg(close > s1 ? "+" : "").arg(close - s1);
        str_price << QString::fromLocal8Bit("先行スパンB:%1(%2%3)").arg(s2).arg(close > s2 ? "+" : "").arg(close - s2);
    }
    //bool isPb = mapSettings["pb"] == "1" ? true : false;
//    bool isHeikin = mapSettings["heikin"] == "1" ? true : false;
    infoPrice->setPlainText(str_price.join("\n"));
}


void MyGraphicsView::setSetting(QString name, QString value){
    if(name == "ma" && value == "0"){
        mapSettings.insert("ma1", "0");
        mapSettings.insert("ma2", "0");
        mapSettings.insert("ma3", "0");
        mapSettings.insert("ma4", "0");
        return;
    }
    mapSettings.insert(name, value);
    if(name == "oscillator")
        isOscillator = (value == "none") ? false : true;
    //qDebug() << "setsettings:" << name << value;
    if(name == "month")
        month = value.toInt();
    else if(name == "volume")
        isVolume = value == "true" ? true : false;
    else if(name == "vol100")
        isVol100 = (value == "true") ? true : false;
    else if(name == "macd" || name == "rsi")
        isOscillator =  (mapSettings["macd"] == "1" || mapSettings["rsi"] == "1") ? true : false;
    //qDebug() << "setting change:" << isOscillator;
}
void MyGraphicsView::setParentSize(int width, int height){
    this->parentWidth = width;
    this->parentHeight = height;
}


void MyGraphicsView::setTradedata(tradeData* tradedata){
    td = tradedata;
}
void MyGraphicsView::setDrawLine(){
    v_scene->setModeDraw();
}void MyGraphicsView::clearDraw(){
    v_scene->clearDraw();
}

void MyGraphicsView::mouseMoveEvent(QMouseEvent *event){
    if(isCross){
        crossLine->setVisible(isCross);
        int offset = (- crossLine->x() ) / day_size;
        //offsetは最終日を基準として何日前かを表す
        //if(to - offset > 0 && offset >= 0){
        if(offset <= to - from && offset >= 0){
            //qDebug() << event->x() << "crossX:" << crossLine->x() << offset << td->list_date[ashi].at(to - offset);
            updatePriceInfo(offset);
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}
void MyGraphicsView::keyPressEvent(QKeyEvent *event){
    //qDebug() << "key press in MyGraphicsview" << event->key();
    //QTreeWidget *tree = qobject_cast<QTreeWidget*>(this->parent()->findChild<QWidget *>("brandTree"));
    if(event->key() == 16777237){   //↓
        QTreeWidget *tree = this->parent()->findChild<QTreeWidget *>("brandTree");
        tree->setFocus();
        QTreeWidgetItem* item = tree->currentItem();
        if(tree->itemBelow(item) != NULL)
        tree->setCurrentItem(tree->itemBelow(item));
    }
    else if(event->key() == 16777235){  //↑
        QTreeWidget *tree = this->parent()->findChild<QTreeWidget *>("brandTree");
        tree->setFocus();
        QTreeWidgetItem* item = tree->currentItem();
        if(tree->itemAbove(item) != NULL)
        tree->setCurrentItem(tree->itemAbove(item));
    }
    else if (event->key() == 16777236){ //→

    }
    else if (event->key() == 16777234){ // ←

    }
    else if(event->key() > 47 && event->key() < 58){
        QLineEdit *tb = this->parent()->findChild<QLineEdit *>("tbCode");

        int code = tb->text().toInt();
        bool f = false;
        if(td->info.code == code){
            if(code < 1000){
                code *= 10;
                code += event->text().toInt();
                if(td->load(code) == 0){
                    f=true;
                }
            }
            if(!f)
                tb->clear();
        }
        tb->setText(tb->text().append(tr("%1").arg(event->text())));

    }
    else if(event->key() == 16777219) //backspace
    {
        QLineEdit *tb = this->parent()->findChild<QLineEdit *>("tbCode");
        tb->setText(tb->text().remove(tb->text().length() - 1, 1));
        /*
        for(int i = 0 ; i < this->parent()->children().count() ; i++){
            if(this->parent()->children().at(i)->objectName() == "mainToolBar"){
                for(int j = 0 ; j < this->parent()->children().at(i)->children().count() ; j++){
                    if(this->parent()->children().at(i)->children().at(j)->objectName() == "tbCode"){
                        qDebug() << this->parent()->children().at(i)->children().at(j)->objectName();;
                        QLineEdit *tb = qobject_cast<QLineEdit*>(this->parent()->children().at(i)->children().at(j));
                        tb->setText(tb->text().remove(tb->text().length() - 1, 1));
                    }
                }
            }
        }
        */
    }
    //77 m 73 i b 66 l 76 p 80 h 72 1:49 2:50 9:57 0:48 -:45
    QGraphicsView::keyPressEvent(event);
}

void MyGraphicsView::setCross(bool flag){
    isCross = flag;
    crossLine->setVisible(flag);
}

void MyGraphicsView::setDebug(bool flag){
    isDebug = flag;
}
