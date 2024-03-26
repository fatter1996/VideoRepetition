#ifndef SERVERSIDE_H
#define SERVERSIDE_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QAbstractSocket>


class ServerSide : public QObject
{
     Q_OBJECT
public:
    ServerSide(QObject* parent = nullptr);
    ~ServerSide();

private:
    QByteArray HeartBeat();
    QByteArray Receipt();
    void Unpacking(QByteArray array, QTcpSocket* m_pSocket);

public slots:
    void onConnection();
    void onError(QAbstractSocket::SocketError);

signals:
    void Receive(QByteArray);
    void NewStream(QByteArray);

private:
    QTcpServer* m_pServer = nullptr;
    QVector<QTcpSocket*> m_pSocketList;

    //QFile* log = nullptr;

};

#endif // SERVERSIDE_H
