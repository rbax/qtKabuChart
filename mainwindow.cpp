/**MainWindow.cpp***********
 * Copyright (c) 2016 nari
 * This source is released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 ***************************/


#if __has_include("ta_libc.h")
#define IS_TALIB 1
#endif

#include "mainwindow.h"
#include "datafile.h"
#include "backgroundthread.h"

#include <QMenuBar>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTextBrowser>

#include <QFileDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
#ifdef Q_OS_WIN32
    qDebug() << "Windows OS";
#endif

#ifdef Q_OS_MAC
    qDebug() << "Mac OS" << "curr dir" << QDir::current();
#endif

    isDebug = true;

    initializeWindow();

    td.ReadIndexToDb();
    setTreeview(0);//登録銘柄Tree
    QFile file101(td.getDatadir() + "/0101");
    bool f = file101.exists();
    if(f)
        changeBrand(101);
    else {
        // 日経平均(0101)ファイルがなければデータディレクトリ設定

        tab->setCurrentIndex(1);
        QMessageBox msgBox;
        msgBox.setText(QString::fromLocal8Bit("omegaデータディレクトリを確認してください"));
        msgBox.exec();
        if(!changeDataDir()){
            qDebug() << "dir error";
            return;
        }
        tab->setCurrentIndex(0);
        f = true;
    }
    // brands一覧をチェックしてなければダウンロード
    if(!td.db.isOpen()) td.db.open();
    QSqlQuery query=QSqlQuery(td.db);
    query.exec(tr("select count(*) from brands"));
    query.next();
    int brands_count = query.value(0).toInt();
    qDebug() << "brands_count" << brands_count;
    if(brands_count < 50){
        status->showMessage(QString::fromLocal8Bit("銘柄一覧を取得するためにk-dbにアクセスしています"));
        downloadKdb(-1);
    }
    // ↑
    isReady = true;

    qDebug() << "ready";
    status->showMessage("ready", 3000);
}

MainWindow::~MainWindow()
{

}

QAction* MainWindow::createMenuAction(QString nameObj, QString text, QString tip, bool checktable, bool checked){
    QAction* action = new QAction(text, this);
    action->setObjectName(nameObj);
    action->setToolTip(tip);
    if(checktable){
        action->setCheckable(true);
        action->setChecked(checked);
        connect(action, SIGNAL(toggled(bool)), this, SLOT(menuCheckedChanged(bool)));
    }
    else
        connect(action, SIGNAL(triggered()), this, SLOT(menuSelected()));
    return action;
}


