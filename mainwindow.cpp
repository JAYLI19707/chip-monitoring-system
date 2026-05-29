#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "software_env.h"

#include <QtCharts>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QChart>
#include <QTimer>

#include <chrono>
#include <thread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init();
}

MainWindow::~MainWindow()
{
    close_sqlite(&m_sqlite_handle);
    m_update_map_timer.stop();
    m_update_warning_data_timer.stop();
    setup_warning_device(SOFTWARE_STATUS_EXIT); // 软件即将退出，设置硬件状态为退出状态
    for (int i = m_setup_status_process_list.length() - 1; i >= 0; --i) {
        if (m_setup_status_process_list.at(i)->waitForFinished()) {
            delete m_setup_status_process_list.at(i);
            m_setup_status_process_list.removeAt(i);
        }
    }
    delete ui;
}

QString MainWindow::read_dht11_data()
{
    QString file_path;
    file_path = file_select(SOFTWARE_PYTHON_READ_DHT11_SCRIPT);
    if (file_path.isEmpty())
    {
        return QString();
    }

    QProcess process;
    process.setProgram("python3"); // 设置程序为 python 解释器
    QStringList arguments; // 参数列表
    arguments << file_path; // Python 脚本路径
    process.setArguments(arguments);

    process.start(); // 启动进程
    process.waitForFinished(); // 等待进程执行完成

    QByteArray output = process.readAllStandardOutput(); // 读取标准输出
    QString outputStr = QString::fromLocal8Bit(output); // 将 QByteArray 转换为 QString

    return outputStr;
}

static qint64 preUpdateTempChartTime = QDateTime::currentMSecsSinceEpoch();
void MainWindow::update_by_dht11() {
    const int refreshTempChartTimeGap = 995; // 1秒 理论上是 1000 但是有时候会有5ms的差距，理论上也应该开始采样
    const int showTempPointMaxCount = 5; // 显示5个点
    QDateTime currentDateTime;

    // 隐式函数 用于重绘曲线图
    // currentDateTime 引用捕获，这样函数内就可以直接用这个变量
    // QChart* chart, int value 是参数，和普通的函数一样
    // void 是返回值
    auto update_chart = [&currentDateTime](QChart* chart, int value) -> void {
        QLineSeries *series = static_cast<QLineSeries *>(chart->series().at(0));
        series->append(currentDateTime.toMSecsSinceEpoch(), value);

        // 超过限定的点数，为了避免重绘曲线很慢，只能删除末端的数据
        if (series->count() > showTempPointMaxCount) {
            series->removePoints(0, series->count() - showTempPointMaxCount);
        }

        // 更新X轴范围
        /*
         * chart->axisX() 已被弃用，下面是新的方法
         * Qt::Horizontal 指代X轴
         * Qt::Vertical 指代Y轴
         * .back指代最后一个数据序列，不过通常只有一个
         * 然后才是正常的 setRange 函数调用
         *
         * 由于前面已经 removePoints，那么就能保证 series->at(0) 是当前可显示的点的最旧的那个（末端的数据）
         */
        chart->axes(Qt::Horizontal).back()->setRange(QDateTime::fromMSecsSinceEpoch(series->at(0).x()),
                                                     QDateTime::fromMSecsSinceEpoch(series->at(series->count() - 1).x()));
    };

    qint64 currentUpdateTime = QDateTime::currentMSecsSinceEpoch();
    // 保证距离上次采样超过了 1s 左右
    if (currentUpdateTime - preUpdateTempChartTime >= refreshTempChartTimeGap) {
        int ret;

        QString python_run_data;
        int temperature = 0;
        int humidity = 0;

        preUpdateTempChartTime = currentUpdateTime;
        currentDateTime = QDateTime::currentDateTime(); // 获取当前时间
        python_run_data = read_dht11_data(); // 读取dht11的采样数据

        ret = analysis_string_dht11_data(python_run_data, humidity, temperature); // 分析数据是否可用
        if (ret) {
            return ;
        }

        // 更新温湿度对应显示的 label 和折线图
        this->ui->label_humidity->setText(QString::number(humidity) + "%");
        update_chart(this->m_humidity_chart, humidity);

        this->ui->label_temperature->setText(QString::number(temperature) + "°C");
        update_chart(this->m_temperature_chart, temperature);

        // 分析当前状态，是否超过阈值，设置对应硬件状态
        analysis_cur_status(humidity, temperature);
    }
}

