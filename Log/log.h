#pragma once
#ifndef LOG_H
#define LOG_H

#include <QTime>
#include <QString>
#include <QDir>
#include "QsLog/QsLog.h"
#include "QsLog/QsLogDest.h"

//每次启动则生成一个带时间戳的日志文件
#define LOG_FILE_PATH QString("Logs")
#define LOG_FILE_NAME QString("Logs/Log_") + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + QString(".log")
//每次启动则生成或使用同一个日志文件
//#define LOG_FILE_NAME QString("Log/") + QString("Log.log")

class QMyLog : public QObject
{
public:
    QMyLog();
    ~QMyLog();

    void initLogger();
    void destroyLogger();

public slots:
    void logSlot(const QString &message, int level);
};

#endif // LOG_H