void MainWindow::initializeWindow(){
    this->setWindowTitle("qtKabuChart");
    this->setObjectName("mainWindow");
    QSettings settings("myChart.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    this->move(settings.value("pos", QPoint(50, 50)).toPoint());
    QSize size = settings.value("size", QSize(800, 600)).toSize();
    resize(size);
    //--settings of QDockWidget and QSplitter--
    QByteArray splitterSize = settings.value("splitterSizes").toByteArray();
    bool isDockFloating = settings.value("isFloating").toBool();
    QSize sizeDock = settings.value("dockSize", QSize(140, 600)).toSize();
    QPoint pointDock = settings.value("pos", QPoint(200, 200)).toPoint();
    settings.endGroup();
    //--
    settings.beginGroup("Data");
    QString indexfile = settings.value("IndexText").toString();
    QString datadir = settings.value("DataDirectory").toString();
    bool iscredit = settings.value("IsCredit", false).toBool();
    settings.endGroup();
    td.initializeDb(datadir, indexfile, iscredit);
    status = statusBar();

    /* ---------------- ↓ menu */
    QAction* newAction = createMenuAction("actDownloadnew", QString::fromLocal8Bit("k-db最新株価ダウンロード(&N)"),QString::fromLocal8Bit( "k-dbから株価データをダウンロードします。"));
    QAction* forceAction = createMenuAction("actRedownload", QString::fromLocal8Bit("k-db最新株価再ダウンロード(&F)"), QString::fromLocal8Bit("k-dbから株価データをダウンロードします。"));
    QAction* yearAction = createMenuAction("actDownloadyear", "download 1year(&Y)", QString::fromLocal8Bit("表示銘柄のみ１年分データをk-dbからダウンロードします。"));
    QAction* allAction = createMenuAction("actDownloadall", QString::fromLocal8Bit("表示中の銘柄の全データをダウンロード(&A)"), QString::fromLocal8Bit("表示銘柄のみ2007年以降のデータをk-dbからダウンロードします。"));
    QAction* settingDataAction = createMenuAction("actSettingdata", QString::fromLocal8Bit("データ設定(&D)"),QString::fromLocal8Bit("データファイル設定。"));
    QAction* settingColorAction = createMenuAction("actSettingcolor", QString::fromLocal8Bit("色設定(&C)"), QString::fromLocal8Bit("色を変更します。"));
    QAction* infoAction = createMenuAction("actInfo", QString::fromLocal8Bit("mChartについて(&I)"), "");
    QAction* quitAction = new QAction(QString::fromLocal8Bit("終了(&Q)"), this);
    quitAction->setToolTip(QString::fromLocal8Bit("アプリケーションを終了します。"));
    quitAction->setObjectName("actQuit");
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    QMenu* fileMenu = menuBar()->addMenu(QString::fromLocal8Bit("ファイル(&F)"));
    QMenu* fileDataMenu = fileMenu->addMenu(QString::fromLocal8Bit("株価ダウンロード(&D)"));
    fileDataMenu->addAction(newAction);
    fileDataMenu->addAction(forceAction);
    fileDataMenu->addAction(yearAction);
    fileDataMenu->addAction(allAction);
    fileMenu->addSeparator();
    QMenu* fileSettingMenu = fileMenu->addMenu(QString::fromLocal8Bit("設定(&S)"));
    fileSettingMenu->addAction(settingDataAction);
    fileSettingMenu->addAction(settingColorAction);
    fileMenu->addSeparator();
    fileMenu->addAction(infoAction);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    QAction* addBrandAction = createMenuAction("actTreeAddbrand", QString::fromLocal8Bit("銘柄追加(&A)"), QString::fromLocal8Bit("表示されている銘柄を最上位フォルダに追加します。"));
    QAction* deleteBrandAction = createMenuAction("actTreeDeletebrand", QString::fromLocal8Bit("銘柄削除(&D)"), QString::fromLocal8Bit("ツリーで選択されている銘柄を削除します。"));
    QAction* addFolderAction = createMenuAction("actTreeAddfolder", QString::fromLocal8Bit("フォルダ作成(&F)"), QString::fromLocal8Bit("ツリーに新規フォルダを作成します。"));
    QAction* deleteFolderAction = createMenuAction("actTreeDeletefolder", QString::fromLocal8Bit("フォルダ削除(&R)"), QString::fromLocal8Bit("フォルダを削除します。"));
    QMenu* editMenu = this->menuBar()->addMenu(QString::fromLocal8Bit("編集(&E)"));
    QMenu* editSubMenu = editMenu->addMenu(QString::fromLocal8Bit("お気に入り(&S)"));
    editSubMenu->addAction(addBrandAction);
    editSubMenu->addAction(deleteBrandAction);
    editSubMenu->addSeparator();
    editSubMenu->addAction(addFolderAction);
    editSubMenu->addAction(deleteFolderAction);
    /*  ↑ menu */

    //-------- settings of chart
    settings.beginGroup("Chart");
    QString currentOscilator = settings.value("oscillator", "none").toString();
    int monthRange = settings.value("month", 6).toInt();
    bool isVolume = settings.value("Vol").toBool();


    /* ----------------  ↓ toolBar  */
    if(isDebug)
        qDebug() << "toolbar";

    mainToolBar = new QToolBar();
    mainToolBar->setObjectName("mainToolBar");
    addToolBar(mainToolBar);
    comboAshi = new QComboBox();
    comboAshi->setObjectName("ashi");
    comboIndicator = new QComboBox();
    comboIndicator->setObjectName("indicator");
    comboAshi->setFocusPolicy(Qt::NoFocus);
    comboIndicator->setFocusPolicy(Qt::NoFocus);
    QStringList ashis;
    ashis << QString::fromLocal8Bit("日足") << QString::fromLocal8Bit("週足") << QString::fromLocal8Bit("月足");
    comboAshi->addItems(ashis);
    comboAshi->setCurrentIndex(0);
    QStringList inds;
    inds << "none";
#if IS_TALIB
    inds << "MACD" << "RSI" << QString::fromLocal8Bit("モメンタム") << QString::fromLocal8Bit("ATR");
#endif

    comboIndicator->setToolTip(QString::fromLocal8Bit("oscillator系のIndicator表示を切り替えます"));
    comboIndicator->addItems(inds);
    comboIndicator->setCurrentText(currentOscilator);
    connect(comboAshi, SIGNAL(currentIndexChanged(int)), this, SLOT(comboCurrentChanged(int)));
    connect(comboIndicator, SIGNAL(currentIndexChanged(QString)), this, SLOT(comboCurrentChanged(QString)));
    checkVol = new QCheckBox();
    checkVol->setObjectName("checkVol");
    checkVol->setText(QString::fromLocal8Bit("出来高"));
    checkVol->setToolTip(QString::fromLocal8Bit("出来高表示を切り替えます"));
    checkVol->setChecked(isVolume);
    checkVol->setFocusPolicy(Qt::NoFocus);

    mainToolBar->addWidget(new QLabel("code"));
    //QLineEdit *tbCode;
    tbCode = new QLineEdit();
    tbCode->setObjectName("tbCode");
    tbCode->setMaximumWidth(40);
    tbCode->setMaxLength(5);
    tbCode->setToolTip(QString::fromLocal8Bit("銘柄コードを入力してEnter"));
    tbCode->setFocusPolicy(Qt::ClickFocus);
    mainToolBar->addWidget(tbCode);

    comboAshi->setMaximumWidth(60);

    mainToolBar->addWidget(comboAshi);
    tbMonth = new QLineEdit();
    tbMonth->setObjectName("tbMonth");
    tbMonth->setMaximumWidth(28);
    tbMonth->setText(tr("%1").arg(monthRange));
    tbMonth->setToolTip(QString::fromLocal8Bit("表示期間を月数で入力してEnter"));
    tbMonth->setFocusPolicy(Qt::ClickFocus);
    //connect(tbCode, SIGNAL(editingFinished()), this, SLOT(tbEditFinished()));
    connect(tbCode, SIGNAL(textChanged(QString)), this, SLOT(tbEditTextChanged(QString)));
    connect(tbMonth, SIGNAL(editingFinished()), this, SLOT(tbEditFinished()));
    connect(checkVol, SIGNAL(clicked(bool)), this, SLOT(checkBoxChanged(bool)));

    mainToolBar->addWidget(new QLabel(QString::fromLocal8Bit("月数")));
    mainToolBar->addWidget(tbMonth);
    mainToolBar->addWidget(checkVol);
    mainToolBar->addWidget(comboIndicator);
    mainToolBar->addSeparator();

    QAction* showValueAction = createMenuAction("actShowprice", QString::fromLocal8Bit("株価表示"), QString::fromLocal8Bit("株価等の情報表示を切り替えます。"), true, true);
    //showValueAction->setChecked(true);
    QAction* crossAction = createMenuAction("actScale", QString::fromLocal8Bit("日付スケール"), QString::fromLocal8Bit("垂直スケールを表示して、日付をドラッグして4本値を表示できます。"), true, false);
    //crossAction->setChecked(false);
    mainToolBar->addAction(showValueAction);
    mainToolBar->addAction(crossAction);
    mainToolBar->addSeparator();
    quitAction->setText("Close");

    mainToolBar->addAction(quitAction);
   /*  ↑ toolBar */

    //------------- QDockWidget and QGraphicsView
    dockWidget = new QDockWidget();
    dockWidget->setObjectName("dockWidget");
    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
    tab = new QTabWidget();
    tab->setObjectName("tab");
    dockWidget->setWidget(tab);
    tab->setTabPosition(QTabWidget::South);

    //------------- QDockWidget
    splitter = new QSplitter(dockWidget);
    splitter->setObjectName("splitter");
    //dockWidget->setWidget(splitter);
    dockWidget->setFloating(isDockFloating);
    dockWidget->resize(sizeDock);
    dockWidget->move(pointDock);

    tab->addTab(splitter, "main");
    QWidget *tabPage2 = new QWidget();
    tab->addTab(tabPage2, QString::fromLocal8Bit("設定"));

    //------------------QTreeWidget
    treeWidget = new QTreeWidget();
    treeWidget->setObjectName("brandTree");
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);    // need ContextMenu
    treeWidget->setHeaderHidden(true);
    treeWidget->setColumnCount(2);
    treeWidget->setColumnWidth(0,60);
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    treeWidget->setDragEnabled(true);
    treeWidget->viewport()->setAcceptDrops(true);
    treeWidget->setDropIndicatorShown(true);
    treeWidget->setDragDropMode(QAbstractItemView::InternalMove);
    treeWidget->setIndentation(5); //デフォルトではアイテム前のインデント多すぎ
    splitter->addWidget(treeWidget);
    connect(treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(treeCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(treeItemExpanded(QTreeWidgetItem*)));
    connect(treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(treeContextMenuRequested(QPoint)));

    if(isDebug)
        qDebug() << "tree connected";

    QFrame* frame = new QFrame();
    splitter->addWidget(frame);
    splitter->setOrientation(Qt::Orientation::Vertical);
    splitter->restoreState(splitterSize);
    splitter->setFocusPolicy(Qt::NoFocus);

    //QWidget *page1 = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    frame->setLayout(layout);

    QHBoxLayout *hBox = new QHBoxLayout();
    QCheckBox *checkBollinger = new QCheckBox();
    checkBollinger->setObjectName("checkBollinger");
    checkBollinger->setText("Bollinger");
    checkBollinger->setToolTip(QString::fromLocal8Bit("ボリンジャーバンドを表示します（MAは非表示）"));
    hBox->addWidget(checkBollinger);
    layout->addLayout(hBox);
    hBox = new QHBoxLayout();
    QCheckBox *checkIchi = new QCheckBox();
    checkIchi->setObjectName("checkIchi");
    checkIchi->setText(QString::fromLocal8Bit("一目"));
    checkIchi->setToolTip(QString::fromLocal8Bit("一目均衡表を表示します"));
    hBox->addWidget(checkIchi);
    QCheckBox *checkHeikin = new QCheckBox();
    checkHeikin->setObjectName("checkHeikin");
    checkHeikin->setText(QString::fromLocal8Bit("平均足"));
    checkHeikin->setToolTip(QString::fromLocal8Bit("ローソク足を平均足に変更します"));
    hBox->addWidget(checkHeikin);
    layout->addLayout(hBox);

    hBox = new QHBoxLayout();
    QCheckBox *checkLine = new QCheckBox();
    checkLine->setText(QString::fromLocal8Bit("ライン"));
    checkLine->setObjectName("checkLine");
    checkLine->setToolTip(QString::fromLocal8Bit("ローソク足を終値のラインチャートに変更します"));
    hBox->addWidget(checkLine);
    QCheckBox *checkParabolic = new QCheckBox();
    checkParabolic->setObjectName("checkParabolic");
    checkParabolic->setText("Parabolic");
    checkParabolic->setToolTip(QString::fromLocal8Bit("バラボリックを表示します"));
    hBox->addWidget(checkParabolic);
    layout->addLayout(hBox);
    checkBollinger->setChecked(settings.value("Bollinger", false).toBool());
    checkIchi->setChecked(settings.value("Ichimoku", false).toBool());
    checkHeikin->setChecked(settings.value("Heikin", false).toBool());
    checkLine->setChecked(settings.value("Line", false).toBool());
    checkParabolic->setChecked(settings.value("Parabolic", false).toBool());
    checkBollinger->setFocusPolicy(Qt::NoFocus);
    checkParabolic->setFocusPolicy(Qt::NoFocus);
    checkIchi->setFocusPolicy(Qt::NoFocus);
    checkHeikin->setFocusPolicy(Qt::NoFocus);
    checkLine->setFocusPolicy(Qt::NoFocus);
    connect(checkBollinger, SIGNAL(clicked(bool)), this, SLOT(checkBoxChanged(bool)));
    connect(checkIchi, SIGNAL(clicked(bool)), this, SLOT(checkBoxChanged(bool)));
    connect(checkHeikin, SIGNAL(clicked(bool)), this, SLOT(checkBoxChanged(bool)));
    connect(checkLine, SIGNAL(clicked(bool)), this, SLOT(checkBoxChanged(bool)));
    connect(checkParabolic, SIGNAL(clicked(bool)), this, SLOT(checkBoxChanged(bool)));


    int array[3][4] = { {  5,  25,  75,  200, }, {  6,  13,  26,  52, }, {  6,  12,  24, 0, }, };
    for(int i =0 ; i < 3 ; i++){
        for(int j = 0 ; j < 4 ; j++){
            ma[i][j] = settings.value(tr("MA%1%2").arg(i == 0 ? "d" : (i == 1 ? "w" : "m")).arg(j + 1), array[i][j]).toInt();
            isMa[i][j] = settings.value(tr("MA%1f%2").arg(i == 0 ? "d" : (i == 1 ? "w" : "m")).arg(j + 1), (i == 2 && j == 3) ? false : true).toBool();
        }
    }
    layout->addWidget(new QLabel(QString::fromLocal8Bit("MA（移動平均線）")));
    QGridLayout *grid = new QGridLayout();
    for(int i =0 ; i < 4 ; i++){
        checkMa[i] = new QCheckBox();
        checkMa[i]->setObjectName(tr("checkMa%1").arg(i + 1));
        checkMa[i]->setText("");
        checkMa[i]->setMaximumWidth(26);
        checkMa[i]->setChecked(isMa[comboAshi->currentIndex()][i]);
        spinMa[i] = new QSpinBox();
        spinMa[i]->setObjectName(tr("spin%1").arg(i + 1));
        spinMa[i]->setMinimum(0);
        spinMa[i]->setMaximum(500);
        spinMa[i]->setMaximumWidth(48);
        spinMa[i]->setAlignment(Qt::AlignRight);
        spinMa[i]->setValue(ma[comboAshi->currentIndex()][i]);
        grid->addWidget(checkMa[i], i < 2 ? 0 : 1, i % 2 * 2);
        grid->addWidget(spinMa[i], i < 2 ? 0 : 1, i % 2 * 2 + 1);
        checkMa[i]->setFocusPolicy(Qt::NoFocus);
        spinMa[i]->setFocusPolicy(Qt::ClickFocus);
        connect(checkMa[i], SIGNAL(clicked(bool)), this, SLOT(checkBoxChanged(bool)));
        connect(spinMa[i], SIGNAL(editingFinished()), this, SLOT(on_spinMA_editingFinished()));

    }
    layout->addLayout(grid);
    layout->addStretch();

    frame->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);

    QAction* actionPageIndicator = createMenuAction("actionPageIndicator", "Indicators", QString::fromLocal8Bit("Indicatorの設定ページに移動します。"));
    QAction* actionPageData = createMenuAction("actionPageData", QString::fromLocal8Bit("データ設定ページ"), QString::fromLocal8Bit("データ設定ページに移動します。"));
    QAction* actionPageColor = createMenuAction("actionPageColor", QString::fromLocal8Bit("色変更ページ"), QString::fromLocal8Bit("色変更ページに移動します。"));
    frame->addAction(actionPageIndicator);
    frame->addAction(actionPageData);
    frame->addAction(actionPageColor);



    QVBoxLayout *layout2 = new QVBoxLayout;
    layout2->setMargin(0);
    tabPage2->setLayout(layout2);
    layout2->addWidget(new QLabel(QString::fromLocal8Bit("データ設定")));

    hBox = new QHBoxLayout();
    grid = new QGridLayout();
    QPushButton *btn = new QPushButton();
    btn->setObjectName("btnDatadir");
    btn->setText("data");
    btn->setToolTip(QString::fromLocal8Bit("株価データフォルダを指定します"));
    btn->setFixedWidth(80);
    connect(btn, SIGNAL(pressed()), this, SLOT(btnClicked()));
    grid->addWidget(btn, 0, 0);
    tbDatadir = new QLineEdit();
    tbDatadir->setObjectName("tbDatadir");
    tbDatadir->setText(td.getDatadir());
    connect(tbDatadir, SIGNAL(editingFinished()), this, SLOT(tbEditFinished()));
    grid->addWidget(tbDatadir);
    tbDatadir->setToolTip(tbDatadir->text());
    checkCredit = new QCheckBox();
    checkCredit->setObjectName("checkCredit");
    checkCredit->setText(QString::fromLocal8Bit("信用売買残"));
    checkCredit->setToolTip(QString::fromLocal8Bit("１日のデータ形式をOmegaChart互換の信用売残買残の８列にします"));
    checkCredit->setChecked(td.isCredit());
    connect(checkCredit, SIGNAL(clicked(bool)), this, SLOT(checkBoxChanged(bool)));
    grid->addWidget(checkCredit, 2, 0);
    btn = new QPushButton();
    btn->setObjectName("btnIndexfile");
    btn->setText("IndexText");
    btn->setToolTip(QString::fromLocal8Bit("OmegaChartのindex.txtを指定します"));
    btn->setFixedWidth(80);
    connect(btn, SIGNAL(pressed()), this, SLOT(btnClicked()));
    grid->addWidget(btn, 3, 0);
    tbIndexfile = new QLineEdit();
    tbIndexfile->setObjectName("tbIndexfile");
    tbIndexfile->setText(td.getIndextext());
    connect(tbIndexfile, SIGNAL(editingFinished()), this, SLOT(tbEditFinished()));
    grid->addWidget(tbIndexfile);
    tbIndexfile->setToolTip(tbIndexfile->text());

    layout2->addLayout(grid);
    layout2->addStretch();

    view = new MyGraphicsView(this);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget *tabPage3 = new QWidget();

    QVBoxLayout *layout3 = new QVBoxLayout(tabPage3);
    tabPage3->setLayout(layout3);
    scroll->setWidget(tabPage3);

    tab->addTab(scroll, QString::fromLocal8Bit("色"));

    layout3->setMargin(0);
    layout3->setAlignment(Qt::AlignTop);
    hBox = new QHBoxLayout();
    grid = new QGridLayout();
    QLabel* label = new QLabel();
    label->setText(QString::fromLocal8Bit("色設定"));
    label->setToolTip(QString::fromLocal8Bit("テキストボックスを選択してEnterキーを押すと色変更可能です。"));
    hBox->addWidget(label);
    layout3->addLayout(hBox);

    QStringList colors = view->getKeysList();
    int count = 0;
    for(int i = 0 ; i < colors.count() ; i++){
        QString cn = colors.at(i);
        if(!cn.contains("Color"))
            continue;
        if(count % 2 == 0){
            hBox = new QHBoxLayout();
        }
        label = new QLabel();
        label->setObjectName(cn);
        QString cl = view->getLabel(cn);
        if(cl == "")
            cl = cn;
        label->setText(cl);
        label->setMaximumWidth(45);
        hBox->addWidget(label);
        //grid->addWidget(label, row, j * 2);
        QLineEdit *edit = new QLineEdit();
        edit->setObjectName(colors.at(i));
        edit->setMaximumWidth(24);
        QColor color = QColor(settings.value(cn).toString());
        //edit->setText(color.name());
        edit->setToolTip(color.name());
        edit->setStyleSheet(tr("QLineEdit {background-color: %1;}").arg(color.name()));
        edit->setFocusPolicy(Qt::ClickFocus);
        connect(edit, SIGNAL(returnPressed()), this, SLOT(colorEditEnter()));
        hBox->setContentsMargins(0, 0, 0, 0);
        hBox->addWidget(edit);
        hBox->setSpacing(3);
        count++;
        //grid->addWidget(edit, row, j * 2 + 1);

        //if(j == 1 || i == colors.count() - 1)
        if( (count % 2 == 1) || (i == colors.count() - 1))
            layout3->addLayout(hBox);
        if(i == 16)
            count++;
    }
    layout3->addWidget(new QLabel(QString::fromLocal8Bit("色テキストボックスを選択して\nEnterキーを押すと変更できます")));


    tabPage2->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    tabPage3->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    tabPage2->addAction(actionPageIndicator);
    tabPage2->addAction(actionPageData);
    tabPage2->addAction(actionPageColor);

    tabPage3->addAction(actionPageIndicator);
    tabPage3->addAction(actionPageData);
    tabPage3->addAction(actionPageColor);




    settings.endGroup();
    // -------------- settings end

    QWidget *tabPage4 = new QWidget();
    tab->addTab(tabPage4, "hint");
    QVBoxLayout *layout4 = new QVBoxLayout(tabPage4);
    QTextBrowser *browser = new QTextBrowser();
    //リソース(画像、音声)の埋め込み
    //http://qt-log.open-memo.net/sub/other__embed_resource.html
    //http://qt.misfrog.com/
    tabPage4->setLayout(layout4);
    layout4->addWidget(browser);
    QFile inputFile(":/usage.html");
    inputFile.open(QIODevice::ReadOnly);
    QTextStream in(&inputFile);
    QString line = in.readAll();
    //qDebug() <<  line;
    inputFile.close();
    browser->setHtml(line);


    //connect(dockWidget, SIGNAL(featuresChanged(QDockWidget::DockWidgetFeatures)), this, SLOT(dockChanged(QDockWidget::DockWidgetFeatures)));
    connect(dockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(dockVisibleChanged(bool)));

    //----------------- MyGraphicsView
    if(isDebug)
        qDebug() << "MyGraphicsView";
    setCentralWidget(view);
    view->setTradedata(&td);

    //----- MyGrapgicsView setting
    view->setParentSize(this->width(), this->height());
    view->setSetting("showValue", "1"); //株価数値ウィンドウ表示
    view->setDockWidth(dockWidget->width());
    for(int i = 0; i < 4 ; i++){
        view->setSetting(tr("ma%1").arg(i + 1), tr("%1").arg(checkMa[i]->isChecked() ? spinMa[i]->value() : 0));
    }
    view->setMonth(tbMonth->text().toInt(), false);
    view->setSetting("volume", checkVol->isChecked() ? "true" : "false");
    view->setSetting("bb", tr("%1").arg(isBollinger));
    view->setSetting("heikin", tr("%1").arg(isHeikin));
    view->setSetting("ichi", tr("%1").arg(isIchi));
    view->setSetting("line", tr("%1").arg(isLine));
    view->setSetting("pb", tr("%1").arg(isParabolic));
    view->setSetting("oscillator", comboIndicator->currentText());
    if(isDebug)
        qDebug() <<  __FILE__ << __FUNCTION__ << "line:" << __LINE__;
    //viewのContextMenu
    //refer to http://www.codeprogress.com/cpp/libraries/qt/showQtExample.php?index=541&key=QActionContextMenuEvent
    viewContextMenu = new QMenu(this);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, SIGNAL(customContextMenuRequested(const QPoint)), this, SLOT(viewContextMenuRequested(const QPoint&)));
    viewContextAction[0] = viewContextMenu->addAction(QString::fromLocal8Bit("監視に追加"));
    viewContextAction[0]->setObjectName("viewContextAdd");
    viewContextAction[0]->setToolTip(QString::fromLocal8Bit("お気に入り先頭フォルダに表示銘柄を追加します"));
    connect(viewContextAction[0], SIGNAL(triggered()), this, SLOT(viewContextMenuClick()));
    /*
    viewContextAction[1] = viewContextMenu->addAction(QString::fromLocal8Bit("表示銘柄の1年間データをダウンロード"));
    viewContextAction[1]->setObjectName("viewContextDownload");
    connect(viewContextAction[1], SIGNAL(triggered()), this, SLOT(viewContextMenuClick()));
    */
    viewContextAction[1] = viewContextMenu->addAction(QString::fromLocal8Bit("ライン描画"));
    viewContextAction[1]->setToolTip(QString::fromLocal8Bit("マウスをドラッグして直線を描きます"));
    viewContextAction[1]->setObjectName("viewContextDrawline");
    connect(viewContextAction[1], SIGNAL(triggered()), this, SLOT(viewContextMenuClick()));
    viewContextAction[2] = viewContextMenu->addAction(QString::fromLocal8Bit("ライン消去"));
    viewContextAction[2]->setToolTip(QString::fromLocal8Bit("マウスで描いた直線を消去します。"));
    viewContextAction[2]->setObjectName("viewContextCrearline");
    connect(viewContextAction[2], SIGNAL(triggered()), this, SLOT(viewContextMenuClick()));

    view->setFocusPolicy(Qt::ClickFocus);

}

