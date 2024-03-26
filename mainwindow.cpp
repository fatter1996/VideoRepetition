#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QFile>
#include <windows.h>
#include <winnetwk.h>
#include <QtConcurrent>
#pragma comment(lib, "Mpr.lib")
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //去掉标题栏
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    //配置文件初始化
    GlobalData::m_pConfigInfo = new ConfigInfo;
    GlobalData::m_pConfigInfo->init();
    //设置显示位置与大小
    setFixedSize(GlobalData::m_pConfigInfo->width, GlobalData::m_pConfigInfo->height);
    move(GlobalData::m_pConfigInfo->pos_x, GlobalData::m_pConfigInfo->pos_y);
    display = new TextDisplay(this);
    ui->setupUi(this);

    textAnalysis = new TextAnalysis;
    tcpServer  = new ServerSide(this);
    //QThread* thread = new QThread;
    //tcpServer->moveToThread(thread);
    //thread->start();
    connect(tcpServer, &ServerSide::Receive, this, &MainWindow::onRecive);
    //TCPThread = new QThread;
    //tcpSocket->moveToThread(TCPThread);
    NETRESOURCE nr;
    nr.dwType = RESOURCETYPE_ANY;
    nr.lpLocalName = NULL;
    nr.lpRemoteName = (LPWSTR)GlobalData::m_pConfigInfo->sreverIP.data();
    nr.lpProvider = NULL;

    WNetAddConnection2(&nr, (LPWSTR)GlobalData::m_pConfigInfo->password.data(),
                       (LPWSTR)GlobalData::m_pConfigInfo->userName.data(), CONNECT_UPDATE_PROFILE);



#ifdef MODE_ONLY
    ui->comboBox->hide();
    PullFlowStart(GlobalData::m_pConfigInfo->url);
    //QtConcurrent::run(this, &MainWindow::PullFlowStart, GlobalData::m_pConfigInfo->url);
#elif defined MODE_MORE
    ui->comboBox->show();
    connect(tcpSocket, &ServerSide::NewStream, this, &MainWindow::onNewStream);
#endif
}

MainWindow::~MainWindow()
{
    if(m_pPull)
        m_pPull->PullFlowStop();
    m_pPull->quit();
    if(record)
        onRecordEnd();
    delete tcpServer;
    delete textAnalysis;
    delete m_pPull;
    delete ui;
}

void MainWindow::PullFlowStart(QString url)
{
    m_pPull = new PullFlow(url, this);
    connect(m_pPull, &PullFlow::Error, this, &MainWindow::onPullFlowError);
    m_pPull->Init((void*)ui->videoWidget->winId());
    m_pPull->start();
}

void MainWindow::onRecive(QByteArray reciveArray)
{
    if((unsigned char)reciveArray.at(9) == 0xaa)
    {
        TextFrame* data = textAnalysis->TextFrameUnpack(reciveArray);
        display->setFixedSize(12 * data->text.length() + 32, 48);

        display->move(this->geometry().x() + data->point.x(), this->geometry().y() + data->point.y());
        //display->move(data->point.x(), data->point.y());
        display->Display(data);
        qDebug() << QString::fromLocal8Bit(data->text);
        display->show();

        delete data;
    }
    else if((unsigned char)reciveArray.at(9) == 0xa5) //开始录屏
    {
        if(isRecording)
            return;
        isRecording = true;
        record = new FileRecord;
        if(record->Init() >= 0)
        {
            connect(record, &FileRecord::RecordEnd, this, &MainWindow::onRecordEnd);
            record->start();
        }
    }
    else if((unsigned char)reciveArray.at(9) == 0x5a) //结束录屏
    {
        onRecordEnd();
    }
    else if((unsigned char)reciveArray.at(9) == 0xf0) //车站信息
    {
        if(station)
        {
            delete station;
            station = nullptr;
        }
        station = textAnalysis->StationFrameUnpack(reciveArray);
    }
}

void MainWindow::timerEvent(QTimerEvent * ev)
{
    if(ev->timerId() == timeId)
    {
        if(lastLabel)
        {
            delete lastLabel;
            lastLabel = nullptr;
        }
    }
}

