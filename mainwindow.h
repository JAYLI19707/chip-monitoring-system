#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts>
#include <QLineSeries>
#include <QChart>

#ifdef __cplusplus
extern "C" {
    #include "sqlite_op.h"
}
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    /*
     * 函数声明区域
     */
    void init(); // 窗口初始化函数，调用一众初始化函数
    void init_read_warning_data(); // 读取文件，获取温湿度阈值
    void init_humidity_charts(); // 初始化创建湿度的曲线图
    void init_temperature_charts(); // 初始化创建温度的曲线图
    void init_sqlite_db();

    // 创建曲线图的通用函数 用于 init_humidity_charts init_temperature_charts
    QChart* init_general_charts(QString target, QString target_unit, int x_count, int y_count, qreal y_min, qreal y_max);

    QString read_dht11_data(); // 获取dht11的温湿度数据，返回的字符串格式 "humidity=20 temperature=30"
    void analysis_cur_status(int cur_humidity, int cur_temperature); // 分析当前状态，是否出发了阈值
    void setup_warning_device(int status); // 根据当前状态来设置硬件状态
    QString file_select(QString file_name); // 文件选择函数判断 SOFTWARE_INSTALL_PATH 是否存在
    int analysis_string_dht11_data(QString data, int& humidty, int& temperature); // 分析温湿度数据字符串是否合理，能否从中获取温湿度数据
    int rw_warning_data_file_context_lock(int rw, QString& buffer); // 读写温湿度阈值记录文件，带锁

    /*
     * 变量声明区域
     */
    Ui::MainWindow *ui;

    int m_humidity_warning; // 湿度的阈值
    QChart* m_humidity_chart; // 湿度的曲线图
    int m_temperature_warning; // 温度的阈值
    QChart* m_temperature_chart; // 温度的曲线图

    QList<QProcess*> m_setup_status_process_list; // 设置硬件状态进程，通常是调用python脚本，控制外设

    QTimer m_update_map_timer;
    QTimer m_update_warning_data_timer;

    sqlite_db m_sqlite_handle;
private slots:
    void update_by_dht11(); // 采集定时器的触发函数，槽函数
    void update_warning_limit(); // 阈值更新定时器的触发函数，槽函数读取文件，尝试更新阈值
    void on_pushButton_exit_clicked(); // 退出按钮点击之后的触发函数，槽函数
    void on_pushButton_humidity_update_clicked(); // 更新湿度阈值的按钮点击之后的触发函数，槽函数
    void on_pushButton_temperature_update_clicked(); // 更新温度阈值的按钮点击之后的触发函数，槽函数
    void on_lineEdit_humidity_textChanged(const QString &arg1);
    void on_lineEdit_temperature_textChanged(const QString &arg1);
};
#endif // MAINWINDOW_H
