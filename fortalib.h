/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#if __has_include("ta_libc.h")

#include "ta_libc.h"

#ifndef FORTALIB_H
#define FORTALIB_H

#include "tradedata.h"

extern bool calcIndicators(tradeData *td, int ashi, int from, QMap<QString, QString> setting);

#endif // FORTALIB_H

#endif
