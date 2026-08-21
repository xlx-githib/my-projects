#include "chatserver.h"

#include <QDebug>
#include <QHostAddress>

ChatServer::ChatServer(QWidget *parent)
    : QMainWindow(parent)
{
    my_server = new QTcpServer(this);
    if(my_server->listen(QHostAddress::Any, 4399)){
       qDebug() << "服务器成功启功" << endl;
    }else{
        qDebug() << "服务器启功失败" << endl;
    }
    connect(my_server, &QTcpServer::newConnection, this, &ChatServer::onNewConnection);
}

ChatServer::~ChatServer()
{

}

void ChatServer::onNewConnection()
{
    QTcpSocket *new_client = my_server->QTcpServer::nextPendingConnection();
    clientlist.insert(new_client, "");
    qDebug() << "新用户进入，等待发送用户名" << endl;
    connect(new_client, &QTcpSocket::readyRead, this, &ChatServer::onReadyRead);
    connect(new_client, &QTcpSocket::disconnected, this, &ChatServer::onDisconnected);
}

void ChatServer::onDisconnected()
{
    QTcpSocket *current_client = qobject_cast<QTcpSocket *>(sender());
    if(!current_client)return  ;
    QString name = clientlist.value(current_client);
    clientlist.remove(current_client);
    current_client->deleteLater();
    qDebug() << name << "离开聊天室" << endl;
    broadcastUserList();
}

void ChatServer::onReadyRead()
{
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if(!senderSocket) return;

    QString data = QString::fromUtf8(senderSocket->readAll());
    qDebug() << "收到数据：" << data;

    QStringList parts = data.split(":");
    if(parts.size() < 2) return;

    QString cmd = parts.at(0);

    if(cmd == "LOGIN"){
        QString username = parts.at(1);
        clientlist[senderSocket] = username;
        qDebug() << username << "进入了聊天室";
        broadcastUserList();
    }
    else if(cmd == "MSG"){
        QString content = parts.mid(1).join(":");
        QString senderName = clientlist.value(senderSocket);
        QString chatData = QString("CHAT:%1:%2").arg(senderName).arg(content);
        for(QTcpSocket *client : clientlist.keys()){
            client->write(chatData.toUtf8());
        }
    }
}

void ChatServer::broadcastUserList()
{
    QStringList userlist;
    for(QString client : clientlist.values()){
        if(!client.isEmpty())userlist.append(client);
    }

    QString data = "USERLIST:" + userlist.join(",");
    for(QTcpSocket *client : clientlist.keys()){
        client->write(data.toUtf8());
    }

}

void ChatServer::closeEvent(QCloseEvent *event)
{
    for(QTcpSocket *client : clientlist.keys()){
        client->disconnectFromHost();
        if(client->state() != QAbstractSocket::UnconnectedState){
            client->waitForDisconnected(1000);
        }
    }
    my_server->close();
    event->accept();
}