/* ↓ slots */
void MainWindow::dockVisibleChanged(bool flag){
    //qDebug() <<  __FILE__ << __FUNCTION__ << "line:" << __LINE__;
    view->setDockWidth(!flag || dockWidget->isFloating() ? 0 : dockWidget->width());
    //qDebug() << "dockWidget width changed" << flag << "floating" << dockWidget->isFloating();
}

void MainWindow::colorEditEnter(){
    //refer to https://wiki.qt.io/How_to_Change_the_Background_Color_of_QWidget/ja
    QLineEdit* edit = qobject_cast<QLineEdit*>(sender());
    QString name = edit->objectName();
    qDebug() << name;
    QColorDialog *dialog = new QColorDialog(this);
    //QColor color = dialog ::getColor(QColor(edit->text()), this );
    QColor color = dialog->getColor(QColor(edit->text()), this );
    if( !color.isValid() ){
       return;
    }
    //qDebug() << color;
    //edit->setText(color.name());
    edit->setStyleSheet(tr("QLineEdit {background-color: %1;}").arg(color.name()));
    if(view != NULL){
        view->updateChart(name, color.name());
    }
    if(name == "canvasColor"){
        QMessageBox msgBox;
        msgBox.setText(QString::fromLocal8Bit("背景色の変更は再起動後に反映されます"));
        msgBox.exec();
    }
}

