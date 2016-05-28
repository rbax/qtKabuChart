#-------------------------------------------------
#
# Project created by QtCreator 2016-05-13T07:29:30
#
#-------------------------------------------------

QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtKabuChart
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    backgroundthread.cpp \
    datafile.cpp \
    mygraphicsscene.cpp \
    mygraphicsview.cpp \
    tradedata.cpp

HEADERS  += mainwindow.h \
    backgroundthread.h \
    datafile.h \
    mygraphicsscene.h \
    mygraphicsview.h \
    tradedata.h

###----- refer to http://elisp.net/qt-doc-ja_JP/qmake-tutorial.html
###http://emblog-qt.hatenablog.jp/entry/qtcreator_variable
macx {
    message("mac")
    exists(/usr/local/lib/libta_lib.dylib){
        message("ex ta-lib")
        SOURCES += fortalib.cpp
        HEADERS  += fortalib.h
        LIBS += -L/usr/local/lib -lta_lib ##一般的にはこちら
        #LIBS += -L/usr/local/Cellar/ta-lib/0.4.0/lib -lta_lib ##QtCreatorをアップデートして自分の環境ではこちらでなければ動かなくなってしまいました
        INCLUDEPATH += /usr/local/include/ta-lib
    }
}

unix:!macx {
    message("unix")
    exists(/usr/local/lib/libta_lib.so){
        message("ex ta-lib")
        SOURCES += fortalib.cpp
        HEADERS  += fortalib.h
        LIBS += -L/usr/local/lib -lta_lib
        INCLUDEPATH += /usr/local/include/ta-lib
    }
}

#windows環境は自信がないので参考程度
win32|win64 {
message("win")
    ##以下ではライブラリの関数が見つからずエラーになってしまう　原因不明
#    exists(c:/ta-lib/c/include/ta_libc.h){
#        message("ex_win_ta_libc.h")
#        SOURCES += fortalib.cpp
#        HEADERS  += fortalib.h
#        LIBS += c:/ta-lib/c/lib/ta_libc_cdd.lib
#        INCLUDEPATH += c:/ta-lib/c/include
#   }
}


#QMAKE_CXXFLAGS += -std=c++11 ##これは必要なさそう
CONFIG += c++11


DISTFILES += \
    reference.txt \
    licence.txt \
    usage.html

RESOURCES += \
    hint.qrc
