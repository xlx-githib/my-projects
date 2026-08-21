#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QTcpSocket>
#include <QMenu>
class EmojiPicker;
namespace Ui {
class ChatWindow;
}

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWindow(QWidget *parent = 0);
    ~ChatWindow();
    void setUsername(const QString &name);
private slots:
    void btn_send_clicked();
    void tcp_ready_read();
    void tcp_disconnected();
    void showAvatarBig();
    void onSendImage();       
    void onSendFile(); 

private:
    Ui::ChatWindow *ui;
    QTcpSocket *my_socket;
    QString username;
    const QString SERVER_IP = "127.0.0.1";
    const quint16 SERVER_PORT = 4399;
    QMenu       *toolMenu;       
    EmojiPicker *emojiPicker;   

    void setupToolButton();     
protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // CHATWINDOW_H