void MainWindow::menuSelected()
{
    //refer to http://hkpr.info/qt/sample/html/s006.php
    QAction* action = qobject_cast<QAction*>(sender());
    if (action->objectName() == "actSettingdata")
        tab->setCurrentIndex(1);
    else if (action->objectName() == "actSettingcolor")
        tab->setCurrentIndex(2);
    else if(action->objectName() == "actionPageIndicator")
        tab->setCurrentIndex(0);
    else if(action->objectName() == "actionPageData")
        tab->setCurrentIndex(1);
    else if(action->objectName() == "actionPageColor")
        tab->setCurrentIndex(2);
    else if (action->objectName() == "actDownloadnew")
        downloadKdb(1);
    else if (action->objectName() == "actRedownload")
        downloadKdb(0);
    else if (action->objectName()== "actDownloadyear")
        downloadKdb(2);
    else if (action->objectName() == "actDownloadall")
        downloadKdb(3);
    else if(action->objectName() == "actTreeAddbrand")
        addItem();
    else if(action->objectName() == "actTreeDeletebrand")
        removeItem();
    else if(action->objectName() == "actTreeAddfolder")
        addFolder();
    else if(action->objectName() == "actTreeDeletefolder")
        deleteActiveTreeNode();
    else if(action->objectName() == "actInfo"){
        QMessageBox msg;
        msg.setText("mChart  ver. Alpha 1.0 【評価版】");
        msg.setInformativeText(QString::fromLocal8Bit(
                "This is chart program of japanese stock.\n\n\n"
                 "株価チャート評価版です。\n"
                "株価及びテクニカル指標等数値の不正確あるいは重大なバグが存在する可能性があります。\n"
                "このプログラムの利用に伴ういかなる損害についての責任を負いません。\n\n"
                "ソースコードはオープンソース（MIT License）で、自由に複製・修正を認めます。\n"
                "     Copyright (c) 2016 nari\n"));
        msg.exec();
    }
    else if(action->objectName() == "actHelp"){

    }
    else
      qDebug() << "action" << action->objectName() << action->text();
}