void MainWindow::analysis_cur_status(int cur_humidity, int cur_temperature)
{
    int cur_status;
    QString sql;

    cur_status = SOFTWARE_STATUS_OK;
    cur_status += (cur_humidity > m_humidity_warning) ? SOFTWARE_STATUS_HUMIDITY_WARNING : 0;
    cur_status += (cur_temperature > m_temperature_warning) ? SOFTWARE_STATUS_TEMPERATURE_WARNING : 0;

    // 写数据到数据库
    sql = "INSERT INTO ht_data (humidity, temperature, humidity_warning, temperature_warning, status) VALUES(";
    sql += QString::number(cur_humidity);
    sql += ", ";
    sql += QString::number(cur_temperature);
    sql += ", ";
    sql += QString::number(m_humidity_warning);
    sql += ", ";
    sql += QString::number(m_temperature_warning);
    sql += ", ";
    sql += QString::number(cur_status);
    sql += ");";
    sqlite_exec(&m_sqlite_handle, sql.toLatin1().data());

    setup_warning_device(cur_status); // 设置对应硬件状态
}

void MainWindow::setup_warning_device(int status)
{
    QString file_path;
    file_path = file_select(SOFTWARE_PYTHON_SETUP_STATUS_SCRIPT);
    if (file_path.isEmpty()) {
        return ;
    }

    if (status < SOFTWARE_STATUS_OK || status > SOFTWARE_STATUS_EXIT) {
        return;
    }

    // 已完成的程序释放内存
    for (int i = m_setup_status_process_list.length() - 1; i >= 0; --i) {
        if (m_setup_status_process_list.at(i)->waitForFinished(30)) {
            delete m_setup_status_process_list.at(i);
            m_setup_status_process_list.removeAt(i);
        }
    }

    QProcess *new_process = new QProcess();;
    new_process->setProgram("python3"); // 设置程序为 python 解释器
    QStringList arguments; // 参数列表
    arguments << file_path; // 替换为你的 Python 脚本路径
    arguments << QString::number(status);
    new_process->setArguments(arguments);

    new_process->start(); // 启动进程
    m_setup_status_process_list.append(new_process);
}

QString MainWindow::file_select(QString file_name)
{
    // 文件路径
    QString file_path;

    file_path = SOFTWARE_INSTALL_PATH + file_name;
    // 判断安装路径下是否存在
    if (QFile::exists(file_path)) {
        return file_path;
    }

    file_path = file_name;
    // 先判断程序运行路径下是否存在
    if (QFile::exists(file_path)) {
        return file_path;
    }

    return QString();
}

int MainWindow::analysis_string_dht11_data(QString data, int& humidty, int& temperature)
{
    QStringList data_group;
    QStringList humidity_data_group;
    QStringList temperature_data_group;
    int temp_humidity;
    int temp_temperature;
    bool ok;

    // 数据格式如下：
    // humidity=52 temperature=26
    data_group = data.split(" ");
    if (data_group.length() != 2) {
        return 1;
    }

    humidity_data_group = data_group.at(0).split("=");
    if (humidity_data_group.length() != 2) {
        return 1;
    }
    temperature_data_group = data_group.at(1).split("=");
    if (temperature_data_group.length() != 2) {
        return 1;
    }

    temp_humidity = humidity_data_group.at(1).toInt(&ok, 10);
    if (!ok) {
        return 1;
    }
    temp_temperature = temperature_data_group.at(1).toInt(&ok, 10);
    if (!ok) {
        return 1;
    }

    humidty = temp_humidity;
    temperature = temp_temperature;

    return 0;
}

#if 0
int MainWindow::rw_warning_data_file_context_lock(int rw, QString& buffer)
{
    int ret;
    QLockFile lockFile(SOFTWARE_WARNING_DATA_LOCK_FILE);

    ret = 0;

    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;   // 写锁
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;          // 锁整个文件

    lockFile.lock();
    QFile file(SOFTWARE_WARNING_DATA_FILE);
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        if (rw) {
            QTextStream in(&file);
            ret = 1;
            if (!in.atEnd()) {
                buffer = in.readLine();
                ret = 0;
            }
        } else {
            QTextStream out(&file);
            out << buffer;
        }
        file.close();
    } else {
        ret = 1;
    }
    lockFile.unlock();
    return ret;
}
#else
#ifdef __cplusplus
extern "C" {
    #include <sys/file.h>
    #include <unistd.h>
    #include <string.h>
}
#endif

static void close_fd(int fd)
{
    close(fd);
}

