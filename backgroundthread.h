/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#ifndef BACKGROUNDTHREAD_H
#define BACKGROUNDTHREAD_H

#include "tradedata.h"
#include <QThread>

class backgroundThread: public QThread {
     Q_OBJECT
public:
    backgroundThread();
    ~backgroundThread();

    void set(tradeData *tradedata, QString directory, bool isomega, bool isupdate = true);
    void setMaxdownload(int countDays);

protected:
    void run();
signals:
    void finishThread();
    void sendMessage(QString message);

private:
    bool isOmega;
    QString dataDir;
    tradeData *td;
    bool isUpdate;
    int maxDownload;
};

#endif // BACKGROUNDTHREAD_H
