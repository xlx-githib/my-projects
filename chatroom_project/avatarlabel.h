#ifndef AVATARLABEL_H
#define AVATARLABEL_H

#include <QLabel>

class AvatarLabel : public QLabel
{
    Q_OBJECT
public:
    explicit AvatarLabel(QWidget *parent = nullptr);

signals:
    void clicked(); // 自定义点击信号

protected:
    void mousePressEvent(QMouseEvent *event) override; // 重写鼠标点击事件
};

#endif // AVATARLABEL_H
