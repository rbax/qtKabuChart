/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include "mygraphicsscene.h"
#include "tradedata.h"
#include <QtGui>
#include <QGraphicsView>

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    MyGraphicsView(QWidget *parent);
    ~MyGraphicsView();

    void setTradedata(tradeData* tradedata);
    void setParentSize(int width, int height);
    void showBrand(int code, int candleashi, int monthFrom);
    void updateChart();

    void updateChart(QString name, int value);
    void updateChart(QString name, QString value);
    void updateChart(int candleashi);
    void setMonth(int monthFrom, bool update = false);

    void setSetting(QString name, QString value);
    void setDockWidth(qreal value);

    void setCross(bool flag);
    void setDebug(bool flag);
    void setDrawLine();
    void clearDraw();

    QStringList getKeysList();
    QString getLabel(QString field);

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent * event);
    void resizeEvent(QResizeEvent* event);

private:
    MyGraphicsScene *v_scene;

    QGraphicsLineItem *crossLine;
    QGraphicsTextItem *infoPrice;
    void updatePriceInfo(int offset);

    void settingsLoad();
    void settingsSave();
    tradeData *td;

    qreal getPriceYposition(qreal price);
    qreal getVolumeYposition(qreal volume, qreal max_volume);
    qreal getIndiYposition(qreal value, qreal max, qreal min);
    void drawScaleDate(QDate d, QDate prev, int i, int x);
    qreal parentWidth;
    qreal parentHeight;
    qreal canvasWidth;
    qreal canvasHeight;
    qreal dockWidth;

    int month_interval;
    int candleCount;
    int from;
    int to;
    QDate fromDate;
    QDate toDate;
    qreal day_size;
    qreal candle_width;
    qreal maxPrice;
    qreal minPrice;
    qreal maxVolume;
    qreal priceScaleTop;
    qreal priceScaleBottom;
    qreal posPriceTop;
    qreal posPriceBottom;
    qreal posVolumeBottom;
    qreal posIndicatorBottom;
    qreal posCanvasLeft;
    qreal posCanvasRight;

    int xFirst;
    int xLast;

    int month;
    int ashi;

    bool isOscillator;
    bool isCross;
    bool isVolume;
    bool isVol100; //出来高 1/100

    bool isDebug;

    QMap<QString, QString> mapSettings;

    QGraphicsRectItem *baseRect;

    QStringList sListKeys;
    QStringList sListLabel;
    QStringList sListDefaultValues;

};


#endif // MYGRAPHICSVIEW_H
