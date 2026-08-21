#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QDebug>
#include <QMap>

class ChatServer : public QMainWindow
{
    Q_OBJECT

public:
    ChatServer(QWidget *parent = 0);
    ~ChatServer();
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    QTcpServer *my_server;
    QMap<QTcpSocket *, QString> clientlist;

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void broadcastUserList();
};

#endif // CHATSERVER_H
