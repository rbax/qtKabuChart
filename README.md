# qtKabuChart
Chart program for japanese stocks only
## Description
This source code is Test alpha version of Qt widget application with Qt5.  
Using this program, you can display candlestick, line chart,Ichimoku,Heikin Ashi.  
If your computer(OsX or Ubuntu) is installed TA-Lib(technical analysis library for c++) , 
you can display Moving Average, Bollinger Band, Parabolic,MACD,RSI,Momentum and ATR.  

Ｑｔ5.5で作成したc++株チャート表示プログラムソースです。  
OsX(El Capitan)とUbuntu14.04環境下でデバッグしています。  
テストαバージョンで、重大なバグが隠れている可能性があります。  
日本株限定で、株価データをk-db.comから取得してチャート表示をします。  

## Caution
I am a beginner for qt and c++.
There is a possibility of serious trouble occurs.  

## Demo
![](https://github.com/narih/qtKabuChart/wiki/images/ceres.png)

## Requirement
- Qt5  
- Ta-Lib(http://ta-lib.org/) open-source for c/c++  
Ta-Libがない場合でもエラーは出ないはずですが、大部分のテクニカル指標が表示できません。  

## Usage
Opne "qtKabuChart.pro" from Qt Creator.  
Select hint.qrc of resource folda and run "add prefix" by ContextMenu.
Set "/" to Prefix TextBox and addfile "usage.html" to prefix folda.

Then, you can run this project.

Qt CreatorでqtKabuChart.proを開きます。
usage.htmlをプログラム上で表示できるようにするために、hint.qrcにprefix "/" と
usage.htmlを追加してください（下図参照）。
これでプロジェクトを実行してください。

初回実行時にデータフォルダを設定するためのフォルダ選択ダイアログが表示されます。
その後、k-db.comにアクセスして、日経平均の１年分データと最新日付の指標と個別銘柄の１日分データがダウンロードされます。
個別銘柄のダウンロードに成功するとツリーウィジェットの銘柄グループの展開ができるようになり、銘柄の変更ができます。
日経平均以外の指標と個別銘柄はデータ数が１０本以下の場合は表示直前に１年分データを随時ダウンロードして更新します。





## License
MIT see LICENCE
