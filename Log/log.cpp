#include "log.h"
#include <QDir>


using namespace QsLogging;

QMyLog::QMyLog()
{

}

QMyLog::~QMyLog()
{
    Logger::destroyInstance();
}

void QMyLog::initLogger()
{
    // 1. 启动日志记录机制
    Logger& logger = Logger::instance();
    logger.setLoggingLevel(QsLogging::TraceLevel);
    //设置log所在目录
    const QString sLogPath(LOG_FILE_NAME);
    //检查目录或创建目录-LWM
    QString fullPath = LOG_FILE_PATH;
    QDir dir(fullPath);
    if(!dir.exists())
    {
        dir.mkpath(fullPath);
    }

    // 2. 添加两个destination
    DestinationPtr fileDestination(DestinationFactory::MakeFileDestination(
      sLogPath, EnableLogRotation, MaxSizeBytes(512), MaxOldLogCount(2)));
    DestinationPtr debugDestination(DestinationFactory::MakeDebugOutputDestination());
    //DestinationPtr functorDestination(DestinationFactory::MakeFunctorDestination(&logFunction));

    //这样和槽函数连接
    DestinationPtr sigsSlotDestination(DestinationFactory::MakeFunctorDestination(this, SLOT(logSlot(QString,int))));
    //DestinationPtr sigsSlotDestination(DestinationFactory::MakeFunctorDestination(this, SLOT(&QMyLog::logSlot(QString,int))));
    logger.addDestination(debugDestination);
    logger.addDestination(fileDestination);
    //logger.addDestination(functorDestination);
    logger.addDestination(sigsSlotDestination);

    // 3. 开始日志记录
    //日志中记录
    QLOG_INFO() << "Program started";
}

void QMyLog::logSlot(const QString &message, int level)
{
    Q_UNUSED (level)
    QLOG_INFO() << qUtf8Printable(message);
}
