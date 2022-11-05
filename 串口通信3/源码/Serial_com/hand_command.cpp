#include "hand_command.h"
#include "ui_hand_command.h"

Hand_Command::Hand_Command(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Hand_Command)
{
    ui->setupUi(this);
}

Hand_Command::~Hand_Command()
{
    delete ui;
}
