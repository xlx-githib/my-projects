// emojipicker.cpp
#include "emojipicker.h"
#include <QGridLayout>
#include <QPushButton>
#include <QKeyEvent>

EmojiPicker::EmojiPicker(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);

    emojiList = {
        "😀", "😂", "🤣", "😊", "😍", "🤩", "😘", "😜",
        "🤔", "😏", "😌", "😔", "😪", "😭", "😤", "😡",
        "👍", "👎", "👏", "🙌", "💪", "👋", "❤️", "💔",
        "🎉", "🔥", "⭐", "✅", "❌", "💯", "🎵", "🍔",
        "☕", "🍺", "🌹", "🌸", "🐱", "🐶", "🚀", "💡"
    };

    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(4, 4, 4, 4);

    int cols = 8;
    for (int i = 0; i < emojiList.size(); i++) {
        QPushButton *btn = new QPushButton(emojiList[i], this);
        btn->setFixedSize(36, 36);
        btn->setStyleSheet(
            "QPushButton { font-size:20px; border:none; background:transparent; }"
            "QPushButton:hover { background:#e0e0e0; border-radius:4px; }");
        connect(btn, &QPushButton::clicked, this, [=]() {
            emit emojiSelected(emojiList[i]);
            close();
        });
        layout->addWidget(btn, i / cols, i % cols);
    }
    setStyleSheet("background:white; border:1px solid #ccc; border-radius:8px;");
}

void EmojiPicker::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
    }
    QWidget::keyPressEvent(event);
}

void EmojiPicker::focusOutEvent(QFocusEvent *event)
{
    close();
    QWidget::focusOutEvent(event);
}