void MainWindow::menuCheckedChanged(bool checked){
    //qDebug() << sender() << checked;
    QAction* action = qobject_cast<QAction*>(sender());
    if(view != NULL){
    if(action->objectName() == "actShowprice")
        view->updateChart("showValue", checked);
    else if(action->objectName() == "actScale"){

            view->setCross(checked);
        }
    }
}

//contextMenu
void MainWindow::viewContextMenuRequested(const QPoint& point)
{
    viewContextMenu->popup(mapToGlobal(point));
}
void MainWindow::viewContextMenuClick(){
    QAction* action = qobject_cast<QAction*>(sender());
    qDebug() << "viewContext" << action->objectName() << action->text();
    if(action->objectName() == "viewContextAdd")
        addItem();
    else if(action->objectName() == "viewContextDownload")
        downloadKdb(2);
    else if(action->objectName() == "viewContextDrawline")
        view->setDrawLine();
    else if(action->objectName() == "viewContextCrearline")
        view->clearDraw();
}

void MainWindow::comboCurrentChanged(int index){
    QComboBox* combo = qobject_cast<QComboBox*>(sender());
    qDebug() << combo->objectName() << index;
    if(combo->objectName() == "ashi" && isReady && td.list_date[0].count() > 0){
        td.convert(index);
        //view->updateChart(index, tbMonth->text().toInt());
        for(int i = 0; i < 4 ; i++){
            checkMa[i]->setChecked(isMa[index][i]);
            spinMa[i]->setValue(ma[index][i]);
        }
        view->updateChart(index);
    }
}
void MainWindow::comboCurrentChanged(const QString &str){
    QComboBox* combo = qobject_cast<QComboBox*>(sender());
    //qDebug() << combo->objectName() << str;
    if(combo->objectName() == "indicator" && isReady && td.list_date[comboAshi->currentIndex()].count() > 0){
        view->updateChart("oscillator", str);
    }
}

bool MainWindow::changeDataDir(){
    QString folder = QFileDialog::getExistingDirectory(this, "data directory", tbDatadir->text());
   if(folder != "") {
       if(QDir(folder).exists()){
            tbDatadir->setText(folder);
            td.setDatadir(tbDatadir->text());
            QFile file101(tbDatadir->text() + "/0101");
            if(file101.exists()){
               int ret = td.load(101);
               if(ret < 0){
                   bool f = td.isCredit();
                   td.setCredit(!f);
                   if(td.load(101) == 0){
                       checkCredit->setChecked(!f);
                       QMessageBox msgBox;
                       msgBox.setText(QString::fromLocal8Bit("データ形式（信用）を変更しました"));
                       msgBox.exec();
                   }
               }
               view->showBrand(101, comboAshi->currentIndex(), tbMonth->text().toInt());
            }
            else{
                td.setCredit(false);
                checkCredit->setChecked(false);

                QMessageBox msgBox;
                msgBox.setText(QString::fromLocal8Bit("「Yes」で株価データをダウンロードします。"
                               "\n日経平均１年分取得後チャートが表示されます。"
                               "\n続いて個別銘柄の最終データを取得します。"
                               "\n初回のみ終了ダイアログ前の個別銘柄表示できません。"
                                "\n\n銘柄データ数が少ない場合、随時１年分を追加ダウンロードします。"
                               "\n時々サーバーエラーが出てアクセス拒否される場合もあるようです。"
                               "\nこの場合にはかなり時間をおいて再チャレンジしてください。"));
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::Yes);
                if(msgBox.exec() == QMessageBox::Yes)
                    downloadKdb(0, -1);
                else
                    return false;
            }
       }
       return true;
   }
   else
       return false;
}

void MainWindow::btnClicked(){
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    qDebug() << btn->objectName();
    if(btn->objectName() == "btnDatadir"){
        changeDataDir();
    }
    else if (btn->objectName() == "btnIndexfile"){
        QString filename = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("インデックスファイル"));
        if(filename != ""){
            QFile fileidx(filename);

            if(fileidx.fileName().contains("index.txt") && fileidx.exists()){
                tbIndexfile->setText(filename);
                td.setIndextext(filename);
                if(td.ReadIndexToDb()){
                    status->showMessage(QString::fromLocal8Bit("index.txtを読み込みました"), 3000);
                    if(td.info.code > 1300){
                        td.load(td.info.code, comboAshi->currentIndex());
                        view->updateChart();
                    }
                }
            }

        }
       // statusBar->showMessage(QString::fromLocal8Bit("index.txtが見つかりません。株価は分割補正されません。"));
    }
}

