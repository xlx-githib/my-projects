#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "reg.h"
#include "chatwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("登录界面");

    Reg *reg = new Reg();
    ChatWindow *chatwidget = new ChatWindow();
    ui->btn_login->setFocus();
    ui->btn_login->setDefault(true);

    connect(ui->btn_clear, &QPushButton::clicked, this, [=](){
        ui->lineEdit_user->clear();
        ui->lineEdit_pwd->clear();
    });
    connect(ui->btn_exit, &QPushButton::clicked, this, &MainWindow::close);
    connect(ui->btn_set, &QPushButton::clicked, this, [=](){
        reg->show();
    });
    connect(ui->btn_login, &QPushButton::clicked, this, [=](){
        chatwidget->setUsername(ui->lineEdit_user->text());
        chatwidget->show();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    qApp->quit();
}
