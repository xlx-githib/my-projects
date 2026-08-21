#include "chatwindow.h"
#include "ui_chatwindow.h"
#include "avatarlabel.h"
#include "emojipicker.h"
#include <QMessageBox>
#include <QDateTime>
#include <QPixmap>
#include <QWidget>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>   
#include <QDir>          
ChatWindow::ChatWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatWindow)
{
    ui->setupUi(this);
    ui->textBrowser_chat->setOpenLinks(false);
    ui->textBrowser_chat->setStyleSheet(
        "QTextBrowser {"
        "  background-image: url(:/images/OnePiece.png);"
        "  background-repeat: no-repeat;"
        "  background-position: center;"
        "  background-origin: content;"
        "}");
    qDebug() << "stylesheet applied:" << ui->textBrowser_chat->styleSheet();
    ui->label->setPixmap(QPixmap(":/images/butterfly1.png"));
    ui->label->setStyleSheet(
        "border-radius: 25px;"
        "border: 2px solid #ffffff;"
        "background-color: #f5f5f5;"
    );
    ui->textEdit_input->setPlaceholderText("输入消息，Ctrl+Enter 发送...");
    ui->textEdit_input->installEventFilter(this);
    this->setWindowTitle("TCP连接");
    emojiPicker = new EmojiPicker(this);
    my_socket = new QTcpSocket(this);

    my_socket->connectToHost(SERVER_IP, SERVER_PORT);
    if(!my_socket->waitForConnected(3000)){
        QMessageBox::warning(this, "警告", "连接服务器失败:" + my_socket->errorString());
    }else{
        ui->textBrowser_chat->append("成功连接服务器");      
        my_socket->write(QString("LOGIN:%1").arg(username).toUtf8());
    }
    setupToolButton();

    connect(ui->label, &AvatarLabel::clicked, this, &ChatWindow::showAvatarBig);
    connect(ui->btn_send, &QPushButton::clicked, this, &ChatWindow::btn_send_clicked);
    connect(my_socket, &QTcpSocket::readyRead, this, &ChatWindow::tcp_ready_read);
    connect(my_socket, &QTcpSocket::disconnected, this, &ChatWindow::tcp_disconnected);
    connect(ui->textBrowser_chat, &QTextBrowser::anchorClicked,
    this, [=](const QUrl &url) {
    QDesktopServices::openUrl(url);
    });

    ui->listWidget_user->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget_user, &QListWidget::customContextMenuRequested,
        this, [=](const QPoint &pos) {
        QListWidgetItem *item = ui->listWidget_user->itemAt(pos);
        if (!item) return;

        // ★ 直接从 item 的 UserRole 取用户名
        QString selectedUser = item->data(Qt::UserRole).toString();
        if (selectedUser.isEmpty() || selectedUser == username) return;

        QMenu menu;
        QAction *actAt      = menu.addAction("@ 提及");
        QAction *actPrivate = menu.addAction("💬 私聊");
        QAction *actInfo    = menu.addAction("📋 查看资料");

        QAction *chosen = menu.exec(ui->listWidget_user->mapToGlobal(pos));
        if (chosen == actAt) {
            ui->textEdit_input->insertPlainText("@" + selectedUser + " ");
            ui->textEdit_input->setFocus();
        } else if (chosen == actPrivate) {
            my_socket->write(QString("PRIVATE:%1:你好，我想和你私聊")
                .arg(selectedUser).toUtf8());
        } else if (chosen == actInfo) {
            QMessageBox::information(this, "用户信息",
                QString("用户名：%1\n状态：在线").arg(selectedUser));
        }
    });
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::tcp_ready_read(){
    QString data = QString::fromUtf8(my_socket->readAll());
    QStringList parts = data.split(":");
    if(parts.size() < 2) return;

    QString type = parts.at(0);

    if(type == "CHAT"){
        QString sender = parts.at(1);
        QString content = parts.mid(2).join(":");
        QString str_time = QDateTime::currentDateTime().toString("hh:mm");

        bool isMine = (sender == username);
        QString bgColor = isMine ? "#95EC69" : "#FFFFFF";
        QString textColor = isMine ? "#000000" : "#333333";
        QString avatarAlign = isMine ? "right" : "left";
        QString avatarChar = sender.at(0).toUpper();
        QString avatarBg = isMine ? "#4CAF50" : "#2196F3";

        QString avatarHtml = QString(
            "<td align='%1' valign='top' width='40'>"
            "<div style='width:36px;height:36px;border-radius:18px;"
            "background:%2;color:#fff;text-align:center;line-height:36px;"
            "font-size:16px;font-weight:bold;'>%3</div>"
            "</td>"
        ).arg(avatarAlign).arg(avatarBg).arg(avatarChar);

        QString html = QString(
            "<table width='100%'><tr>"
            "%1"
            "<td align='%2' style='padding:4px;'>"
            "<div style='display:inline-block;max-width:60%%;"
            "background:%3;border-radius:8px;padding:8px 12px;"
            "color:%4;font-size:14px;'>%5</div>"
            "<div style='font-size:11px;color:#999;margin-top:2px;'>%6</div>"
            "</td>"
            "%7"
            "</tr></table>"
        ).arg(isMine ? "" : avatarHtml)
         .arg(isMine ? "right" : "left")
         .arg(bgColor)
         .arg(textColor)
         .arg(content.toHtmlEscaped())
         .arg(str_time)
         .arg(isMine ? avatarHtml : "");

        ui->textBrowser_chat->append(html);

        QTextCursor cursor = ui->textBrowser_chat->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->textBrowser_chat->setTextCursor(cursor);
    }
    else if(type == "USERLIST"){
        QString userStr = parts.mid(1).join(":");
        QStringList userList = userStr.split(",");

        ui->listWidget_user->clear();

        QListWidgetItem *titleItem = new QListWidgetItem("💬 ChatRoom");
        titleItem->setFont(QFont("楷体", 16, QFont::Bold));
        titleItem->setFlags(titleItem->flags() & ~Qt::ItemIsSelectable);
        ui->listWidget_user->addItem(titleItem);

        for(const QString &user : userList){
            if(user.isEmpty()) continue;

            QListWidgetItem *item = new QListWidgetItem();
            QWidget *w = new QWidget();
            QHBoxLayout *lay = new QHBoxLayout(w);
            lay->setContentsMargins(4, 4, 4, 4);
            lay->setSpacing(8);

            QLabel *avatar = new QLabel(user.at(0).toUpper());
            avatar->setFixedSize(32, 32);
            avatar->setAlignment(Qt::AlignCenter);
            avatar->setStyleSheet(QString(
                "background:%1;color:white;border-radius:16px;"
                "font-weight:bold;font-size:14px;")
                .arg(user == username ? "#4CAF50" : "#2196F3"));

            QLabel *name = new QLabel(user);
            name->setStyleSheet("font-size:13px;");

            QLabel *status = new QLabel("●");
            status->setStyleSheet("color:#4CAF50;font-size:10px;");

            lay->addWidget(avatar);
            lay->addWidget(name);
            lay->addStretch();
            lay->addWidget(status);

            item->setData(Qt::UserRole, user);
            item->setSizeHint(w->sizeHint());
            ui->listWidget_user->addItem(item);
            ui->listWidget_user->setItemWidget(item, w);
        }
    }

}