int MainWindow::rw_warning_data_file_context_lock(int rw, QString& buffer)
{
    char ori_data[64];
    // 打开文件
    int fd = open(SOFTWARE_WARNING_DATA_FILE, O_RDWR | O_CREAT, 0666); // 不会清空文件，所以写入的时候，写完的时候记得写入 0 代表文件结束
    if (fd == -1) {
        return 1;
    }

    // ... 进行文件操作 ...
    if (rw) {
        int len;
        if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
            close_fd(fd);
            return 1;
        }
        len = read(fd, ori_data, 63);
        ori_data[63] = 0;
        if (ori_data[len - 1] == '\n') {
            ori_data[len - 1] = 0;
        }
        if (ori_data[len - 2] == '\r') {
            ori_data[len - 2] = 0;
        }
        len = strlen(ori_data);
        buffer = QString::fromLatin1(ori_data, strlen(ori_data));
    } else {
        flock(fd, LOCK_EX);
        memset(ori_data, 0, 1);
        write(fd, buffer.toLatin1().data(), buffer.length());
        write(fd, ori_data, 1);
    }

    // 释放锁
    flock(fd, LOCK_UN); // 解锁

    close_fd(fd);
    return 0;
}
#endif

void MainWindow::on_pushButton_exit_clicked()
{
    this->close();
}

void MainWindow::on_pushButton_humidity_update_clicked()
{
    int temp;
    QString new_limit;
    bool ok;
    temp = this->ui->lineEdit_humidity->text().toInt(&ok);
    if (!ok) {
        return ;
    }
    m_humidity_warning = temp;
    this->ui->label_cur_humidity_warning->setText(QString::number(temp) + "%");
    new_limit = "humidity=" + QString::number(m_humidity_warning) + " temperature=" + QString::number(m_temperature_warning);
    rw_warning_data_file_context_lock(FILE_RW_W, new_limit);
}

void MainWindow::on_pushButton_temperature_update_clicked()
{
    int temp;
    QString new_limit;
    bool ok;
    temp = this->ui->lineEdit_temperature->text().toInt(&ok);
    if (!ok) {
        return ;
    }
    m_temperature_warning = temp;
    this->ui->label_cur_temperature_warning->setText(QString::number(temp) + "°C");
    new_limit = "humidity=" + QString::number(m_humidity_warning) + " temperature=" + QString::number(m_temperature_warning);
    rw_warning_data_file_context_lock(FILE_RW_W, new_limit);
}

void MainWindow::update_warning_limit()
{
    int ret;
    QString new_data;
    int humidity;
    int temperature;

    ret = rw_warning_data_file_context_lock(FILE_RW_R, new_data);
    if (ret) {
        return ;
    }
    ret = analysis_string_dht11_data(new_data, humidity, temperature);
    if (ret) {
        return ;
    }
    m_humidity_warning = humidity;
    m_temperature_warning = temperature;
    this->ui->label_cur_humidity_warning->setText(QString::number(humidity) + "%");
    this->ui->label_cur_temperature_warning->setText(QString::number(temperature) + "°C");
}

void MainWindow::on_lineEdit_humidity_textChanged(const QString &arg1)
{
    int limit;
    bool ok;
    QString temp;
    int num;

    (void)arg1; // 消除编译警告
    temp = this->ui->lineEdit_humidity->text();
    num = temp.toInt(&ok);

    // 如果不是整数，不要新增的最后一个字符
    if (!ok) {
        temp = temp.left(temp.length() - 1);
        this->ui->lineEdit_humidity->setText(temp);
    }

    // 如果不范围内，不要新增的最后一个字符
    limit = ((num < DHT11_HUMIDITY_MIN_LIMIT) || (num > DHT11_HUMIDITY_MAX_LIMIT)) ? 1 : 0;
    if (limit) {
        temp = temp.left(temp.length() - 1);
        this->ui->lineEdit_humidity->setText(temp);
    }
}

void MainWindow::on_lineEdit_temperature_textChanged(const QString &arg1)
{
    int limit;
    bool ok;
    QString temp;
    int num;

    (void)arg1; // 消除编译警告
    temp = this->ui->lineEdit_temperature->text();
    num = temp.toInt(&ok);

    // 如果不是整数，不要新增的最后一个字符
    if (!ok) {
        temp = temp.left(temp.length() - 1);
        this->ui->lineEdit_temperature->setText(temp);
    }

    // 如果不范围内，不要新增的最后一个字符
    limit = ((num < DHT11_TEMPERATURE_MIN_LIMIT) || (num > DHT11_TEMPERATURE_MAX_LIMIT)) ? 1 : 0;
    if (limit) {
        temp = temp.left(temp.length() - 1);
        this->ui->lineEdit_temperature->setText(temp);
    }
}
