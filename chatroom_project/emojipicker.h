// emojipicker.h
#ifndef EMOJIPICKER_H
#define EMOJIPICKER_H

#include <QWidget>
#include <QVector>

class EmojiPicker : public QWidget
{
    Q_OBJECT
public:
    explicit EmojiPicker(QWidget *parent = nullptr);
signals:
    void emojiSelected(const QString &emoji);
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
private:
    QVector<QString> emojiList;
};

#endif // EMOJIPICKER_H