void ChatWindow::tcp_disconnected(){
    ui->textBrowser_chat->append("与服务器断开");
}

void ChatWindow::btn_send_clicked()
{
    QString content = ui->textEdit_input->toPlainText().trimmed();
    if(content.isEmpty())return;
    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    if(my_socket->state() != QAbstractSocket::ConnectedState){
        QMessageBox::critical(this, "失败", "未连接到服务器，无法发送");
        return;
    }
    my_socket->write(QString("MSG:%1").arg(content).toUtf8());

    ui->textEdit_input->clear();
}

void ChatWindow::setUsername(const QString &name){
    username = name;
    my_socket->write(QString("LOGIN:%1").arg(username).toUtf8());
}

void ChatWindow::showAvatarBig(){
    QDialog *previewDialog = new QDialog(this);
    previewDialog->setWindowTitle("头像预览");
    previewDialog->setModal(true);
    previewDialog->resize(320, 320);
    previewDialog->setStyleSheet("background-color: #1a1a1a;");

    QVBoxLayout *layout = new QVBoxLayout(previewDialog);
    layout->setContentsMargins(10, 10, 10, 10);

    QLabel *bigAvatar = new QLabel(previewDialog);
    bigAvatar->setAlignment(Qt::AlignCenter);
    bigAvatar->setPixmap(ui->label->pixmap()->scaled(
        300, 300,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    ));
    layout->addWidget(bigAvatar);

    previewDialog->exec();
    previewDialog->deleteLater();
}

void ChatWindow::closeEvent(QCloseEvent *event)
{
    if(my_socket->state() == QAbstractSocket::ConnectedState){
        my_socket->disconnectFromHost();
        my_socket->waitForDisconnected(1000);
    }
    event->accept();
}

