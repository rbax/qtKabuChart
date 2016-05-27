/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 *
 * This source code is a modification of article website below.
 * http://www.walletfox.com/course/qgraphicsitemruntimedrawing.php
 * Cppyright (c) Walletfox.com, 2014
 * *******************************************************************/

#ifndef MYGRAPHICSSCENE_H
#define MYGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>

class MyGraphicsScene : public QGraphicsScene
{

public:
    enum Mode {NoMode, SelectObject, DrawLine};
    MyGraphicsScene(QObject* parent = 0);
    void setMode(Mode mode);
    void setModeDraw();
    void clearDraw();
    void setLineColor(QString colorname);
    void setDebug(bool flag);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
    Mode sceneMode;
    QPointF origPoint;
    QGraphicsLineItem *itemToDraw;
    QList<QGraphicsLineItem*> freeLines;

    QString lineColor;

    bool isDebug;
};


#endif // MYGRAPHICSSCENE_H