void MainWindow::tbEditTextChanged(const QString &text){
    QLineEdit* tb = qobject_cast<QLineEdit*>(sender());
    if(tb->objectName() == "tbCode"){
        qDebug() << "tbCode" << text;
        int code = text.toInt();
        if(code > 100 && td.load(code) == 0){
            changeBrand(code);
        }
    }

}

void MainWindow::tbEditFinished(){
    QLineEdit* tb = qobject_cast<QLineEdit*>(sender());
    //qDebug() << tb->objectName() << tb->text();
    if(tb->objectName() == "tbIndexfile")
        td.setIndextext(tb->text());
    else if(tb->objectName() == "tbDatadir")
        td.setDatadir(tb->text());
    else if(tb->objectName() == "tbCode")
        changeBrand(tb->text().toInt());
    else if(tb->objectName() == "tbMonth")
        view->setMonth(tb->text().toInt(), true);
}

void MainWindow::checkBoxChanged(bool checked){
    QCheckBox* cb = qobject_cast<QCheckBox*>(sender());
    qDebug() << cb->objectName() << cb->text() << checked;
    if(cb->objectName().indexOf("checkMa") == 0){
        int no = cb->objectName().replace("checkMa", "").toInt();
        qDebug() << "ma" << no;
        ma[comboAshi->currentIndex()][no - 1] = spinMa[no - 1]->value();
        isMa[comboAshi->currentIndex()][no - 1] = checked;
        if(isReady && td.list_date[comboAshi->currentIndex()].count() > 0)
            view->updateChart(tr("ma%1").arg(no), checkMa[no - 1]->isChecked() ? spinMa[no - 1]->value() : 0);
    }
    else if(cb->objectName() == "checkVol" && isReady){
            view->updateChart("volume", checked);
    }
    else if(cb->objectName() == "checkBollinger"){
        isReady = false;
        if(checked){
            for(int i = 0; i < 4 ; i++)
                checkMa[i]->setChecked(false);
            view->setSetting("ma", "0");
       }
       else{
            for(int i = 0 ; i < 4 ; i++){
                if(spinMa[i]->value() > 0){
                    checkMa[i]->setChecked(true);
                    view->setSetting(tr("ma%1").arg(i + 1), spinMa[i]->text());
                }
            }
       }
        isReady=true;
        view->updateChart("bb", checked);
    }
    else if(cb->objectName() == "checkLine" && isReady){
        view->updateChart("line", checked);
    }
    else if(cb->objectName() == "checkHeikin" && isReady){
        view->updateChart("heikin", checked);
    }
    else if(cb->objectName() == "checkParabolic" && isReady){
        view->updateChart("pb", checked);
    }
    else if(cb->objectName() == "checkCredit" && isReady){
        td.setCredit(checked);
        view->setSetting("vol100", checked ? "false" : "true");
    }
    else if(cb->objectName() == "checkIchi" && isReady){
        isReady = false;
        if(checked){
            for(int i = 0; i < 4 ; i++)
                checkMa[i]->setChecked(false);
            view->setSetting("ma", "0");
       }
       else{
            for(int i = 0 ; i < 4 ; i++){
                if(spinMa[i]->value() > 0){
                    checkMa[i]->setChecked(true);
                    view->setSetting(tr("ma%1").arg(i + 1), spinMa[i]->text());
                }
            }
       }
        isReady= true;
        if(isReady)
            view->updateChart("ichi", checked);
    }
}
void MainWindow::on_spinMA_editingFinished(){
    QSpinBox* spin = qobject_cast<QSpinBox*>(sender());
    int no = spin->objectName().replace("spin", "").toInt();
    ma[comboAshi->currentIndex()][no - 1] = spin->value();
    qDebug() << spin->objectName() << no << spin->text() << "ma" << comboAshi->currentIndex() << no - 1 << ma[comboAshi->currentIndex()][no - 1];

    if(isReady && td.list_date[comboAshi->currentIndex()].count() > 0)
        view->updateChart(tr("ma%1").arg(no), checkMa[no - 1]->isChecked() ? spinMa[no - 1]->value() : 0);

}

/* ↑ slots  */

// ?? settings 保存

void MainWindow::settingSave(){
    QSettings settings("myChart.ini", QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("size",this->size());
    settings.setValue("pos", this->pos());
    settings.setValue("splitterSizes", splitter->saveState());
    settings.setValue("isFloating", dockWidget->isFloating());
    settings.setValue("dockSize", dockWidget->size());
    settings.setValue("posDock", dockWidget->pos());
    settings.endGroup();
    settings.beginGroup("Data");
    settings.setValue("IndexText", td.getIndextext());
    settings.setValue("DataDirectory", td.getDatadir());
    settings.setValue("IsCredit", td.isCredit());
    settings.endGroup();
    settings.beginGroup("Chart");
    settings.setValue("month", tbMonth->text().toInt());
    settings.setValue("Vol", checkVol->isChecked());

    for(int i = 0; i < 3 ; i++){
        for(int j =0 ; j < 4 ; j++){
            QString no = tr("%1").arg(j + 1);
            QString dwm;
            switch (i) {
            case 1:
                dwm = "w";
                break;
            case 2:
                dwm = "m";
                break;
            default:
                dwm = "d";
                break;
            }
            settings.setValue("MA" + dwm + no, ma[i][j]);
            //qDebug() << i << j << "MA" + dwm + no << ma[i][j] << "MA" + dwm + "f" + no << isMa[i][j];
            settings.setValue("MA" + dwm + "f" + no, isMa[i][j]);
            //settings.setValue("MA" + dwm + "Color" + no, maColor[i][j]);
            //settings.setValue("MA" + dwm + "Width" + no, maWidth[i][j]);
        }
    }
/*
    settings.setValue("Ichimoku", checkIchi->isChecked());
    settings.setValue("Bollinger", checkBollinger->isChecked());
    settings.setValue("Parabolic", checkParabolic->isChecked());
    settings.setValue("Heikin", checkHeikin->isChecked());
    settings.setValue("Line", checkLine->isChecked());
*/
    settings.setValue("oscillator", comboIndicator->currentText());
    settings.endGroup();
    if(isDebug)
        qDebug() << "setting saved";

    //treeの追加項目を保存
    if(!td.db.isOpen()) td.db.open();
    QSqlQuery query= QSqlQuery(td.db);
    query = td.db.exec("delete from fileDB.favorite");

    int topCount = treeWidget->topLevelItemCount();
    for (int i = 0; i < topCount; i++)
    {
        QTreeWidgetItem *node = treeWidget->topLevelItem(i);

        QString s = node->text(1);
        if(!brandFolder.contains(s)){
            for(int j = 0; j < node->childCount() ; j++){
                QString sql = tr("insert into fileDB.favorite values (%1, '%2')").arg(node->child(j)->text(0)).arg(s);
                query = td.db.exec(sql);
                //qDebug() << "brands add" << sql;
            }
        }
    }

    //qDebug() << "save favorite" << query.lastError();
    query.exec("drop table fileDB.brands");
    query.exec("create table fileDB.brands as select * from brands");
    //qDebug() << "saved brands" << query.lastError();
    qDebug() <<  __FILE__ << __FUNCTION__ << "line:" << __LINE__;

}
// ↑ sttings

void MainWindow::closeEvent(QCloseEvent *event){

    settingSave();
    td.closeDb();
    //delete td;

    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event){
    view->setParentSize(this->width(), this->height());
    view->updateChart();
    QMainWindow::resizeEvent(event);
}


// TreeWidget 銘柄一覧
void MainWindow::AddRoot(QString name, QString description, QMap<int, QString> list){

    QTreeWidgetItem *itm = new QTreeWidgetItem(treeWidget);
    itm->setText(0,name);
    itm->setText(1, description);
    itm->setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );
    treeWidget->addTopLevelItem(itm);
    if(list.count() > 0){
        QMap<int, QString>::const_iterator it = list.constBegin();
        while (it != list.constEnd()) {
            QString str;
            AddChild(itm, str.setNum(it.key()), it.value());
            ++it;
        }
    }

}