bool ChatWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->textEdit_input && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        // Ctrl+Enter 发送
        if (keyEvent->key() == Qt::Key_Return 
            && (keyEvent->modifiers() & Qt::ControlModifier)) {
            btn_send_clicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ChatWindow::setupToolButton()
{
    ui->btn_tool->setPopupMode(QToolButton::InstantPopup);
    ui->btn_tool->setText("＋");
    ui->btn_tool->setArrowType(Qt::NoArrow);
    ui->btn_tool->setCursor(Qt::PointingHandCursor);
    ui->btn_tool->setToolTip("表情 / 图片 / 文件");
    ui->btn_tool->setStyleSheet(
        "QToolButton {"
        "  font-size: 22px; font-weight: bold;"
        "  border: 1px solid #ccc; border-radius: 6px;"
        "  background: #f5f5f5; color: #666;"
        "}"
        "QToolButton:hover { background: #e0e0e0; color: #333; }"
        "QToolButton:pressed { background: #d0d0d0; }"
    );

    toolMenu = new QMenu(this);
    toolMenu->setStyleSheet(
        "QMenu {"
        "  background: white; border: 1px solid #ddd;"
        "  border-radius: 8px; padding: 4px;"
        "}"
        "QMenu::item {"
        "  padding: 8px 32px 8px 16px; margin: 2px 4px;"
        "  border-radius: 6px; font-size: 14px;"
        "}"
        "QMenu::item:selected { background: #e8f0fe; color: #1a73e8; }"
    );

    QAction *actEmoji = toolMenu->addAction("😀  表情");
    QAction *actImage = toolMenu->addAction("📷  发送图片");
    QAction *actFile  = toolMenu->addAction("📎  发送文件");

    ui->btn_tool->setMenu(toolMenu);

    connect(emojiPicker, &EmojiPicker::emojiSelected, this, [=](const QString &emoji) {
            ui->textEdit_input->insertPlainText(emoji);
            ui->textEdit_input->setFocus();
        });
    connect(actEmoji, &QAction::triggered, this, [=]() {
        QPoint pos = ui->btn_tool->mapToGlobal(
            QPoint(0, -emojiPicker->height()));
        emojiPicker->move(pos);
        emojiPicker->show();
    });

    connect(actImage, &QAction::triggered, this, &ChatWindow::onSendImage);
    connect(actFile,  &QAction::triggered, this, &ChatWindow::onSendFile);
}

void ChatWindow::onSendImage()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "选择图片", "",
        "图片文件 (*.png *.jpg *.jpeg *.gif *.bmp *.webp)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件");
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QFileInfo info(filePath);
    QString base64 = data.toBase64();


    QString header = QString("IMAGE:%1:%2:%3")
        .arg(username).arg(info.fileName()).arg(base64.size());
    my_socket->write(header.toUtf8());
    my_socket->write(base64.toUtf8());

    QString html = QString(
        "<table width='100%'><tr><td></td>"
        "<td align='right' style='padding:4px;'>"
        "<div style='font-size:11px;color:#999;margin-bottom:2px;'>我</div>"
        "<img src='%1' width='180' "
        "style='border-radius:10px;border:1px solid #ddd;'/>"
        "</td></tr></table>"
    ).arg(filePath);
    ui->textBrowser_chat->append(html);
}

void ChatWindow::onSendFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件", "", "所有文件 (*.*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件");
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QFileInfo info(filePath);
    QString base64 = data.toBase64();

   QString header = QString("FILE:%1:%2:%3")
       .arg(username).arg(info.fileName()).arg(base64.size());
   my_socket->write(header.toUtf8());
   my_socket->write(base64.toUtf8());

   QString html = QString(
        "<table width='100%'><tr>"
        "<td></td>"
        "<td align='right' style='padding:4px;'>"
        "<div style='font-size:11px;color:#999;margin-bottom:2px;'>我</div>"
        "<div style='display:inline-block;background:#95EC69;border-radius:10px;"
        "padding:12px 16px;max-width:70%;text-align:left;'>"
        "<div style='font-size:24px;margin-bottom:4px;'>📎</div>"
        "<div style='font-size:13px;font-weight:bold;color:#333;'>%1</div>"
        "<div style='font-size:11px;color:#888;margin:4px 0;'>%2 KB</div>"
        "<a href='file:///%3'>📥 点击打开</a>"
        "</div>"
        "</td>"
        "</tr></table>"
    ).arg(info.fileName())
     .arg(info.size() / 1024)
     .arg(filePath);  // filePath 就是完整路径，如 C:/Users/xxx/a.txt

    ui->textBrowser_chat->append(html);

    QTextCursor cursor = ui->textBrowser_chat->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textBrowser_chat->setTextCursor(cursor);
}

