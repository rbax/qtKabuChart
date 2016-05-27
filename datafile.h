/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#ifndef DATAFILE_H
#define DATAFILE_H

#include "tradedata.h"

extern QString getSource(QString url);
extern QList< QStringList > readSjCsv(QString fn);
extern quint32 getLastDate(int code, QString directory, bool isOmega);
extern QList<quint32> downloadBrand(tradeData *td, int code, QString directory, bool isOmega, bool fAll = false);
extern int downloadDay(tradeData *td, bool isUpdate, QString directory, bool isOmega, quint32 targetdate = 0);
extern int downloadStocks(quint32 idate, QString directory, bool isOmega);

#endif // DATAFILE_H
