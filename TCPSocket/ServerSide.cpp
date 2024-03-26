#include "ServerSide.h"
#include <QDebug>
#include <QAbstractSocket>
#include "Universal.h"
#pragma execution_character_set("utf-8")

ServerSide::ServerSide(QObject* parent)
{
    Q_UNUSED (parent)
    //log = new QFile("TcpLog.txt");
    //log->open(QIODevice::WriteOnly | QIODevice::Append);
    m_pServer = new QTcpServer;
    connect(m_pServer, &QTcpServer::newConnection, this, &ServerSide::onConnection);
    m_pServer->listen(QHostAddress(GlobalData::m_pConfigInfo->localIp), GlobalData::m_pConfigInfo->localPORT); //绑定本地IP和端口
}

ServerSide::~ServerSide()
{
    if(m_pServer)
    {
        m_pServer->close();
        m_pServer = nullptr;
    }
    //log->close();
}

void ServerSide::onConnection()
{
    //取出连接好的套接字
    QTcpSocket* m_pSocket = m_pServer->nextPendingConnection();

    //获得通信套接字的控制信息
    QString ip = m_pSocket->peerAddress().toString();//获取连接的 ip地址
    quint16 port = m_pSocket->peerPort();//获取连接的 端口号
    m_pSocketList.append(m_pSocket);
    qDebug() << QString("客户端连接成功: %1:%2").arg(ip).arg(port);
    //log->write("\r\n--- onConnection ---\r\n");
    //log->write(QString("客户端连接成功: %1:%2").arg(ip).arg(port).toStdString().c_str());

    //数据接收槽函数
    connect(m_pSocket, &QTcpSocket::readyRead, this, [=](){
        QByteArray receiveStr = m_pSocket->readAll();
        qDebug() << "--- Receive ---";
        qDebug() << receiveStr.toHex();
        //log->write("\r\n--- Receive First ---\r\n");
        //log->write(receiveStr.toHex());
        Unpacking(receiveStr, m_pSocket);
    });

    connect(m_pSocket, &QTcpSocket::disconnected, this, [=]()
    {
        qDebug() << QString("客户端连接断开: %1:%2").arg(ip).arg(port);
    });
}

void ServerSide::Unpacking(QByteArray array, QTcpSocket* m_pSocket)
{
    if(array.length() <= 10)
        return;
    //长度校验
    int lenght = GlobalData::getBigEndian_2Bit(array.mid(4,2));
    if(array.length() < lenght)
            return;

    if(array.length() > lenght) //有粘包,需特殊处理
    {
        QByteArray arr = array.mid(lenght);
        array = array.left(lenght);
        Unpacking(arr, m_pSocket);
    }

    if((unsigned char)array.at(10) == 0xb5 && array.length() == 15) //心跳包,收到后自动回复
    {
        m_pSocket->write(HeartBeat());
        //qDebug() << "--- Receive ---";
        //qDebug() << receiveStr.toHex();
    }
    else if((unsigned char)array.at(9) == 0x80) //新的视频流
    {
        //发送回执
        m_pSocket->write(Receipt());
        emit NewStream(array);
        //qDebug() << "--- Receive ---";
        //qDebug() << receiveStr.toHex();
    }
    else
    {
        //log->write("\r\n--- Receive ---\r\n");
        //log->write(array.toHex());

        //qDebug() << "--- Receive ---";
        //qDebug() << receiveStr.toHex();
        emit Receive(array);
    }

}

QByteArray ServerSide::HeartBeat()
{
    QByteArray heart;
    heart.append(QByteArray::fromHex("efefefef"));
    heart.append(QByteArray::fromHex("0f00"));
    heart.append(QByteArray::fromHex("0000"));
    heart.append(QByteArray::fromHex("ee"));
    heart.append(QByteArray::fromHex("00"));
    heart.append(QByteArray::fromHex("5b"));
    heart.append(QByteArray::fromHex("fefefefe"));
    //qDebug() << "--- Send ---";
    //qDebug() << heart.toHex();
    return heart;
}

QByteArray ServerSide::Receipt()
{
    QByteArray heart;
    heart.append(QByteArray::fromHex("efefefef"));
    heart.append(QByteArray::fromHex("0e00"));
    heart.append(QByteArray::fromHex("0000"));
    heart.append(QByteArray::fromHex("ee"));
    heart.append(QByteArray::fromHex("8f"));
    heart.append(QByteArray::fromHex("fefefefe"));
    //qDebug() << "--- Send ---";
    //qDebug() << heart.toHex();
    return heart;
}

void ServerSide::onError(QAbstractSocket::SocketError error)
{


}
