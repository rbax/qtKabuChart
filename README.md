# qtKabuChart
Chart program for japanese stocks only
## Description
This source code is Test alpha version of Qt widget application created by open-source Qt5.5.0.  
Using this program, you can display candlestick chart of japanese stocks.  

マックとUnix環境下で個人的に株チャートを表示する、評価版プログラムです。  
open-source版のQt5.5で作成したc++プログラムソースです。  
OsX(El Capitan)とUbuntu14.04環境下でデバッグしています。  
テストαバージョンで、重大なバグが隠れている可能性があります。  
日本株限定で、株価データを[http://k-db.com](http://k-db.com)から取得してチャート表示をします。  

## Caution and Limitation
I am a beginner for qt and c++.  
There is a possibility of serious trouble occurs.  
This program is for japanese stocks only.

データ取得元k-db.comのアクセス制限によりデータ取得ができない場合があります。

## Demo
<<<<<<< HEAD
![](https://github.com/narih/qtKabuChart/wiki/images/ceres.png)
=======
https://github.com/narih/qtKabuChart/wiki/images/chart.png
>>>>>>> parent of e64ac95... Update README.md

## Requirement
- [Open source Qt5.x](https://www.qt.io/download-open-source/  "ダウンロードページ") 
- [Ta-Lib](http://ta-lib.org/) open-source for c/c++  
Even if Ta-Lib library is not installed, I think error will not be occur.  
But, technical indicators not visible like MA.  
Ta-Libがない場合でもエラーは出ないはずですが、大部分のテクニカル指標が表示できません。  

##Feature
- Show candlestick or line chart
- Change brand by selecting tree node (or input code from keyboard)  
- Change day,week,month time-series by combobox
- Change display period by editing month textbox
- Toggle display volume, price information, date select scale line
- Display indicator Ichimoku, Heikin ashi
- With Ta-Lib, you can display MA(x4), Bollinger band, Parabolic
- With Ta-Lib, you can display oscillator MACD, RSI, Momentum, ATR
- Change colors of chart elements by color dialog

## Usage
Qt CreatorでqtKabuChart.proを開きます。  
初回実行時にデータフォルダを設定するためのフォルダ選択ダイアログが表示されます。  
指定したデータフォルダに1000件以上の銘柄個別ファイルが作成されますので注意してください。  
フォルダに0101ファイル（日経平均）が存在すればこれを読み込んで表示します。  
存在しなければ、k-db.comにアクセスして、まず日経平均の１年分データを取得し、
0101ファイルを作成してチャートが表示されます。  
引き続き指標と個別銘柄の最新日付１日分データがダウンロードされます。  
個別銘柄のダウンロードに成功するとツリーウィジェットの銘柄グループの展開ができるようになり、銘柄の変更ができます。  
日経平均以外の指標と個別銘柄はデータ数が１０本以下の場合は表示直前に１年分データを随時ダウンロードして更新します。  

usage.htmlに使い方などについての説明を記載しましたので参照してください。

####usage.htmlを「hint」タブページで参照する方法
Qt Creatorのプロジェクトタブのリソースフォルダに「hint.rc」を見つけてコンテキストメニューを表示します。
usage.htmlをプログラム上で表示できるようにするために、hint.qrcにprefix "/" と
usage.htmlを追加してください（下図参照）。
![](https://github.com/narih/qtKabuChart/wiki/images/add_resource.png)
これでプロジェクトを実行すると、「hint」タブページでhtmlファイルが参照できると思います。


## Known Issues
- I don't know how to include the Ta-Lib library under Windows environment.  
UbuntuのVirtualBox（Windows10）のQt5.6.0MSVC2013 64bit環境で、プロジェクトに  
SOURCES += fortalib.cpp  
HEADERS  += fortalib.h  
LIBS += c:/ta-lib/c/lib/ta_libc_cdd.lib  
INCLUDEPATH += c:/ta-lib/c/include  
を追加しても、Ta-Lib内の関数見つからないとのエラーが発生してビルドできませんでした。  
解決策わかる人がいれば教えてください。
- I can not resize the QDockWidget by reading saved Settings.  
終了時QSettingsにQDockWidgetのサイズを保存しているが、初期化で反映されません。
- There is a case that I can not display the chart.  
銘柄変更を繰り返しているうちにチャート表示部のみ白くなり、以降終了までチャートが表示されないことがあります。  
以前は、QGraphicsViewのQGraphicsSceneを銘柄変更時にclear()していました。  
この時は、数十銘柄の表示でチャートが表示できなくなってしまうことが多発しました。  
Macのアクティビティモニタでプロセスのメモリ推移を監視してみましたが、大きな変動はなさそうでした。  
現在は、QGraphicsViewでQGraphicsSceneにQGraphicsItemをchildとして追加し、銘柄変更時にこれらをクリアする仕様にしています。  
これで、以前のようにチャート表示ができなくなることはなくなったかもしれません。  

## License
MIT see LICENCE