void MainWindow::AddChild(QTreeWidgetItem *parent, QString name, QString description){
    QTreeWidgetItem *itm = new QTreeWidgetItem(); //
    itm->setText(0,name);
    itm->setText(1, description.replace(QString(QString::fromLocal8Bit("業種別株価指数 ")), QString("")));
    itm->setTextAlignment(1,Qt::AlignLeft);
    parent->addChild(itm);
}

void MainWindow::setTreeview(int pos){
    if(pos == 0){       // && brandFolder.count() == 0
        if(!td.db.isOpen()) td.db.open();
        QSqlQuery query=QSqlQuery(td.db);
        QSqlQuery query2=QSqlQuery(td.db);
        QMap<int, QString> list;
        int added = 0;
        query.exec("select distinct folder from favorite;");
        while(query.next()){
            QString grp = query.value(0).toString();
            //qDebug() << query.value(0).toString();
            query2.exec(tr("select favorite.code, brands.name from favorite inner join brands on favorite.code = brands.code where folder = '%1'").arg(grp));
            while(query2.next()){
                //qDebug() << query2.value(0).toInt() << query2.value(1).toString();
                list.insert(query2.value(0).toInt(), query2.value(1).toString());
            }
            AddRoot("", grp, list);
            list.clear();
            added++;
        }
        if(added < 1){
            list.insert(6758, "Sony");
            AddRoot("", QString::fromLocal8Bit("登録銘柄"), list);
            list.clear();
        }
        brandFolder << QString::fromLocal8Bit("指数") << QString::fromLocal8Bit("東証1部") << QString::fromLocal8Bit("東証2部")
                    << QString::fromLocal8Bit("マザーズ") << QString::fromLocal8Bit("JQスタンダード") << QString::fromLocal8Bit("JQグロース")
                    << QString::fromLocal8Bit("札証") << QString::fromLocal8Bit("福証");
     //   treeWidget->setColumnCount(2);//列の数をセット
        for (int i = 0; i < brandFolder.size(); ++i){
            //list = td.getFavorite(i);
            AddRoot("", brandFolder.at(i), list);
        }
    }
}

void MainWindow::treeItemExpanded(QTreeWidgetItem *item)
{
    //int pos = brandFolder.indexOf(item->text(1));
    //qDebug() << item->childCount() << item->text(1) << pos <<brandFolder.count();

    if(item->childCount() == 0){
        QMap<int, QString> list;
        QString market;
        if(item->text(1) == QString::fromLocal8Bit("指数"))
            market = "I";
        else if (item->text(1) == QString::fromLocal8Bit("東証1部"))
            market = "T1";
        else if (item->text(1) == QString::fromLocal8Bit("東証2部"))
            market = "T2";
        else if (item->text(1) == QString::fromLocal8Bit("マザーズ"))
            market = "M";
        else if (item->text(1) == QString::fromLocal8Bit("JQスタンダード"))
            market = "JS";
        else if (item->text(1) == QString::fromLocal8Bit("JQグロース"))
            market = "JG";
        else if (item->text(1) == QString::fromLocal8Bit("札証"))
            market = "S";
        else if (item->text(1) == QString::fromLocal8Bit("福証"))
            market = "F";
        QSqlQuery query=QSqlQuery(td.db);
            query.exec(tr("select code, name, market from brands where marketsymbol = '%1'").arg(market));
            while(query.next()){
                list.insert(query.value(0).toInt(), query.value(1).toString());
            }

        if(list.count() > 0){
            QMap<int, QString>::const_iterator it = list.constBegin();
            while (it != list.constEnd()) {
                QString str;
                AddChild(item, str.setNum(it.key()), it.value());
                //qDebug() << it.key() << it.value();
                ++it;
            }
        }
    }
}


void MainWindow::treeCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    //qDebug() << current->text(0); 0:""はfolder
    int code = current->text(0).toInt();
    if(code > 100 && code < 100000){
        changeBrand(code);
        tbCode->setText("");
    }
}

void MainWindow::treeContextMenuRequested(const QPoint &pos)
{
    qDebug() << "context";
    QMenu *menu = new QMenu;
    //QModelIndex index = this->currentIndex();
    //QString fileName = this->model()->data(this->model()->index(index.row(), 0),0).toString();
    menu->addAction(QString::fromLocal8Bit("お気に入りに追加"), this, SLOT(addItem()));
    menu->addAction(tr("remove Item"), this, SLOT(removeItem()));
    menu->addAction(QString::fromLocal8Bit("Folda追加"), this ,SLOT(addFolder()));
    //menu->addAction(tr("remove Folda"), this, SLOT(removeFolder()));
    menu->addAction(QString::fromLocal8Bit("Folda削除"), this, SLOT(deleteActiveTreeNode()));
    menu->exec(QCursor::pos());
    menu->exec(pos);
    menu->exec(mapToGlobal(pos));
}

void MainWindow::addItem(){
    QString str;
    QTreeWidgetItem *itm = new QTreeWidgetItem(); //
    itm->setText(0,str.setNum(td.info.code));
    itm->setText(1, td.info.name);
    itm->setTextAlignment(1,Qt::AlignLeft);
    treeWidget->topLevelItem(0)->addChild(itm);

}
void MainWindow::removeItem(){

    QTreeWidgetItem *item = treeWidget->currentItem();
    if(item)
        delete item->parent()->takeChild(item->parent()->indexOfChild(item));

}


void MainWindow::deleteActiveTreeNode(){
    QTreeWidgetItem *item = treeWidget->currentItem();
    if(item)
        delete item->parent()->takeChild(item->parent()->indexOfChild(item));
}

void MainWindow::addFolder(){
    bool ok;
    QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
               tr("Folder name:"), QLineEdit::Normal, QDir::home().dirName(), &ok);
    if (ok && !text.isEmpty()){

        QTreeWidgetItem *itm = new QTreeWidgetItem(treeWidget);
        itm->setText(0,"");
        itm->setText(1, text);
        itm->setChildIndicatorPolicy( QTreeWidgetItem::ShowIndicator );

        treeWidget->addTopLevelItem(itm);
    }
}

// ↑ TreeWidget

bool MainWindow::changeBrand(int code){
    int ret = td.load(code, comboAshi->currentIndex());
    if(ret > 0){
        status->showMessage(QString::fromLocal8Bit("銘柄は見つかりません"), 30000);
        return false;
    }
    else if(ret < 0){
        QMessageBox msgBox;
        msgBox.setText(QString::fromLocal8Bit("日付が認識できません\n列数（信用残含むか）を確認してください。"));
        msgBox.exec();
        return false;
    }
    if(td.list_date[0].count() < 20)    //日足２０本以下の場合は１年分をダウンロード
        downloadKdb(2);
    if(td.list_date[comboAshi->currentIndex()].count() > 0){
        view->updateChart();
    }
    else{
        QMessageBox msgBox;
        msgBox.setText(QString::fromLocal8Bit("%1には有効データがありません。").arg(code));
        msgBox.exec();
        return false;
    }
    return true;
}


