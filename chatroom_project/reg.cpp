#include "reg.h"
#include "ui_reg.h"

#include <QMessageBox>
#include <QDebug>
#include <QString>
#include <stdio.h>
#include <string.h>

Reg::Reg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Reg)
{
    ui->setupUi(this);
    this->setWindowTitle("注册");

    connect(ui->btn_Zclear, &QPushButton::clicked, this, [=](){
        ui->lineEdit_Zuser->text().clear();
        ui->lineEdit_Zpwd->text().clear();
        ui->lineEdit_Zpwd2->text().clear();
    });

    connect(ui->btn_Zclose, &QPushButton::clicked, this, &Reg::close);
    connect(ui->btn_Zset, &QPushButton::clicked, this, [=](){
        QString user;
        QString pwd1;
        QString pwd2;
        char tmp1[300];
        QString str;
        char *pname;
        char *ppwd;
        QByteArray by_name;
        QByteArray by_pwd;
        FILE *pf1;
        FILE *pf2;

        user = ui->lineEdit_Zuser->text();
        pwd1 = ui->lineEdit_Zpwd->text();
        pwd2 = ui->lineEdit_Zpwd2->text();

        if((pf1 = fopen("./data.txt", "r+")) == NULL){
            qDebug() << "打开文件失败1" ;
        }

        while((fgets(tmp1, 300, pf1)) != NULL){
            str = strtok(tmp1, ":");
            if(str == user){
                QMessageBox::critical(this, "错误", "该用户名已存在");
                return ;
            }
        }

        fclose(pf1);

        if((pf2 = fopen("./data.txt", "a+")) == NULL){
            qDebug() << "打开文件失败2" ;
        }

        if(user == NULL || pwd1 == NULL || pwd2 == NULL)
        {
            QMessageBox::critical(this, "错误", "缺少用户名或密码");
        }
        else if(pwd1 != pwd2)
        {
            QMessageBox::critical(this, "错误", "密码输入错误");
            ui->lineEdit_Zpwd->clear();
            ui->lineEdit_Zpwd2->clear();
        }
        else
        {
            by_name = user.toUtf8();
            by_pwd = pwd1.toUtf8();
            pname = by_name.data();
            ppwd = by_pwd.data();
            fputs(pname, pf2);
            fputs(":", pf2);
            fputs(ppwd, pf2);
            fputs("\n", pf2);
            QMessageBox::information(this, "恭喜", "成功注册");
            this->close();
        }
        fclose(pf2);
    });
}

Reg::~Reg()
{
    delete ui;
}
