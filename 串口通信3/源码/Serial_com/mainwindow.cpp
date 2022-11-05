#include "mainwindow.h"
#include "ui_mainwindow.h"

QStringList info=
{
    "1200-8-无-1",
    "9600-8-无-1",
    "19200-8-无-1",
    "1200-7-偶-1",
    "2400-7-偶-1",
    "4800-7-偶-1",
    "9600-7-偶-1",
    "19200-7-偶-1",
    "38400-8-无-1",
    "38400-7-偶-1",
    "115200-8-无-1",
    "115200-7-偶-1"
};
/*实现 端口轮询*/
int communicatSet[][4]={
    //Baud rate            Data bits           Parity                Stop bits
    {QSerialPort::Baud1200,QSerialPort::Data8,QSerialPort::NoParity,QSerialPort::OneStop},
    {QSerialPort::Baud9600,QSerialPort::Data8,QSerialPort::NoParity,QSerialPort::OneStop},
    {QSerialPort::Baud19200,QSerialPort::Data8,QSerialPort::NoParity,QSerialPort::OneStop},
    {QSerialPort::Baud1200,QSerialPort::Data7,QSerialPort::EvenParity,QSerialPort::OneStop},
    {QSerialPort::Baud2400,QSerialPort::Data7,QSerialPort::EvenParity,QSerialPort::OneStop},
    {QSerialPort::Baud4800,QSerialPort::Data7,QSerialPort::EvenParity,QSerialPort::OneStop},
    {QSerialPort::Baud9600,QSerialPort::Data7,QSerialPort::EvenParity,QSerialPort::OneStop},
    {QSerialPort::Baud19200,QSerialPort::Data7,QSerialPort::EvenParity,QSerialPort::OneStop},
    {QSerialPort::Baud38400,QSerialPort::Data8,QSerialPort::NoParity,QSerialPort::OneStop},
    {QSerialPort::Baud38400,QSerialPort::Data7,QSerialPort::EvenParity,QSerialPort::OneStop},
    {QSerialPort::Baud115200,QSerialPort::Data8,QSerialPort::NoParity,QSerialPort::OneStop},
    {QSerialPort::Baud115200,QSerialPort::Data7,QSerialPort::EvenParity,QSerialPort::OneStop}
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowFlag(Qt::WindowMaximizeButtonHint, false); //设置最大化为灰色
    init_Page();

    //被动模式 -> 测量 -> 缓冲区跟踪测量
    buffer_time = new QTimer;
    buffer_time->stop();
    connect(buffer_time,&QTimer::timeout,this,[=]()
    {
        readSerialData();
    });
    //被动模式 -> 测量 -> 定时读数
    timing_time = new QTimer;
    timing_time->stop();
    connect(timing_time,&QTimer::timeout,this,[=]()
    {
        on_timing_start_clicked();
    });
    //主动模式 -> 主动测量配置
    automeasure_time = new QTimer;
    automeasure_time->stop();
    connect(automeasure_time,&QTimer::timeout,this,[=]()
    {
        on_timedreadData_start_clicked();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::serialPortPoll()
{
    //轮询集合行数
    //if(ui->comboBox_port-)
    int setlen=sizeof(communicatSet)/sizeof(communicatSet[0]);
    for(int index=0;index<setlen;index++){
        if(serial->isOpen())                                  // 如果串口打开了，先给他关闭
        {
            serial->close();
        }
        //设置串口连接参数 进行连接
        serial->setBaudRate(communicatSet[index][0]);
        serial->setDataBits((QSerialPort::DataBits)communicatSet[index][1]);
        serial->setParity((QSerialPort::Parity)communicatSet[index][2]);
        //当前选择的串口名字
        serial->setPortName(ui->comboBox_port->currentText());
        ui->textBrowser_record->append("打开"+ui->comboBox_port->currentText()+" @ "+info[index]);
        if(!serial->open(QIODevice::ReadWrite)){
            continue;
        }else{

            mtimer=new QTimer();
            //延迟50ms接受信息
            connect(mtimer,&QTimer::timeout,this,&MainWindow::readSerialData);
            //串口接受消息事件
            connect(serial,&QSerialPort::readyRead,this,[=](){
                mtimer->start(50);
            });

            //发送dg命令获取设备id
            char command[512];
            sprintf(command,"dg\r\n");
            dataSend(command);
            //休眠一秒
            sleep(200);

            if(CurrectMode){
                //如果状态位变换 不变则继续
                //返回index
                char command1[512];
                sprintf(command1,"s%sc\r\n",device_num.data());
                dataSend(command1);
                sleep(200);
                char command2[512];
                sprintf(command2,"dg\r\n");
                dataSend(command2);
                sleep(200);
                serial->clear();
                serial->close();
                mtimer->stop();
                QMessageBox::about(NULL,"提示","请再次点击[检查连接]按钮");
                return index;
            }else{
                serial->clear();
                serial->close();
                mtimer->stop();
                continue;
            }
        }
    }
    //连接失败
    return -1;
}

void MainWindow::on_pushButton_checkConnect_clicked()
{
    if(connect_flag==0)
    {
        index = serialPortPoll();
        if(!(index<0))
            ui->comboBox_conf->setCurrentIndex(index);
        if(CurrectMode)
            connect_flag=1;
    }
    else if(connect_flag==1)
    {
        QString serialName = ui->comboBox_port->currentText();
        serial->setPortName(serialName);
        if(serial->open(QIODevice::ReadWrite))
        {
            serial->setBaudRate(communicatSet[index][0]);
            serial->setDataBits((QSerialPort::DataBits)communicatSet[index][1]);
            serial->setParity((QSerialPort::Parity)communicatSet[index][2]);
            mtimer=new QTimer();
            //延迟50ms接受信息
            connect(mtimer,&QTimer::timeout,this,&MainWindow::readSerialData);
            //串口接受消息事件
            connect(serial,&QSerialPort::readyRead,this,[=](){
                mtimer->start(50);
            });
            init_command();
            inited_Page();
        }
        else
        {
            if(serial!=NULL)
            {
                mtimer->stop();
                serial->close();
                serial=NULL;
            }
        }
    }
}

void MainWindow::dataSend(char *str)
{
    serial->write(str);
    ui->textBrowser_record->append(QString("-> ")+str);
}

void MainWindow::init_Page()
{
    CurrectMode = false;
    //设置软件版本与序列号中的lineEdit为不可编辑
    ui->lineEdit_product->setFocusPolicy(Qt::NoFocus);
    ui->lineEdit_modVersion->setFocusPolicy(Qt::NoFocus);
    ui->lineEdit_interfaceVersion->setFocusPolicy(Qt::NoFocus);
    //设置连接页面中状态显示的lineEdit为不可编辑
    ui->lineEdit_status->setFocusPolicy(Qt::NoFocus);

    //设置连接页面中部分按钮的可用状态
    ui->pushButton_checkConnect->setEnabled(false);
    ui->pushButton_readDeviceConf->setEnabled(false);
    ui->pushButton_conFailed->setEnabled(false);
    ui->pushButton_refreshPort->setEnabled(true);
    //设置各个tabWidget初始化界面为第一个
    ui->tabWidget->setCurrentIndex(0);
    ui->tabWidget_passive->setCurrentIndex(0);
    ui->tabWidget_active->setCurrentIndex(0);
    ui->tabWidget_conf->setCurrentIndex(0);

    //未连接设备时，其他页面不可用
    //被动模式 -> 测量页面初始化
    ui->tab_measure->setEnabled(false);
    ui->groupBox_manualRead->setEnabled(false);
    ui->groupBox_timeRead->setEnabled(false);
    ui->sensor_stop->setEnabled(false);
    ui->timing_stop->setEnabled(false);
    ui->buffer_stop->setEnabled(false);
    ui->distance_display->setFocusPolicy(Qt::NoFocus);
    ui->sensor_distance->setFocusPolicy(Qt::NoFocus);
    ui->measure_distance->setFocusPolicy(Qt::NoFocus);
    ui->measure_newvalue->setFocusPolicy(Qt::NoFocus);
    //主动模式 -> 自动测量配置
    ui->tab_automeasure->setEnabled(false);
    ui->groupBox_manualReadData->setEnabled(false);
    ui->groupBox_timingReadData->setEnabled(false);
    ui->groupBox_passiveMeasure->setEnabled(false);
    ui->automeasure_distance->setFocusPolicy(Qt::NoFocus);
    ui->automeasure_newvalue->setFocusPolicy(Qt::NoFocus);
    ui->timedreadData_start->setEnabled(true);
    ui->timedreadData_stop->setEnabled(false);
    ui->tab_manualmeasure->setEnabled(false);
    //主动模式 -> 手动触发测量配置
    ui->widget_activate->setEnabled(false);
    ui->DI1_edit->setFocusPolicy(Qt::NoFocus);
    ui->read_digital_input->setVisible(true);
    //主动模式 -> 自动测量配置(含用户配置常数)

    //配置
    //配置 -> 测量参数
    ui->tab_messurePara->setEnabled(false);
    ui->standard->setVisible(true);
    ui->user_timedmeasure->setVisible(true);
    //配置 -> 过滤器
    ui->tab_filter->setEnabled(false);
    if(!(ui->filter_open->isChecked()))
    {
        ui->filter_length->setEnabled(false);
        ui->filter_extremevalue->setEnabled(false);
        ui->filter_error->setEnabled(false);
    }
    //配置 -> 模拟量输出
    ui->tab_analogOutput->setEnabled(false);
    QRegExp rx2("^?[0-9]+([.][0-9]{1})?$");
    QRegExpValidator *pReg2 = new QRegExpValidator(rx2, this);
    ui->error_current->setValidator(pReg2);
    connect(ui->error_current,&QLineEdit::editingFinished,this,[=](){
            double num=ui->error_current->text().toDouble();
            num=num>20?20:num;
            num=num<0?0:num;
            ui->error_current->setText(QString::number(num));});
    //配置 -> 数字量输出
    ui->tab_digitalOutput->setEnabled(false);
    //配置 -> SSI
    ui->tab_SSI->setEnabled(false);
    if(!(ui->SSI->isChecked()))
    {
        ui->additional_errorbits1->setEnabled(false);
        ui->additional_errorbits2->setEnabled(false);
        ui->groupBox_codeData->setEnabled(false);
        ui->groupBox_measureValue->setEnabled(false);
        ui->groupBox_errorDistanceOutput->setEnabled(false);
    }
    //配置 -> 用户要求配置的输出格式
    //...
    //配置 -> 用户配置参数
    //...
    //配置 -> DeviceID
    ui->tab_deviceID->setEnabled(false);


    //tabBar文字横向显示
    //被动模式页面
    QString text1 = "测量(含用户配\n置常数)";
    ui->tabWidget_passive->tabBar()->setStyle(new TabBarStyle(Qt::Horizontal));
    ui->tabWidget_passive->setTabText(1,text1);
    //主动模式页面
    QString text2 = "自动测量配置\n(含用户配置常\n数)";
    QString text3 = "手动触发测量\n配置";
    ui->tabWidget_active->tabBar()->setStyle(new TabBarStyle(Qt::Horizontal));
    ui->tabWidget_active->setTabText(1,text2);
    ui->tabWidget_active->setTabText(2,text3);
    //配置页面
    QString text4 = "用户要求配置\n的输出格式";
    ui->tabWidget_conf->tabBar()->setStyle(new TabBarStyle(Qt::Horizontal));
    ui->tabWidget_conf->setTabText(5,text4);

}

void MainWindow::inited_Page()
{
    ui->pushButton_checkConnect->setEnabled(false);
    ui->pushButton_readDeviceConf->setEnabled(true);
    ui->pushButton_conFailed->setEnabled(true);
    ui->pushButton_refreshPort->setEnabled(false);
    //连接设备后，其他页面可用
    //被动模式 -> 测量
    ui->tab_measure->setEnabled(true);
    //主动模式 -> 自动测量配置
    ui->tab_automeasure->setEnabled(true);
    ui->tab_manualmeasure->setEnabled(true);
    ui->tab_conf->setEnabled(true);
    //主动模式 -> 手动测量配置
    ui->read_digital_input->setVisible(false);
    //主动模式 -> 自动测量配置(含用户配置常数)

    //配置
    //配置 -> 测量参数
    ui->tab_messurePara->setEnabled(true);
    ui->standard->setVisible(false);
    ui->user_timedmeasure->setVisible(false);
    //配置 -> 过滤器
    ui->tab_filter->setEnabled(true);
    //配置 -> 模拟量输出
    ui->tab_analogOutput->setEnabled(true);
    //配置 -> 数字量输出
    ui->tab_digitalOutput->setEnabled(true);
    //配置 -> SSI
    ui->tab_SSI->setEnabled(true);
    //配置 -> 用户要求配置的输出格式
    //...
    //配置 -> 用户配置参数
    //...
    //配置 -> DeviceID
    ui->tab_deviceID->setEnabled(true);
}

void MainWindow::init_command()
{
    char command[512];
    sprintf(command,"dg\r\n");
    dataSend(command);
    sleep(200);
    sprintf(command,"s%ssv\r\n",device_num.data());
    dataSend(command);
    //获取序列号(sNsn)
    sleep(200);
    sprintf(command,"s%ssn\r\n",device_num.data());
    dataSend(command);
}

void MainWindow::sleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MainWindow::on_pushButton_refreshPort_clicked()
{
    serial = new QSerialPort;                       //申请内存,并设置父对象
    ui->comboBox_port->clear();
    // 获取计算机中有效的端口号，然后将端口号的名称给端口选择控件
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        serial->setPort(info);                      // 在对象中设置串口
        if(serial->open(QIODevice::ReadWrite))      // 以读写方式打开串口
        {
            ui->comboBox_port->addItem(info.portName());  // 添加计算机中的端口
            serial->close();                        // 关闭
        }
    }
    if(ui->comboBox_port->currentText()!=NULL)
        ui->pushButton_checkConnect->setEnabled(true);
}

void MainWindow::on_pushButton_conFailed_clicked()
{
    timing_time->stop();
    buffer_time->stop();
    mtimer->stop();
    ui->lineEdit_status->clear();
    ui->lineEdit_status->setText("断开连接");
    if(serial->isOpen())
    {
        serial->clear();
        serial->close();
    }
    init_Page();
}

void MainWindow::readSerialData()
{
    char BUF[512] = {0};                                       // 存储转换类型后的数据
    QByteArray data = serial->readAll();                      // 读取数据

    if(!data.isEmpty())                                 // 接收到数据
    {
        // 清除之前的数据，防止追加，因为每次获取的数据不一样
        sscanf(data.data(),"%[^\r\n]",BUF);
        ui->textBrowser_record->append(QString("<- ")+data);

        /*开始处理指令*/
        char head[20];
        char content[512];

        if(strstr(BUF,"+")!=NULL){
            /*表示接收到正常的回复信息*/
            sscanf(BUF, "%[^+]%*c%s", head,content);
            QString data(head);

            //当进行dg探测命令时，如果收到返回，将不再进行探测，并更改CurrectMode的状态
            if(!CurrectMode){
                QStringList listCommand = data.split("+");//QString字符串分割函数
                char num[10]={0};
                sscanf(listCommand[0].toLatin1().data(), "%*c%[0-9]%*s",num);
                if(strlen(num)!=0){
                    device_num=QString(num).toLatin1();
                    ui->comboBox_deviceID->setCurrentText(device_num);
                    CurrectMode=true;
                }
                return ;
            }
            QStringList listCommand = data.split(device_num);//QString字符串分割函数
            if(strstr(head,device_num.data())==NULL){
                return;
            }
            if(listCommand[1]=="g"){
                sNgRetHand(content);
                return ;
            }else if(listCommand[1]=="h"){
                sNhRetHand(content);
                return ;
            }else if(listCommand[1]=="sv"){
                sNsvRetHand(content);
                return ;
            }else if(listCommand[1]=="sn"){
                sNsnRetHand(content);
                return ;
            }
            else if(listCommand[1]=="q"){
                sNqRetHand(content);
                return ;
            }
            else if(listCommand[1]=="RI")
            {
                sNRIRetHand(content);
                return ;
            }
        }else if(strstr(BUF,"@")!=NULL){
            /*接受到错误信息*/
            sscanf(BUF, "%[^@]%*c%s", head,content);

            if(strcasecmp(content,"E203")==0){
                if(!CurrectMode){
                    //检查命令，参数和通信设置(波特率，停止位，奇偶校验和终止)
                    char num[10]={0};
                    sscanf(head, "%*c%[0-9]%*s",num);
                    device_num=QString(num).toLatin1();
                    ui->comboBox_deviceID->setCurrentText(device_num);
                    CurrectMode=true;
                }
            }
        }else if(strstr(BUF,"?")!=NULL){
            /*停止命令 返回信息*/
            if(strstr(head,device_num.data())==NULL){
                return;
            }
        }
    }
}

/**
 * 处理sNg指令
 */
void MainWindow::sNgRetHand(char *msg)
{
    double data=QString(msg).toDouble();
    if(ui->distance_mm->isChecked()){
        data=data/10.0;
        ui->distance_display->setText(QString::number(data));
    }else if(ui->distance_cm->isChecked()){
        data=data/100.0;
        ui->distance_display->setText(QString::number(data));
    }else if(ui->distance_m->isChecked()){
        data=data/10000.0;
        ui->distance_display->setText(QString::number(data));
    }else{
        ui->distance_display->setText(QString(msg));
    }
}
/**
 * 处理sNh指令
 */
void MainWindow::sNhRetHand(char *msg)
{
    double data=QString(msg).toDouble();
    if(ui->sensor_mm->isChecked()){
        data=data/10.0;
        ui->sensor_distance->setText(QString::number(data));
    }else if(ui->sensor_cm->isChecked()){
        data=data/100.0;
        ui->sensor_distance->setText(QString::number(data));
    }else if(ui->sensor_m->isChecked()){
        data=data/10000.0;
        ui->sensor_distance->setText(QString::number(data));
    }else{
        ui->sensor_distance->setText(QString(msg));
    }
}
/**
 * 处理sNsv指令
 */
void MainWindow::sNsvRetHand(char *msg)
{
    char interface_version[8];
    char model_version[8];
    sscanf(msg,"%4s%4s",model_version,interface_version);

    ui->lineEdit_modVersion->setText(QString(model_version));
    ui->lineEdit_interfaceVersion->setText(QString(interface_version));
}
/**
 * 处理sNsn指令
 */
void MainWindow::sNsnRetHand(char *msg)
{
    ui->lineEdit_product->setText(QString(msg));
}
/**
 * 处理sNq指令
 */
void MainWindow::sNqRetHand(char *msg)
{
    /* sNq 返回格式 gNq+aaaaaaaa+b <CrLF>*/
    /*接收到的命令aaaaaaaa+b*/
    int type;
    char str[512];

    sscanf(msg,"%[^+]%d\r\n",str,&type);
    double data=QString(str).toDouble();
    if(automeasure_flag==0)
    {
        if(ui->measureDistance_mm->isChecked()){
            data=data/10.0;
        }else if(ui->measureDistance_cm->isChecked()){
            data=data/100.0;

        }else if(ui->measureDistance_m->isChecked()){
            data=data/10000.0;
        }
        ui->measure_distance->setText(QString::number(data));
        switch(type){
        case 0:ui->measure_newvalue->setText("没有新的测量结果");break;
        case 1:ui->measure_newvalue->setText("一个新测量值，未被覆盖");break;
        case 2:ui->measure_newvalue->setText("超过一个测量值被覆盖");break;
        }
    }
    else if(automeasure_flag==1)
    {
        if(ui->automeasureDistance_mm->isChecked()){
            data=data/10.0;
        }else if(ui->automeasureDistance_cm->isChecked()){
            data=data/100.0;

        }else if(ui->automeasureDistance_m->isChecked()){
            data=data/10000.0;
        }
        ui->automeasure_distance->setText(QString::number(data));
        switch(type){
        case 0:ui->automeasure_newvalue->setText("没有新的测量结果");break;
        case 1:ui->automeasure_newvalue->setText("一个新测量值，未被覆盖");break;
        case 2:ui->automeasure_newvalue->setText("超过一个测量值被覆盖");break;
        }
    }
}

void MainWindow::sNRIRetHand(char *msg)
{
    ui->DI1_edit->setText(QString(msg));
}

void MainWindow::on_distance_measure_clicked()
{
    char command[512];
    sprintf(command,"s%sg\r\n",device_num.data());
    dataSend(command);
}

void MainWindow::on_sensor_start_clicked()
{
    ui->sensor_stop->setEnabled(true);
    ui->sensor_start->setEnabled(false);
    double temp_time = ui->lineEdit_sampleTime1->text().toDouble()*1000;
    char command[512];
    sprintf(command,"s%sh+%.0f\r\n",device_num.data(),temp_time);
    dataSend(command);
}

void MainWindow::on_sensor_stop_clicked()
{
    clear_command();
    ui->sensor_stop->setEnabled(false);
    ui->sensor_start->setEnabled(true);
}

void MainWindow::on_pushButton_recordClear_clicked()
{
    ui->textBrowser_record->clear();
}

void MainWindow::clear_command()
{
    char command[512];
    sprintf(command,"s%sc\r\n",device_num.data());
    dataSend(command);
}

void MainWindow::saveConf_command()
{
    //发送sNs指令，保存参数配置
    char command[512];
    sprintf(command,"s%ss\r\n",device_num.data());
    dataSend(command);
}
void MainWindow::on_buffer_start_clicked()
{
    double time = ui->lineEdit_sampleTime2->text().toDouble()*1000;
    ui->buffer_stop->setEnabled(true);
    ui->buffer_start->setEnabled(false);
    ui->groupBox_messure->setEnabled(false);
    ui->groupBox_manualRead->setEnabled(true);
    ui->groupBox_sensor->setEnabled(false);
    ui->groupBox_timeRead->setEnabled(true);
    buffer_time->start(time);
    char command[512];
    sprintf(command,"s%sf+%.0f\r\n",device_num.data(),time);
    dataSend(command);
    automeasure_flag=0;
}

void MainWindow::on_buffer_stop_clicked()
{
    clear_command();
    ui->buffer_stop->setEnabled(false);
    ui->buffer_start->setEnabled(true);
    ui->groupBox_messure->setEnabled(true);
    ui->groupBox_manualRead->setEnabled(false);
    ui->groupBox_sensor->setEnabled(true);
    ui->groupBox_timeRead->setEnabled(false);
    buffer_time->stop();
    timing_time->stop();
    ui->timing_stop->setEnabled(false);
    ui->timing_start->setEnabled(true);
}

void MainWindow::on_manual_read_clicked()
{
    char command[512];
    sprintf(command,"s%sq\r\n",device_num.data());
    dataSend(command);
}

void MainWindow::on_timing_start_clicked()
{
    double time = ui->lineEdit_sampleTime3->text().toDouble()*1000;
    ui->timing_stop->setEnabled(true);
    ui->timing_start->setEnabled(false);
    timing_time->start(time);
    char command[512];
    sprintf(command,"s%sq\r\n",device_num.data());
    dataSend(command);
}

void MainWindow::on_timing_stop_clicked()
{
    ui->timing_stop->setEnabled(false);
    ui->timing_start->setEnabled(true);
    timing_time->stop();
}

void MainWindow::on_automeasure_writemodule_clicked()
{
    if(ui->automeasure_open->isChecked())
    {
        ui->groupBox_manualReadData->setEnabled(true);
        ui->groupBox_timingReadData->setEnabled(true);
        ui->groupBox_passiveMeasure->setEnabled(true);
        clear_command();
        sleep(200);
        int _time = ui->lineEdit_sampleTime4->text().toDouble()*1000;
        QString temp_time = QString("%1").arg(_time,8,10,QLatin1Char('0'));
        QByteArray time = temp_time.toLatin1();
        char command[512];
        sprintf(command,"s%sA+%s\r\n",device_num.data(),time.data());
        dataSend(command);
        automeasure_flag = 1;
        ui->tab_measure->setEnabled(false);
    }
    else
    {
        ui->groupBox_manualReadData->setEnabled(false);
        ui->groupBox_timingReadData->setEnabled(false);
        ui->groupBox_passiveMeasure->setEnabled(false);
        automeasure_flag=0;
        ui->tab_measure->setEnabled(true);
        on_buffer_stop_clicked();
        sleep(200);
        saveConf_command();
    }
}

void MainWindow::on_manualreading_button_clicked()
{
    char command[512];
    sprintf(command,"s%sq\r\n",device_num.data());
    dataSend(command);
}

void MainWindow::on_timedreadData_start_clicked()
{
    double time = ui->lineEdit_sampleTime5->text().toDouble()*1000;
    ui->timedreadData_stop->setEnabled(true);
    ui->timedreadData_start->setEnabled(false);
    automeasure_time->start(time);
    char command[512];
    sprintf(command,"s%sq\r\n",device_num.data());
    dataSend(command);
}

void MainWindow::on_timedreadData_stop_clicked()
{
    clear_command();
    ui->timedreadData_stop->setEnabled(false);
    ui->timedreadData_start->setEnabled(true);
    automeasure_time->stop();
}

void MainWindow::on_automeasure_open_stateChanged(int)
{
    if(ui->automeasure_open->isChecked())
        automeasure_flag = 1;
    else automeasure_flag = 0;
}

void MainWindow::on_notActivate_clicked()
{
    ui->widget_activate->setEnabled(false);
}

void MainWindow::on_activate_clicked()
{
    ui->widget_activate->setEnabled(true);
}

void MainWindow::on_read_DI1_button_clicked()
{
    char command[512];
    sprintf(command,"s%sRI\r\n",device_num.data());
    dataSend(command);
}


void MainWindow::on_RS422_clicked()
{
    ui->additional_errorbits1->setEnabled(false);
    ui->additional_errorbits2->setEnabled(false);
    ui->groupBox_codeData->setEnabled(false);
    ui->groupBox_measureValue->setEnabled(false);
    ui->groupBox_errorDistanceOutput->setEnabled(false);
}

void MainWindow::on_SSI_clicked()
{
    ui->additional_errorbits1->setEnabled(true);
    ui->additional_errorbits2->setEnabled(true);
    ui->groupBox_codeData->setEnabled(true);
    ui->groupBox_measureValue->setEnabled(true);
    ui->groupBox_errorDistanceOutput->setEnabled(true);
}

void MainWindow::on_filter_close_clicked()
{
    ui->filter_length->setEnabled(false);
    ui->filter_extremevalue->setEnabled(false);
    ui->filter_error->setEnabled(false);
}

void MainWindow::on_filter_open_clicked()
{
    ui->filter_length->setEnabled(true);
    ui->filter_extremevalue->setEnabled(true);
    ui->filter_error->setEnabled(true);
}

void MainWindow::on_downtodevice_8_clicked()
{
    if(ui->notActivate->isChecked())
    {
        clear_command();
        sleep(200);
        char command[512];
        sprintf(command,"s%sDI1+00000000\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
    else if(ui->activate->isChecked())
    {
        if(ui->trigger_distance_measure->isChecked())   //触发测量距离
        {
            clear_command();
            sleep(200);
            char command[512];
            sprintf(command,"s%sDI1+00000002\r\n",device_num.data());
            dataSend(command);
            sleep(200);
            saveConf_command();
        }
        if(ui->track_measure->isChecked())      //开始/停止单个传感器跟踪测量
        {
            clear_command();
            sleep(200);
            char command[512];
            sprintf(command,"s%sDI1+000000003\r\n",device_num.data());
            dataSend(command);
            sleep(200);
            saveConf_command();
        }
        if(ui->buffer_track_measure->isChecked())  //开始/停止单个传感器缓冲跟踪测量
        {
            clear_command();
            sleep(200);
            char command1[512];
            double time1 = ui->time->text().toFloat()*1000;
            sprintf(command1,"s%sf+%.0f\r\n",device_num.data(),time1);
            dataSend(command1);
            sleep(200);
            clear_command();
            sleep(200);
            char command2[512];
            sprintf(command2,"s%sDI1+00000004\r\n",device_num.data());
            dataSend(command2);
            sleep(200);
            saveConf_command();
        }
        if(ui->timed_track_measure->isChecked())  //开始/停止单个传感器定时跟踪测量
        {
            clear_command();
            sleep(200);
            char command1[512];
            double time1 = ui->time->text().toFloat()*1000;
            sprintf(command1,"s%sf+%.0f\r\n",device_num.data(),time1);
            dataSend(command1);
            sleep(200);
            clear_command();
            sleep(200);
            char command2[512];
            sprintf(command2,"s%sDI1+00000008\r\n",device_num.data());
            dataSend(command2);
            sleep(200);
            saveConf_command();
        }
    }
}

void MainWindow::on_downtodevice_1_clicked()
{
    if(ui->normal->isChecked())
    {
        clear_command();
        sleep(200);
        char command[512];
        sprintf(command,"s%smc+0\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
    if(ui->fast->isChecked())
    {
        clear_command();
        sleep(200);
        char command[512];
        sprintf(command,"s%smc+1\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
    if(ui->precise->isChecked())
    {
        clear_command();
        sleep(200);
        char command[512];
        sprintf(command,"s%smc+2\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
    if(ui->timing_measure->isChecked())
    {
        clear_command();
        sleep(200);
        char command[512];
        sprintf(command,"s%smc+3\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
    if(ui->move_target->isChecked())
    {
        clear_command();
        sleep(200);
        char command[512];
        sprintf(command,"s%smc+4\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
}

void MainWindow::on_downtodevice_2_clicked()
{
    if(ui->filter_close->isChecked())
    {
        clear_command();
        sleep(200);
        char command[512];
        sprintf(command,"s%sfi+0+0+0\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
    else if(ui->filter_open->isChecked())
    {
        double _data1 = ui->filter_length->text().toInt();
        double _data2 = ui->filter_extremevalue->text().toInt();
        double _data3 = ui->filter_error->text().toInt();
        if(_data1 * 0.4 >= _data2*2 + _data3)
        {
            clear_command();
            sleep(200);
            QByteArray data1 = ui->filter_length->text().toLatin1();
            QByteArray data2 = ui->filter_extremevalue->text().toLatin1();
            QByteArray data3 = ui->filter_error->text().toLatin1();
            char command[512];
            sprintf(command,"s%sfi+%s+%s+%s\r\n",device_num.data(),data1.data(),data2.data(),data3.data());
            dataSend(command);
            sleep(200);
            saveConf_command();
        }
        else
        {
            QMessageBox::about(NULL,"提示","下载是有一个错误:\n"
                                         "过滤器参数设置不正确.它们必须满足:\n"
                                         "0.4倍过滤长度大于或等于被过滤掉的峰值数量加上错误数量之和的两倍");
        }
    }
}

void MainWindow::on_downtodevice_3_clicked()
{
    clear_command();
    sleep(200);
    if(ui->mA_0->isChecked())
    {
        //发送sNvm+a指令，设置模拟输出最小电平
        char command1[512];
        sprintf(command1,"s%svm+0\r\n",device_num.data());
        dataSend(command1);
        sleep(200);
    }
    else if(ui->mA_4->isChecked())
    {
        //发送sNvm+a指令，设置模拟输出最小电平
        char command2[512];
        sprintf(command2,"s%svm+1\r\n",device_num.data());
        dataSend(command2);
        sleep(200);
    }
    //发送sNve+aaa指令，在错误情况下设置模拟输出值
    if(ui->replace_value->isChecked())
    {
        char command3[512];
        QString temp_error = QString("%1").arg(ui->error_current->text().toDouble()*10,3,'g',3,QLatin1Char('0'));
        QByteArray error = temp_error.toLatin1();
        sprintf(command3,"s%sve+%s\r\n",device_num.data(),error.data());
        dataSend(command3);
        sleep(200);
    }
    else if(ui->final_value->isChecked())
    {
        char command3[512];
        sprintf(command3,"s%sve+999\r\n",device_num.data());
        dataSend(command3);
        sleep(200);
    }
    //发送sNv指令，获取模拟输出距离范围
    QString temp_max = QString("%1").arg(ui->max_current->text().toInt()*10,8,10,QLatin1Char('0'));
    QString temp_min = QString("%1").arg(ui->min_current->text().toInt()*10,8,10,QLatin1Char('0'));
    char command4[512];
    QByteArray max = temp_max.toLatin1();
    QByteArray min = temp_min.toLatin1();
    sprintf(command4,"s%sv+%s+%s\r\n",device_num.data(),min.data(),max.data());
    dataSend(command4);
    sleep(200);
    saveConf_command();
}

void MainWindow::on_downtodevice_4_clicked()
{
    clear_command();
    sleep(200);
    int _data1_open = ui->digital_sN1Open->text().toDouble()*10;
    int _data1_close = ui->digital_sN1Close->text().toDouble()*10;
    QString temp_data1_open = QString("%1").arg(_data1_open,8,10,QLatin1Char('0'));
    QString temp_data1_close = QString("%1").arg(_data1_close,8,10,QLatin1Char('0'));
    QByteArray data1_open = temp_data1_open.toLatin1();
    QByteArray data1_close = temp_data1_close.toLatin1();
    char command1[512];
    sprintf(command1,"s%s1+%s+%s\r\n",device_num.data(),data1_open.data(),data1_close.data());
    dataSend(command1);
    sleep(200);

    int _data2_open = ui->digital_sN2Open->text().toDouble()*10;
    int _data2_close = ui->digital_sN2Close->text().toDouble()*10;
    QString temp_data2_open = QString("%1").arg(_data2_open,8,10,QLatin1Char('0'));
    QString temp_data2_close = QString("%1").arg(_data2_close,8,10,QLatin1Char('0'));
    QByteArray data2_open = temp_data2_open.toLatin1();
    QByteArray data2_close = temp_data2_close.toLatin1();
    char command2[512];
    sprintf(command2,"s%s2+%s+%s\r\n",device_num.data(),data2_open.data(),data2_close.data());
    dataSend(command2);
    sleep(200);

    if(ui->low_level->isChecked())
    {
        char command[512];
        sprintf(command,"s%sot+0\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
    if(ui->high_level->isChecked())
    {
        char command[512];
        sprintf(command,"s%sot+1\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
    if(ui->pushpull_output->isChecked())
    {
        char command[512];
        sprintf(command,"s%sot+2\r\n",device_num.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
    }
}

void MainWindow::on_downtodevice_5_clicked()
{
    int data1,data2,data3,data4;
    clear_command();
    sleep(200);
    if(ui->SSI->isChecked())
    {
        if(ui->error_replace_value->isChecked())
        {
            char command[512];
            QByteArray data1 = ui->error_distance_output->text().toLatin1();
            sprintf(command,"s%sSSIe+%s\r\n",device_num.data(),data1.data());
            dataSend(command);
            sleep(200);
        }
        else if(ui->error_lasted_value->isChecked())
        {
            char command[512];
            sprintf(command,"s%sSSIe-1\r\n",device_num.data());
            dataSend(command);
            sleep(200);
        }
        else if(ui->error_code->isChecked())
        {
            char command[512];
            sprintf(command,"s%sSSIe-2\r\n",device_num.data());
            dataSend(command);
            sleep(200);
        }
        if(ui->binary->isChecked()) data1=0;
        if(ui->graycode->isChecked())   data1=1;
        if(ui->additional_errorbits1->isChecked())  data2=1;else data2=0;
        if(ui->additional_errorbits2->isChecked())  data3=1;else data3=0;
        if(ui->measure24bits->isChecked())  data4=0;
        if(ui->measure23bits->isChecked())  data4=1;
        int _data = 1 + data1*2 + data2*4 + data3*8 + data4*16 ;
        QString temp_data = QString::number(_data);
        QByteArray send_data = temp_data.toLatin1();
        char command[512];
        sprintf(command,"s%sSSI+%s\r\n",device_num.data(),send_data.data());
        dataSend(command);
        sleep(200);
        saveConf_command();
        QMessageBox::about(NULL,"建议","SSI已被激活\n当设备是通过RS422连接的时候不要使用这个应用\n否则当外部的SSI"
                                     "主机使用运行的时候,传感器会出现工作紊乱");
    }
    if(ui->RS422->isChecked())
    {
        char command1[512];
        sprintf(command1,"s%sSSI+0\r\n",device_num.data());
        dataSend(command1);
        sleep(200);

        char command2[512];
        sprintf(command2,"s%sSSIe+0\r\n",device_num.data());
        dataSend(command2);
        sleep(200);

        saveConf_command();
        QMessageBox::about(NULL,"建议","现在断开SSI设备");
    }
}

void MainWindow::on_downtodevice_7_clicked()
{
    clear_command();
    sleep(200);
    char command[512];
    QByteArray data1 = ui->comboBox_deviceNewID->currentText().toLatin1();
    sprintf(command,"s%sid+%s\r\n",device_num.data(),data1.data());
    dataSend(command);
    sleep(200);
    device_num = data1;
    ui->comboBox_deviceID->setCurrentText(device_num);
    saveConf_command();
}

void MainWindow::on_measure24bits_clicked()
{
    ui->label_errorReplaceValue->setText("(0..16777215)");
}

void MainWindow::on_measure23bits_clicked()
{
    ui->label_errorReplaceValue->setText("(0..8388607)");
}






void MainWindow::on_file_action_quit_triggered()
{
    if(serial->isOpen())
    {
        mtimer->stop();
        serial->close();
    }
    this->close();
}

void MainWindow::on_tool_action_orderManual_triggered()
{
    Hand_Command *h_command = new Hand_Command;
    h_command->show();
}




