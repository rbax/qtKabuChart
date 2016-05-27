/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 *
 * This source code is a modification of article website below.
 * http://www.walletfox.com/course/qgraphicsitemruntimedrawing.php
 * Cppyright (c) Walletfox.com, 2014
 * *******************************************************************/

#include "mygraphicsscene.h"
#include <QDebug>

/*
 *refer to
 * How to draw QGraphicsLineItem during run time with mouse coordinates
 * http://www.walletfox.com/course/qgraphicsitemruntimedrawing.php
 */

MyGraphicsScene::MyGraphicsScene(QObject* parent)
{
    sceneMode = NoMode;
    itemToDraw = 0;

}

void MyGraphicsScene::setLineColor(QString colorname){
    lineColor = colorname;
}

void MyGraphicsScene::clearDraw(){
    //lineを消せない
    //itemToDraw->~QGraphicsLineItem();

    if(freeLines.count() > 0){
        for(int i = 0; i < freeLines.count() ; i++){
            //freeLines.at(i)->setVisible(false);
            QGraphicsLineItem *item = freeLines.at(i);
            freeLines.removeAt(i);  //これ必要
            delete item;
            //freeLines.at(i)->~QGraphicsLineItem();
        }
    }

}

void MyGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event){
    if(sceneMode == DrawLine)
        origPoint = event->scenePos();
    QGraphicsScene::mousePressEvent(event);
}

void MyGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event){
    if(sceneMode == DrawLine){
        if(!itemToDraw && origPoint.x() > 0 && origPoint.y() > 0){
            itemToDraw = new QGraphicsLineItem;
            this->addItem(itemToDraw);
            freeLines.append(itemToDraw);
            QPen pen;
            pen.setColor(lineColor);
            pen.setWidth(3);
            pen.setStyle(Qt::SolidLine);
            itemToDraw->setPen(pen);
            itemToDraw->setPos(origPoint);
        }
        if(origPoint.x() > 0 && origPoint.y() > 0){
            itemToDraw->setLine(0,0,event->scenePos().x() - origPoint.x(),event->scenePos().y() - origPoint.y());
        }
    }
    else
        QGraphicsScene::mouseMoveEvent(event);
}


void MyGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
    itemToDraw = 0;
    sceneMode = NoMode;
    QGraphicsScene::mouseReleaseEvent(event);
}

void MyGraphicsScene::setMode(Mode mode){
    sceneMode = mode;
    itemToDraw = 0;

}
void MyGraphicsScene::setModeDraw(){
    sceneMode = DrawLine;
    origPoint.setX(-1);
    origPoint.setY(-1);
    itemToDraw = 0;
}
