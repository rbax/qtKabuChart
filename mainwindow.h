/*********************************************************************
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 * *******************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mygraphicsview.h"

#include <QMainWindow>
#include <QtCore>
#include <QtGui>

#include <QToolBar>
#include <QStatusBar>

#include <QDockWidget>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent* event);
    void keyPressEvent(QKeyEvent * event);


private slots:
    void menuSelected();
    void menuCheckedChanged(bool checked);
    void checkBoxChanged(bool checked);
    void comboCurrentChanged(int index);
    void comboCurrentChanged(const QString &str);
    void tbEditTextChanged(const QString & text);
    void tbEditFinished();
    void btnClicked();

    void dockVisibleChanged(bool flag);

    void viewContextMenuRequested(const QPoint& point);
    void viewContextMenuClick();

    void treeItemExpanded(QTreeWidgetItem *item);
    void treeCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void treeContextMenuRequested(const QPoint &pos);

    void on_spinMA_editingFinished();

    void colorEditEnter();

    void deleteActiveTreeNode();
    void addFolder();
    void addItem();
    void removeItem();

    void backgroundFinished();
    void backgroundRecieveMessage(QString message);


private:
    QToolBar *mainToolBar;
    QStatusBar *status;
    QDockWidget *dockWidget;
    QSplitter *splitter;
    QTreeWidget *treeWidget;
    QTabWidget *tab;
    QComboBox *comboAshi;
    QComboBox *comboIndicator;
    QLineEdit *tbCode;
    QLineEdit *tbMonth;
    QCheckBox *checkVol;
    QCheckBox *checkMa[4];
    QSpinBox *spinMa[4];
    QLineEdit *tbDatadir;
    QLineEdit *tbIndexfile;
    QCheckBox *checkCredit;
    void initializeWindow();

    QMenu* viewContextMenu; //viewのContextMenu
    QAction* viewContextAction[4];


    tradeData td;
    MyGraphicsView *view;
    MyGraphicsScene *scene;

    void settingSave();

    bool changeDataDir();

    bool isReady;
    bool isBackgroundRunning;
    bool isShowMessage;
    bool isDebug;

    int ma[3][4];
    bool isMa[3][4];
    int maWidth[3][4];
    bool isBollinger;
    bool isHeikin;
    bool isIchi;
    bool isLine;
    bool isParabolic;


    void setTreeview(int pos);
    void AddRoot(QString name, QString description, QMap<int, QString> list);
    void AddChild(QTreeWidgetItem *parent, QString name, QString description);

    void downloadKdb(int type, int max = 60);
    bool changeBrand(int code);


    QStringList brandFolder;

    QAction* createMenuAction(QString nameObj, QString text, QString tip, bool checktable = false, bool checked = false);


};


#endif // MAINWINDOW_H
