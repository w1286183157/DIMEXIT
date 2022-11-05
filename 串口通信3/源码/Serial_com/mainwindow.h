#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTime>
#include "hand_command.h"
#include <QMessageBox>
#include <QTimer>
#include <stdio.h>
#include "tabbarstyle.h"
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void init_Page();   //初始化开始界面中各个部件
    void inited_Page();  //连接成功后设置一些部件可用

    void init_command();    //初始化命令
    void clear_command();  //清除命令 -> 停止当前执行并重置状态
    void saveConf_command();  //保存配置命令 -> sNs
    void dataSend(char *);  //发送数据

    void sNgRetHand(char *);
    /*sNh距离测量 指令的处理函数*/
    void sNhRetHand(char*);
    /*读取缓存区数据*/
    void sNqRetHand(char *);
    /*获取数字输入(sNRI)*/
    void sNRIRetHand(char *);
    /*处理sNsv软件版本返回信息*/
    void sNsvRetHand(char *);
    /*处理sNsn软件版本返回信息*/
    void sNsnRetHand(char *);
    void sleep(int msec);
    int serialPortPoll();

private slots:
    void readSerialData();  //读取从自定义串口类获得的数据
    void on_pushButton_checkConnect_clicked();
    void on_pushButton_refreshPort_clicked();   //端口刷新
    void on_pushButton_conFailed_clicked();  //断开连接
    void on_distance_measure_clicked();  //测量距离
    void on_sensor_start_clicked();   //定时测量距离开始
    void on_sensor_stop_clicked();    //定时测量距离停止
    void on_pushButton_recordClear_clicked();  //通讯记录清除
    void on_buffer_start_clicked();     //缓冲区跟踪测量开始
    void on_buffer_stop_clicked();      //缓冲区跟踪测量停止
    void on_manual_read_clicked();  //手动读取数据
    void on_timing_start_clicked();     //定时读数开始
    void on_timing_stop_clicked();      //定时读数结束
    void on_automeasure_writemodule_clicked();  //主动模式 ->自动测量配置 -> 写入到模组按钮
    void on_manualreading_button_clicked();     //主动模式 ->自动测量配置 -> 读数
    void on_timedreadData_start_clicked();      //主动模式 ->自动测量配置 -> 定时读数开始
    void on_timedreadData_stop_clicked();       //主动模式 ->自动测量配置 -> 定时读数结束
    void on_automeasure_open_stateChanged(int); //主动模式 ->自动测量配置 -> 是否开启自动测量
    void on_notActivate_clicked();  //主动模式 ->手动触发测量配置 -> 未激活
    void on_activate_clicked();     //主动模式 ->手动触发测量配置 -> 激活
    void on_read_DI1_button_clicked(); //主动模式 -> 手动触发测量配置 -> 获取DI1信
    void on_RS422_clicked();    //配置 -> SSI -> RS422打开
    void on_SSI_clicked();      //配置 -> SSI -> SSI打开  
    void on_filter_close_clicked();     //配置 -> 过滤器 -> 关闭
    void on_filter_open_clicked();      //配置 -> 过滤器 -> 打开
    void on_downtodevice_8_clicked();   //主动模式 -> 手动触发测量配置
    void on_downtodevice_1_clicked();   //配置 -> 测量参数
    void on_downtodevice_2_clicked();   //配置 -> 过滤器
    void on_downtodevice_3_clicked();   //配置 -> 模拟量输出
    void on_downtodevice_4_clicked();   //配置 -> 数字量输出
    void on_downtodevice_5_clicked();   //配置 -> SSI
    void on_downtodevice_7_clicked();   //配置 -> DeviceID
    void on_measure24bits_clicked();
    void on_measure23bits_clicked();

    void on_file_action_quit_triggered();   //退出
    void on_tool_action_orderManual_triggered();  //工具 -> 手动输入命令



private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    QStringList serialStrList;
    QTimer *buffer_time;  //被动模式 -> 测量 -> 缓冲区跟踪测量
    QTimer *timing_time;  //被动模式 -> 测量 -> 定时读数
    QTimer *automeasure_time;  //主动模式 -> 自动测量配置
    QTimer *mtimer;
    QByteArray device_num;  //设备地址号
    int automeasure_flag=0;
    bool CurrectMode=false;     //收到返回结果状态位
    int connect_flag=0;
    int index;
};
#endif // MAINWINDOW_H