void MainWindow::onNewStream(QByteArray reciveArray)
{
    StreamFrame* data = textAnalysis->StreamFrameUnpack(reciveArray);
    for(int i = 0; i < urlList.size(); i++)
    {
        if(urlList.at(i) == data->url)
            return;
    }
    ui->comboBox->addItem(data->name);
    urlList.append(data->url);
    if(urlList.size() == 1)
    {
        nowUrl = data->url;
        PullFlowStart(data->url);
    }
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    m_pPull->PullFlowStop();
    delete m_pPull;
    m_pPull = nullptr;
    nowUrl = urlList.at(index);
    PullFlowStart(nowUrl);
}

void MainWindow::onPullFlowError()
{
    //QMessageBox::information(nullptr, QString("提示"), QString("数据库连接失败，部分功能会受影响！"));
    m_pPull->PullFlowStop();
    m_pPull->quit();
#ifdef MODE_ONLY
    //PullFlowStart(GlobalData::m_pConfigInfo->url);
    QtConcurrent::run(this, &MainWindow::PullFlowStart, GlobalData::m_pConfigInfo->url);
#elif defined MODE_MORE
    PullFlowStart(nowUrl);
#endif
}

void MainWindow::onRecordEnd()
{
    if(record && isRecording)
    {
        QString path = record->RecordStop();
        record->wait(100);
        delete record;
        record = nullptr;

        QtConcurrent::run(this, &MainWindow::VideoRemuxing, path);
        //VideoRemuxing(path);
        isRecording = false;
    }
}

void MainWindow::VideoRemuxing(QString filePath)
{
    QString outPath = QCoreApplication::applicationDirPath()
            + QString("/video/Record%1.mp4").arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"));
    QFile out(outPath);
    if(out.exists()) //如果文件已存在重新命名
    {
        outPath.replace(".mp4", "(2).mp4");
    }

    QString savePath = GlobalData::m_pConfigInfo->sreverIP;
    savePath.append(GlobalData::m_pConfigInfo->savePath);

    if(station)
    {
        savePath.append(QString("%1").arg(QString::fromLocal8Bit(station->loginName)));
        QDir dir(savePath);
        if(!dir.exists())
        {
            dir.mkdir(savePath);
        }

        savePath.append(QString("/%1-%2-%3.mp4")
                        .arg(QString::fromLocal8Bit(station->name))
                        .arg(QString::fromLocal8Bit(station->loginName))
                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")));
    }
    else
    {
        savePath.append(QString("%1.mp4").arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")));
    }


    //std::wstring cmd = QString(" -i %1 %2").arg(filePath).arg(outPath).toStdWString();
    //std::wstring strexeContent  = TEXT("ffmpeg.exe ");
    outPath.replace('/', '\\');
    savePath.replace('/', '\\');
    std::wstring cmd = QString(" %1 \"%2\" \"%3\"").arg(filePath).arg(outPath).arg(savePath).toStdWString();
    std::wstring strexeContent  = TEXT("remuxing.bat");
    qDebug() << filePath << "\n" << outPath << "\n" << savePath;
    qDebug() << cmd;
    SHELLEXECUTEINFO ShExecInfo = { 0 };
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = strexeContent.c_str();
    ShExecInfo.lpParameters = cmd.c_str();
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ShellExecuteEx(&ShExecInfo);
    ////关闭该exe程序
    //if( ShExecInfo.hProcess != NULL)
    //{
    //    //等待程序运行完毕
    //    WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
    //    //关闭程序
    //    TerminateProcess(ShExecInfo.hProcess,0);
    //    ShExecInfo.hProcess = NULL;
    //}
    //
    ////保存到共享文件夹
    //if(!QFile::copy(outPath, savePath))
    //{
    //    qDebug() << "File copy to server faild.";
    //}
    //else
    //{
    //    qDebug() << "File copy to server success.";
    //}
    //
    ////删除临时文件
    //QFile video(filePath);
    //video.remove();
    //out.remove();

}