void MainWindow::keyPressEvent(QKeyEvent *event){
    qDebug() << "main window keypress:" << event->key();
    if (event->modifiers()==Qt::ShiftModifier){
        qDebug()<<"shift and one" << event->text();

    }
    else if(event->modifiers()==Qt::ControlModifier){
        //qDebug() << "command(ctr)" << event->text() << event->key();

        if(event->text() == "k"){
            QMessageBox msgBox;
            msgBox.setText("download 1year");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            if(msgBox.exec() == QMessageBox::Yes)
                downloadKdb(2);

        }
        else if(event->key() == 73){
            //mac command+i
            qDebug() << "ctr+i";
            QString filename;
            if(!QFile(filename).exists())
                filename = tr("%1/%2").arg(tbDatadir->text()).arg("index.txt");
            //QFile file;
            //file.setFileName();
            tbIndexfile->setText(filename);
            td.setIndextext(filename);
            qDebug() << td.getIndextext();
            //return;
/*
#ifdef INDEXFILE_H
         QMessageBox msgBox;
        msgBox.setText(tr("download index file to %1").arg(td.getIndextext()));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if(msgBox.exec() == QMessageBox::Yes)
            downloadIndexfile(filename);
#endif
*/
            if(td.ReadIndexToDb()){
                status->showMessage(QString::fromLocal8Bit("index.txtを読み込みました"), 3000);
                if(td.info.code > 1300){
                    td.load(td.info.code, comboAshi->currentIndex());
                    view->updateChart();
                }
            }

        }
        else if (event->key() == 82){
            QMessageBox msgBox;
            msgBox.setText("access web");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            if(msgBox.exec() == QMessageBox::Yes){
                QRegExp rx("<tr.*><td.*>([0-9]{1,2})</td><td.*><a.*>([0-9]{4})</a></td><td.*>(.*)</td><td.*>(.*)</td>");
                rx.setMinimal(true);
                QMap<int, QString> list;
                for(int i = 0; i < 3; i++){
                    QString source = getSource(tr("http://info.finance.yahoo.co.jp/ranking/?kd=1&tm=d&vl=a&mk=1&p=%1").arg(i + 1));
                    //qDebug() << source;
                    int pos = 0;
                    while ((pos = rx.indexIn(source, pos)) != -1) {
                        int code = rx.cap(2).toInt();
                        list.insert(code, rx.cap(4) + rx.cap(3));
                        //qDebug() << rx.cap(1) << rx.cap(2) << rx.cap(3) <<rx.cap(4);
                        pos += rx.matchedLength();
                    }
                    status->showMessage(tr("page %1 readed").arg(i + 1), 1000);
                }
                    AddRoot("", QString::fromLocal8Bit("値上り"), list);
                    list.clear();
                brandFolder.append(QString::fromLocal8Bit("値上り"));
                status->showMessage("finished", 1000);
            }
        }
    }
    else if(event->modifiers()==Qt::AltModifier){
        if(event->key() == 73){
        }
    }
    else{
        if(event->key() == 32){
            QTreeWidgetItem* item = treeWidget->currentItem();
            //int pos = treeWidget->currentIndex().row();
            //int index = item->parent()->indexOfChild(item);
           // treeWidget->setTreePosition(index + 1);
            treeWidget->setCurrentItem(treeWidget->itemBelow(item));
            //treeWidget->itemBelow(item)->setSelected(true);
            treeWidget->setFocus();
        }
        else if(event->key() == 65)
            addItem();
        else if(16777234)
            treeWidget->collapseItem(treeWidget->currentItem()->parent());
    }
    QMainWindow::keyPressEvent(event);
}


void MainWindow::backgroundFinished(){
    //qDebug() << QThread::currentThreadId() << "backgroundFinished()";
     status->showMessage(QString::fromLocal8Bit("データ更新完了しました"), 30000);
     if(isShowMessage){
         QMessageBox msgBox;
         msgBox.setText(QString::fromLocal8Bit("更新終了しました"));
         msgBox.exec();
     }
     isBackgroundRunning = false;
}
void MainWindow::backgroundRecieveMessage(QString message){
    qDebug() << "message recieved" << message;
    status->showMessage(message, 5000);
    if(message.contains("error")){
        QMessageBox msgBox;
        msgBox.setText(QString::fromLocal8Bit("エラー\n") + message);
        msgBox.exec();
    }
    else if(message.contains(QString::fromLocal8Bit("日経平均を更新しました"))){
        changeBrand(101);
        status->showMessage(QString::fromLocal8Bit("ダウンロード中"));
    }

}

void MainWindow::downloadKdb(int type, int max){
    if(type < 2){  //new date
        isShowMessage = false;

        quint32 last = getLastDate(101, tbDatadir->text(), td.isCredit());
        QDateTime now = QDateTime::currentDateTime();
        QDateTime now_offset = now.addSecs(- 60 * 60 * 16);
        quint32 idate = now_offset.date().year() * 10000 + now_offset.date().month() * 100 + now_offset.date().day();
        int offset = idate - last;
        if(type == 1 && offset == 0){
            qDebug() << "last:" << last << "target:" << idate << "offset:" << offset;
            status->showMessage(QString::fromLocal8Bit("新規データはないと思われます。\n16時以降に再度実行してください。"), 30000);
            return;
        }
        if(offset > 4 || max < 0)
            isShowMessage = true;
        qDebug() << "last:" << last << "target:" << idate << "offset:" << offset;
        //return;
        status->showMessage("background start", 1000);
        backgroundThread *back = new backgroundThread;
        if(max > 0)
            back->setMaxdownload(max);
        back->set(&td, tbDatadir->text(), td.isCredit());
        connect(back, &backgroundThread::finishThread, this, &MainWindow::backgroundFinished); // Qt::AutoConnection
        connect(back, &backgroundThread::finished, back, &backgroundThread::deleteLater);
        connect(back, &backgroundThread::sendMessage, this, &MainWindow::backgroundRecieveMessage );
        isBackgroundRunning = true;
        back->start();

    }
    else if(type == 2){     // brand 1year
        status->showMessage(QString::fromLocal8Bit("k-dbにアクセスしています download 1 year data"), 5000);
        //qDebug() << td.info.code << td.info.market << td.info.marketsymbol;
        //return;
        downloadBrand(&td, td.info.code, tbDatadir->text(), td.isCredit());
        td.load(td.info.code, comboAshi->currentIndex());
        view->updateChart();
        status->showMessage(QString::fromLocal8Bit("更新しました"), 1000);
    }
    else if(type == 3){
        status->showMessage(QString::fromLocal8Bit("k-dbにアクセスしています"), 5000);
        int code = td.info.code;
        downloadBrand(&td, td.info.code, tbDatadir->text(), td.isCredit(), true);
        td.load(code, comboAshi->currentIndex());
        tbMonth->setText(tr("%1").arg(36));
        view->updateChart();
        status->showMessage(QString::fromLocal8Bit("更新しました"), 1000);
    }
}
