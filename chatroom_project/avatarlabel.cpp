#include "avatarlabel.h"
#include <QMouseEvent>

AvatarLabel::AvatarLabel(QWidget *parent) : QLabel(parent)
{
    setFixedSize(42, 42);       // 头像默认尺寸，可自行调整
    setScaledContents(true);    // 图片自动缩放填满控件
    setCursor(Qt::PointingHandCursor); // 鼠标移上去变成手型，提示可点击
}

void AvatarLabel::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        emit clicked(); // 左键点击时发出信号
    }
    QLabel::mousePressEvent(event);
}
