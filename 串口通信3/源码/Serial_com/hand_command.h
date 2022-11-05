#ifndef HAND_COMMAND_H
#define HAND_COMMAND_H

#include <QMainWindow>

namespace Ui {
class Hand_Command;
}

class Hand_Command : public QMainWindow
{
    Q_OBJECT

public:
    explicit Hand_Command(QWidget *parent = nullptr);
    ~Hand_Command();

private:
    Ui::Hand_Command *ui;
};

#endif // HAND_COMMAND_H
