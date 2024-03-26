#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "PullFlow/PullFlow.h"
#include "TCPSocket/ServerSide.h"
#include "TextAnalysis/TextAnalysis.h"
#include "PullFlow/AudioPlay.h"
#include "PullFlow/VideoPlay.h"
#include "FileRecord/FileRecord.h"
#include "TextDisplay.h"

#pragma execution_character_set("utf-8")


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void PullFlowStart(QString url);
    void timerEvent(QTimerEvent * ev);
private slots:
    void onRecive(QByteArray);
    void onNewStream(QByteArray);
    void onPullFlowError();
    void onRecordEnd();
    void VideoRemuxing(QString filePath);

    void on_comboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;

    PullFlow* m_pPull = nullptr;
    AudioPlay* audio = nullptr;
    VideoPlay* video = nullptr;
    ServerSide* tcpServer  = nullptr;
    TextAnalysis* textAnalysis = nullptr;

    FileRecord* record = nullptr;
    StationFrame* station = nullptr;

    bool isRecording = false;

    QVector<QString> urlList;
    QString nowUrl;

    QLabel* lastLabel = nullptr;
    TextDisplay* display = nullptr;
    int timeId = 0;

};
#endif // MAINWINDOW_H
