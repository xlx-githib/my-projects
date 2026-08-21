#ifndef REG_H
#define REG_H

#include <QWidget>

namespace Ui {
class Reg;
}

class Reg : public QWidget
{
    Q_OBJECT

public:
    explicit Reg(QWidget *parent = 0);
    ~Reg();

private:
    Ui::Reg *ui;
};

#endif // REG_H
