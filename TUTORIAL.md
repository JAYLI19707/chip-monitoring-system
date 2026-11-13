<p align="center">
  <strong>简体中文</strong> |
  <a href="TUTORIAL.en.md">English</a>
</p>

# 智慧监控与数据采集系统 —— 项目教程

> 本教程面向初学者，从零开始讲解这个项目的**整体功能、每个文件的作用、核心代码逻辑，以及各模块如何拼接成一个完整的系统**。

---

## 目录

1. [这个项目到底做了什么？](#1-这个项目到底做了什么)
2. [硬件与运行环境](#2-硬件与运行环境)
3. [项目文件总览](#3-项目文件总览)
4. [核心文件逐一详解](#4-核心文件逐一详解)
   - [4.1 software_env.h —— 全局配置中心](#41-software_envh--全局配置中心)
   - [4.2 main.cpp —— 程序入口](#42-maincpp--程序入口)
   - [4.3 mainwindow.h —— 主窗口类声明](#43-mainwindowh--主窗口类声明)
   - [4.4 mainwindow_init.cpp —— 初始化一切](#44-mainwindow_initcpp--初始化一切)
   - [4.5 mainwindow.cpp —— 核心业务逻辑](#45-mainwindowcpp--核心业务逻辑)
   - [4.6 mainwindow.ui —— 界面布局](#46-mainwindowui--界面布局)
   - [4.7 sqlite_op.h / sqlite_op.c —— 数据库工具库](#47-sqlite_oph--sqlite_opc--数据库工具库)
   - [4.8 tool.h —— 工具宏](#48-toolh--工具宏)
   - [4.9 supervision_system_demo.pro —— 构建配置](#49-supervision_system_demopro--构建配置)
5. [数据流：从传感器到屏幕的完整旅程](#5-数据流从传感器到屏幕的完整旅程)
6. [各模块如何拼接成一个项目](#6-各模块如何拼接成一个项目)
7. [关键设计思路总结](#7-关键设计思路总结)

---

## 1. 这个项目到底做了什么？

想象一个**智能温湿度监控器**，它的工作流程是：

```
┌─────────────┐    ┌──────────────┐    ┌─────────────┐    ┌─────────────┐
│  DHT11 传感器 │───>│  Python 脚本  │───>│  C++ 主程序  │───>│  Qt 界面显示  │
│  (采集数据)   │    │  (读取 GPIO)  │    │  (分析判断)  │    │  (图表+数字)  │
└─────────────┘    └──────────────┘    └──────┬──────┘    └─────────────┘
                                              │
                                    ┌─────────┼─────────┐
                                    v         v         v
                              ┌─────────┐ ┌───────┐ ┌──────────┐
                              │ SQLite  │ │ LED灯  │ │ 蜂鸣器    │
                              │ 数据库存储│ │ 状态指示│ │ 超限报警  │
                              └─────────┘ └───────┘ └──────────┘
```

**用一句话概括**：每隔1秒读一次温湿度传感器，显示在屏幕上，如果超过阈值就亮灯+响蜂鸣器报警，同时把数据存进数据库。

### 具体功能点：

| 功能 | 说明 |
|------|------|
| **实时采集** | 每秒通过 DHT11 传感器读取温度和湿度 |
| **界面展示** | 用 Qt5 图形界面显示当前数值 + 实时折线图 |
| **阈值报警** | 用户可设置温度/湿度上限，超过就触发报警 |
| **LED 指示** | 绿灯=正常，黄灯=温度过高，红灯=湿度过高，红+黄=都过高 |
| **蜂鸣器** | 超限时蜂鸣器响，正常时关闭 |
| **数据存储** | 每次采集的数据都写入 SQLite 数据库 |
| **触摸屏支持** | 支持触摸屏操作，带虚拟键盘 |

---

## 2. 硬件与运行环境

```
硬件平台：龙芯 2K300 开发板（MIPS 架构）
操作系统：OpenHarmony Linux（定制版，支持 Qt5）
传感器：  DHT11 温湿度传感器（接在 Pi HAT 板上）
外设：    绿色 LED、黄色 LED、红色 LED、蜂鸣器
开发语言：C++（主程序） + Python（硬件控制脚本）
GUI 框架：Qt5（含 Charts 模块）
数据库：  SQLite3
```

**为什么用两种语言？**
- **C++** 负责界面显示、数据分析、数据库操作 —— 这些是 Qt 框架擅长的事
- **Python** 负责直接操作 GPIO 引脚读传感器、控制 LED 和蜂鸣器 —— Python 的 `RPi.GPIO` 库用起来最简单

两者通过 **QProcess（子进程）** 连接：C++ 程序调用 `python3 xxx.py`，Python 脚本执行完把结果打印到标准输出，C++ 再读取。

---

## 3. 项目文件总览

```
chipchip-master/
|-- main.cpp                    <-- 程序入口（12行，启动 Qt 应用）
|-- mainwindow.h                <-- 主窗口类声明（72行，定义有哪些功能）
|-- mainwindow.cpp              <-- 主窗口核心逻辑（438行，所有业务代码）
|-- mainwindow_init.cpp         <-- 初始化代码（150行，启动时的准备工作）
|-- mainwindow.ui               <-- 界面布局文件（XML，Qt Designer 生成）
|-- software_env.h              <-- 全局配置常量（28行，所有魔法数字的来源）
|-- supervision_system_demo.pro <-- Qt 项目文件（构建配置）
|-- common/
|   |-- sqlite_op/
|   |   |-- sqlite_op.h         <-- 数据库操作函数声明
|   |   |-- sqlite_op.c         <-- 数据库操作函数实现
|   |-- tool/
|       |-- tool.h              <-- 工具宏定义
|-- README.md                   <-- 项目说明文档
|-- 可执行程序和系统/             <-- 预编译的系统镜像和部署包
```

**文件大小对比（代码行数）**：

```
mainwindow.cpp         ████████████████████████████████████████  438行  <-- 最核心
mainwindow_init.cpp    ██████████████                            150行
sqlite_op.c            ████████████                              234行
sqlite_op.h            ████████                                  94行
mainwindow.h           ████████                                  72行
mainwindow.ui          █████████                                 165行
software_env.h         ███                                       28行
main.cpp               ██                                        12行
tool.h                 █                                         13行
```

---

## 4. 核心文件逐一详解

### 4.1 software_env.h —— 全局配置中心

**作用**：整个项目的"配置文件"，所有重要的常量都在这里定义。

```cpp
// 文件路径配置
#define SOFTWARE_INSTALL_PATH "/opt/supervision_system_demo/"  // 程序安装目录
#define SOFTWARE_PYTHON_READ_DHT11_SCRIPT "python_dht11_read.py"      // 读传感器的脚本
#define SOFTWARE_PYTHON_SETUP_STATUS_SCRIPT "python_status_setup.py"   // 控制LED/蜂鸣器的脚本
#define SOFTWARE_WARNING_DATA_FILE "humidity_temperature_warning_data" // 阈值保存文件
#define SOFTWARE_SQLITE_DB "ss_sample_value.db"                        // 数据库文件名

// 状态码定义（传递给 Python 脚本的参数）
#define SOFTWARE_STATUS_OK 0                    // 一切正常 -> 绿灯
#define SOFTWARE_STATUS_HUMIDITY_WARNING 1      // 湿度超标 -> 红灯
#define SOFTWARE_STATUS_TEMPERATURE_WARNING 2   // 温度超标 -> 黄灯
#define SOFTWARE_STATUS_ALL_WARNING 3           // 都超标   -> 红灯+黄灯
#define SOFTWARE_STATUS_EXIT 4                  // 程序退出 -> 全部关闭

// 默认阈值
#define WARNING_DEFAULT_HUMIDITY 20       // 默认湿度阈值 20%
#define WARNING_DEFAULT_TEMPERATURE 40    // 默认温度阈值 40°C

// 传感器量程限制
#define DHT11_HUMIDITY_MAX_LIMIT 90       // 湿度最大 90%
#define DHT11_TEMPERATURE_MAX_LIMIT 50    // 温度最大 50°C
```

**为什么单独一个文件？**
> 把所有"魔法数字"集中管理。如果以后要改阈值、改文件路径、改状态码，只需要改这一个文件，不用到处找。这是嵌入式开发的最佳实践。

---

### 4.2 main.cpp —— 程序入口

**作用**：整个程序从这里开始执行，只有 12 行。

```cpp
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // 启用虚拟键盘（触摸屏输入用）
    qputenv("QT_IM_MODULE", QByteArray("Qt5Input"));

    QApplication a(argc, argv);  // 创建 Qt 应用
    MainWindow w;                // 创建主窗口
    w.show();                    // 显示窗口
    return a.exec();             // 进入事件循环（等待用户操作）
}
```

**逐行解读**：
1. `qputenv(...)` — 设置环境变量，让 Qt 知道要用虚拟键盘（嵌入式触摸屏没有实体键盘）
2. `QApplication a(argc, argv)` — Qt 应用对象，管理整个程序的生命周期
3. `MainWindow w` — 创建主窗口对象，**构造函数里会调用 init() 初始化一切**
4. `w.show()` — 把窗口显示出来
5. `a.exec()` — 进入**事件循环**，程序在这里"转圈"等待事件（定时器触发、按钮点击等），直到用户关闭窗口

---

### 4.3 mainwindow.h —— 主窗口类声明

**作用**：定义主窗口"有什么"——成员变量和成员函数。

```cpp
class MainWindow : public QMainWindow
{
    Q_OBJECT  // Qt 的宏，让这个类支持信号和槽机制

public:
    MainWindow(QWidget *parent = nullptr);  // 构造函数
    ~MainWindow();                          // 析构函数

private:
    // ========== 函数声明 ==========
    void init();                           // 总初始化函数
    void init_read_warning_data();         // 读取阈值文件
    void init_humidity_charts();           // 创建湿度图表
    void init_temperature_charts();        // 创建温度图表
    void init_sqlite_db();                 // 初始化数据库

    QString read_dht11_data();             // 读取 DHT11 传感器
    void analysis_cur_status(int cur_humidity, int cur_temperature);  // 分析是否超标
    void setup_warning_device(int status); // 控制 LED 和蜂鸣器

    // ========== 变量声明 ==========
    Ui::MainWindow *ui;           // 界面指针（指向 .ui 文件生成的界面）

    int m_humidity_warning;       // 湿度阈值
    int m_temperature_warning;    // 温度阈值
    QChart* m_humidity_chart;     // 湿度折线图
    QChart* m_temperature_chart;  // 温度折线图

    QList<QProcess*> m_setup_status_process_list;  // Python 子进程列表
    QTimer m_update_map_timer;           // 1秒定时器（读传感器）
    QTimer m_update_warning_data_timer;  // 200ms定时器（检查阈值变化）

    sqlite_db m_sqlite_handle;   // 数据库连接句柄

private slots:
    void update_by_dht11();       // 定时器触发：读传感器+更新界面
    void update_warning_limit();  // 定时器触发：检查阈值文件变化
    void on_pushButton_exit_clicked();              // 退出按钮
    void on_pushButton_humidity_update_clicked();   // 更新湿度阈值按钮
    void on_pushButton_temperature_update_clicked();// 更新温度阈值按钮
};
```

**关键概念解释**：

| 概念 | 通俗解释 |
|------|---------|
| `Q_OBJECT` | 告诉 Qt "这个类要用信号和槽"，类似于"这个类支持事件处理" |
| `private slots` | "槽函数"——当某个事件发生时（定时器响了、按钮被点），Qt 自动调用这些函数 |
| `QTimer` | 定时器，每隔 N 毫秒触发一次对应的槽函数 |
| `QProcess` | 子进程管理器，用来调用外部的 Python 脚本 |
| `QChart*` | 指向折线图对象的指针 |
| `sqlite_db` | 自定义的数据库连接结构体 |

---

### 4.4 mainwindow_init.cpp —— 初始化一切

**作用**：程序启动时的"准备工作"，把所有东西都设置好。

#### init() —— 总初始化函数

```cpp
void MainWindow::init()
{
    // 1. 隐藏不需要的菜单栏和状态栏
    this->ui->menubar->hide();
    this->ui->statusbar->hide();

    // 2. 依次初始化各个模块
    init_read_warning_data();      // 读取阈值
    init_humidity_charts();        // 创建湿度图表
    init_temperature_charts();     // 创建温度图表
    init_sqlite_db();              // 初始化数据库

    // 3. 设置输入框的数值范围限制（只能输入传感器量程内的数字）
    auto update_lineedit_limit = [](QLineEdit* lineedit, int min, int max) {
        QIntValidator* intValidator = new QIntValidator(min, max, lineedit);
        lineedit->setValidator(intValidator);
    };
    update_lineedit_limit(ui->lineEdit_humidity, 0, 90);     // 湿度 0~90
    update_lineedit_limit(ui->lineEdit_temperature, 0, 50);  // 温度 0~50

    // 4. 在界面上显示初始阈值
    ui->label_cur_humidity_warning->setText(QString::number(m_humidity_warning) + "%");
    ui->label_cur_temperature_warning->setText(QString::number(m_temperature_warning) + "°C");

    // 5. 启动两个定时器
    // 定时器1：每1秒读一次传感器
    connect(&m_update_map_timer, &QTimer::timeout, this, &MainWindow::update_by_dht11);
    m_update_map_timer.start(1000);

    // 定时器2：每200ms检查一次阈值文件是否被外部修改
    connect(&m_update_warning_data_timer, &QTimer::timeout, this, &MainWindow::update_warning_limit);
    m_update_warning_data_timer.start(200);

    // 6. 立即执行一次传感器读取（不等定时器）
    update_by_dht11();
}
```

#### init_read_warning_data() —— 读取阈值

```cpp
void MainWindow::init_read_warning_data()
{
    // 设置默认值
    m_humidity_warning = 20;     // 默认湿度阈值 20%
    m_temperature_warning = 40;  // 默认温度阈值 40°C

    // 尝试从文件读取阈值
    QString file_data;
    int ret = rw_warning_data_file_context_lock(FILE_RW_R, file_data);

    // 解析文件内容（格式: "humidity=20 temperature=40"）
    ret += analysis_string_dht11_data(file_data, m_humidity_warning, m_temperature_warning);

    // 如果读取失败（文件不存在或格式错误），就用默认值写入文件
    if (ret) {
        file_data = "humidity=20 temperature=40";
        rw_warning_data_file_context_lock(FILE_RW_W, file_data);
    }
}
```

#### init_sqlite_db() —— 初始化数据库

```cpp
void MainWindow::init_sqlite_db()
{
    // 打开（或创建）数据库文件
    QString db_path = file_select(SOFTWARE_SQLITE_DB);
    open_sqlite(&m_sqlite_handle, db_path.toLatin1().data());

    // 创建数据表（如果不存在）
    // 表结构：id | humidity | temperature | humidity_warning | temperature_warning | status
    sqlite_exec(&m_sqlite_handle,
        "CREATE TABLE IF NOT EXISTS ht_data "
        "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "humidity INTEGER,"
        "temperature INTEGER,"
        "humidity_warning INTEGER,"
        "temperature_warning INTEGER,"
        "status INTEGER);");
}
```

#### init_general_charts() —— 创建折线图

```cpp
QChart* MainWindow::init_general_charts(QString target, QString target_unit,
                                         int x_count, int y_count,
                                         qreal y_min, qreal y_max)
{
    // 创建图表对象
    QChart* chart = new QChart();
    chart->setTitle(target + "变化图");          // 标题，如 "湿度变化图"
    chart->setAnimationOptions(QChart::SeriesAnimations);  // 开启动画

    // X 轴：时间轴
    QDateTimeAxis *dateTimeAxisX = new QDateTimeAxis;
    dateTimeAxisX->setTickCount(x_count);        // 刻度数量
    dateTimeAxisX->setFormat("hh:mm:ss");        // 时间格式
    dateTimeAxisX->setTitleText("时间");

    // Y 轴：数值轴
    QValueAxis *valueAxisY = new QValueAxis;
    valueAxisY->setTickCount(y_count);
    valueAxisY->setRange(y_min, y_max);          // 数值范围
    valueAxisY->setTitleText(target + "(" + target_unit + ")");

    // 数据线
    QLineSeries *series = new QLineSeries();
    series->setName(target);

    // 组装：把数据线和坐标轴都加到图表上
    chart->addSeries(series);
    chart->addAxis(dateTimeAxisX, Qt::AlignBottom);
    chart->addAxis(valueAxisY, Qt::AlignLeft);
    series->attachAxis(dateTimeAxisX);
    series->attachAxis(valueAxisY);

    // 创建图表视图并添加到界面布局
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);  // 抗锯齿
    this->ui->horizontalLayout_charts->addWidget(chartView);

    return chart;
}
```

---

### 4.5 mainwindow.cpp —— 核心业务逻辑

这是**最重要的文件**，包含了所有业务逻辑。

#### read_dht11_data() —— 读传感器

```cpp
QString MainWindow::read_dht11_data()
{
    // 1. 找到 Python 脚本的路径
    QString file_path = file_select(SOFTWARE_PYTHON_READ_DHT11_SCRIPT);
    if (file_path.isEmpty()) return QString();

    // 2. 创建子进程，执行 "python3 python_dht11_read.py"
    QProcess process;
    process.setProgram("python3");
    QStringList arguments;
    arguments << file_path;
    process.setArguments(arguments);

    // 3. 启动并等待执行完成
    process.start();
    process.waitForFinished();

    // 4. 读取 Python 脚本的输出（标准输出）
    QByteArray output = process.readAllStandardOutput();
    return QString::fromLocal8Bit(output);
    // 返回格式: "humidity=52 temperature=26"
}
```

**通俗理解**：
> 就像你在命令行输入 `python3 python_dht11_read.py`，然后读取它打印出来的文字。Python 脚本会读取传感器，然后 print("humidity=52 temperature=26")。

#### update_by_dht11() —— 每秒执行一次的核心函数

```cpp
void MainWindow::update_by_dht11()
{
    // 用一个 lambda 函数来更新折线图
    auto update_chart = [&currentDateTime](QChart* chart, int value) {
        QLineSeries *series = static_cast<QLineSeries*>(chart->series().at(0));
        series->append(currentDateTime.toMSecsSinceEpoch(), value);  // 添加数据点

        // 最多保留 5 个点，超出就删除最旧的
        if (series->count() > 5) {
            series->removePoints(0, series->count() - 5);
        }

        // 更新 X 轴范围（让时间轴跟着数据走）
        chart->axes(Qt::Horizontal).back()->setRange(
            QDateTime::fromMSecsSinceEpoch(series->at(0).x()),
            QDateTime::fromMSecsSinceEpoch(series->at(series->count() - 1).x()));
    };

    // 确保距离上次采样超过 1 秒
    if (当前时间 - 上次采样时间 >= 995毫秒) {
        // 1. 读取传感器
        QString python_run_data = read_dht11_data();

        // 2. 解析数据（从 "humidity=52 temperature=26" 中提取数字）
        int humidity, temperature;
        if (analysis_string_dht11_data(python_run_data, humidity, temperature)) {
            return;  // 解析失败，跳过本次
        }

        // 3. 更新界面数字
        ui->label_humidity->setText(QString::number(humidity) + "%");
        ui->label_temperature->setText(QString::number(temperature) + "°C");

        // 4. 更新折线图
        update_chart(m_humidity_chart, humidity);
        update_chart(m_temperature_chart, temperature);

        // 5. 分析状态，写数据库，控制硬件
        analysis_cur_status(humidity, temperature);
    }
}
```

#### analysis_cur_status() —— 判断是否超标

```cpp
void MainWindow::analysis_cur_status(int cur_humidity, int cur_temperature)
{
    // 计算状态码（用加法组合）
    int cur_status = SOFTWARE_STATUS_OK;  // 0

    // 如果湿度超标，加上 1
    cur_status += (cur_humidity > m_humidity_warning) ? SOFTWARE_STATUS_HUMIDITY_WARNING : 0;
    // 如果温度超标，加上 2
    cur_status += (cur_temperature > m_temperature_warning) ? SOFTWARE_STATUS_TEMPERATURE_WARNING : 0;

    // 状态码组合表：
    // 0 = 都没超 -> 绿灯
    // 1 = 湿度超 -> 红灯
    // 2 = 温度超 -> 黄灯
    // 3 = 都超   -> 红灯+黄灯

    // 写入数据库
    QString sql = "INSERT INTO ht_data (humidity, temperature, humidity_warning, "
                  "temperature_warning, status) VALUES(" +
        QString::number(cur_humidity) + ", " +
        QString::number(cur_temperature) + ", " +
        QString::number(m_humidity_warning) + ", " +
        QString::number(m_temperature_warning) + ", " +
        QString::number(cur_status) + ");";
    sqlite_exec(&m_sqlite_handle, sql.toLatin1().data());

    // 控制硬件
    setup_warning_device(cur_status);
}
```

**状态码设计的巧妙之处**：
> 用 0、1、2、3 四个数字表示四种状态，而且 3 = 1 + 2（湿度超 + 温度超）。这样用简单的加法就能组合出所有情况，不需要复杂的 if-else 嵌套。

#### setup_warning_device() —— 控制 LED 和蜂鸣器

```cpp
void MainWindow::setup_warning_device(int status)
{
    // 1. 清理已经执行完的 Python 子进程（释放内存）
    for (int i = m_setup_status_process_list.length() - 1; i >= 0; --i) {
        if (m_setup_status_process_list.at(i)->waitForFinished(30)) {
            delete m_setup_status_process_list.at(i);
            m_setup_status_process_list.removeAt(i);
        }
    }

    // 2. 创建新的子进程，执行: python3 python_status_setup.py <status>
    QProcess *new_process = new QProcess();
    new_process->setProgram("python3");
    QStringList arguments;
    arguments << file_path << QString::number(status);  // 传入状态码
    new_process->setArguments(arguments);
    new_process->start();

    // 3. 把进程加入列表，方便后续管理
    m_setup_status_process_list.append(new_process);
}
```

**通俗理解**：
> 相当于在命令行执行 `python3 python_status_setup.py 2`，Python 脚本收到参数 2，就知道要亮黄灯（温度超标）。脚本内部用 RPi.GPIO 库来控制硬件引脚。

#### rw_warning_data_file_context_lock() —— 带锁的文件读写

```cpp
int MainWindow::rw_warning_data_file_context_lock(int rw, QString& buffer)
{
    char ori_data[64];

    // 打开文件（不存在则创建）
    int fd = open(SOFTWARE_WARNING_DATA_FILE, O_RDWR | O_CREAT, 0666);

    if (rw) {  // 读操作
        flock(fd, LOCK_EX | LOCK_NB);    // 加排他锁（不让别人同时写）
        int len = read(fd, ori_data, 63); // 读取内容
        // 去掉末尾的换行符
        if (ori_data[len-1] == '\n') ori_data[len-1] = 0;
        if (ori_data[len-2] == '\r') ori_data[len-2] = 0;
        buffer = QString::fromLatin1(ori_data);
    } else {   // 写操作
        flock(fd, LOCK_EX);               // 加排他锁
        write(fd, buffer.toLatin1().data(), buffer.length());  // 写入内容
        write(fd, "\0", 1);               // 写入结束符
    }

    flock(fd, LOCK_UN);  // 解锁
    close(fd);
    return 0;
}
```

**为什么需要文件锁？**
> 因为有两个地方会操作阈值文件：(1) 用户在界面上点"更新"按钮；(2) 200ms 定时器轮询读取。如果同时读写，数据会乱。加锁就像"上厕所锁门"——一个人在用的时候，其他人等着。

#### analysis_string_dht11_data() —— 解析传感器数据

```cpp
int MainWindow::analysis_string_dht11_data(QString data, int& humidity, int& temperature)
{
    // 输入: "humidity=52 temperature=26"
    QStringList data_group = data.split(" ");  // 按空格分割
    // 结果: ["humidity=52", "temperature=26"]

    if (data_group.length() != 2) return 1;  // 格式错误

    QStringList humidity_data_group = data_group.at(0).split("=");  // 按等号分割
    // 结果: ["humidity", "52"]

    QStringList temperature_data_group = data_group.at(1).split("=");
    // 结果: ["temperature", "26"]

    humidity = humidity_data_group.at(1).toInt();       // 52
    temperature = temperature_data_group.at(1).toInt();  // 26

    return 0;  // 成功
}
```

#### 析构函数 —— 程序退出时的清理

```cpp
MainWindow::~MainWindow()
{
    close_sqlite(&m_sqlite_handle);           // 关闭数据库
    m_update_map_timer.stop();                // 停止定时器
    m_update_warning_data_timer.stop();
    setup_warning_device(SOFTWARE_STATUS_EXIT); // 告诉硬件"我要退出了"（关闭所有LED和蜂鸣器）

    // 等待所有 Python 子进程结束
    for (int i = m_setup_status_process_list.length() - 1; i >= 0; --i) {
        if (m_setup_status_process_list.at(i)->waitForFinished()) {
            delete m_setup_status_process_list.at(i);
        }
    }

    delete ui;  // 释放界面资源
}
```

---

### 4.6 mainwindow.ui —— 界面布局

这是一个 XML 文件，由 Qt Designer 可视化设计工具生成。它定义了窗口的外观布局：

```
+-------------------------------------------------------------------+
|  当前湿度: 52%    湿度阈值: 20%    当前温度: 26°C   温度阈值: 40°C   [退出软件]  |  <-- 第一行
+-------------------------------------------------------------------+
|  湿度阈值: [____] [更新]        温度阈值: [____] [更新]                  |  <-- 第二行
+-------------------------------------------------------------------+
|                                                                   |
|  +------------------------+    +------------------------+         |
|  |    湿度变化图            |    |    温度变化图            |         |  <-- 第三行
|  |   (实时折线图)           |    |   (实时折线图)           |         |    (运行时动态添加)
|  |                        |    |                        |         |
|  +------------------------+    +------------------------+         |
|                                                                   |
+-------------------------------------------------------------------+
```

**关键 UI 元素**：

| 控件名 | 类型 | 作用 |
|--------|------|------|
| `label_humidity` | QLabel | 显示当前湿度值 |
| `label_temperature` | QLabel | 显示当前温度值 |
| `label_cur_humidity_warning` | QLabel | 显示当前湿度阈值 |
| `label_cur_temperature_warning` | QLabel | 显示当前温度阈值 |
| `lineEdit_humidity` | QLineEdit | 输入新的湿度阈值 |
| `lineEdit_temperature` | QLineEdit | 输入新的温度阈值 |
| `pushButton_humidity_update` | QPushButton | 更新湿度阈值按钮 |
| `pushButton_temperature_update` | QPushButton | 更新温度阈值按钮 |
| `pushButton_exit` | QPushButton | 退出程序按钮 |
| `horizontalLayout_charts` | QHBoxLayout | 放折线图的容器（运行时动态添加） |

---

### 4.7 sqlite_op.h / sqlite_op.c —— 数据库工具库

**作用**：对 SQLite3 C API 的一层封装，让数据库操作更简单。

#### 数据结构

```c
typedef struct sqlite_db {
    char file_name[128];   // 数据库文件路径
    sqlite3* database;     // SQLite3 连接指针
    int open_target;       // 标记是否已打开（53=已打开）
} sqlite_db;
```

#### 提供的函数

```c
// 检查数据库文件是否存在
int check_sqlite_exist(char* file_name);

// 打开/关闭数据库
int open_sqlite(sqlite_db* db, char* file_name);
int close_sqlite(sqlite_db* db);

// 执行 SQL（INSERT/UPDATE/DELETE/CREATE）
int sqlite_exec(sqlite_db* db, char* sql);

// 查询数据（SELECT）
int sqlite_get_data(sqlite_db* db, char* sql, char*** data, int* row, int* col);

// 获取最后插入的行 ID
int sqlite_get_last_id(sqlite_db* db, char* table_name, char** id);

// 清空表
int truncate_table(sqlite_db* db, char* table_name);

// 释放查询结果内存
int free_data(char** data);
```

**为什么需要封装？**
> 原始的 SQLite3 API 比较繁琐（要手动管理错误信息、内存分配等）。封装后，主程序只需要调用 `sqlite_exec(&db, "INSERT ...")` 一行代码就能完成操作，不用关心底层细节。

#### 核心函数实现示例

```c
// 执行 SQL 语句
int sqlite_exec(sqlite_db* db, char* sql)
{
    char* errmsg = NULL;
    int res = sqlite3_exec(db->database, sql, NULL, NULL, &errmsg);
    if (res) {
        printf("Error: %s\nSQL: %s\n", errmsg, sql);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// 打开数据库
int open_sqlite(sqlite_db* db, char* file_name)
{
    // 如果已经打开了，先关闭
    if (db->open_target == 53) {
        sqlite3_close(db->database);
    }

    // 保存文件名
    strncpy(db->file_name, file_name, 127);

    // 打开数据库
    int res = sqlite3_open(db->file_name, &db->database);
    if (res) {
        printf("Cannot open database: %s\n", sqlite3_errmsg(db->database));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

---

### 4.8 tool.h —— 工具宏

```c
// 如果函数返回失败，立即返回失败（简化错误处理）
#define IF_RES_FAIL_RETURN if(res == EXIT_FAILURE) { return EXIT_FAILURE; }

// 计算字符串缓冲区的实际可用长度（减去末尾的 '\0'）
#define STRING_TRUE_LEN(buf_max) (buf_max - 1)
```

**使用场景**：
```c
int sqlite_get_last_id(sqlite_db* db, char* table_name, char** id)
{
    int res = sqlite_get_data(db, sql, &data, &row, &col);
    IF_RES_FAIL_RETURN;  // 如果查询失败，直接返回，不用写 if 判断
    // ... 继续处理
}
```

---

### 4.9 supervision_system_demo.pro —— 构建配置

```qmake
# 使用哪些 Qt 模块
QT += core gui           # 基础模块
QT += charts             # 图表模块（折线图）
QT += virtualkeyboard    # 虚拟键盘模块

# C++ 标准
CONFIG += c++11

# 源文件列表
SOURCES += common/sqlite_op/sqlite_op.c main.cpp mainwindow.cpp mainwindow_init.cpp

# 头文件列表
HEADERS += common/sqlite_op/sqlite_op.h common/tool/tool.h mainwindow.h software_env.h

# 包含路径（让 #include "sqlite_op.h" 能找到文件）
INCLUDEPATH += common/sqlite_op common/tool

# 界面文件
FORMS += mainwindow.ui

# 链接 SQLite3 库
unix:!macx: LIBS += -lsqlite3
```

**这个文件的作用**：
> 告诉 Qt 的构建工具 qmake："这些是源文件、这些是头文件、这些是需要的库"。然后 qmake 会自动生成 Makefile，再用 make 编译。

---

## 5. 数据流：从传感器到屏幕的完整旅程

让我们追踪一次完整的数据采集过程：

```
时间线：
  T=0s    定时器 m_update_map_timer 触发
          |
  T=0.01s 调用 update_by_dht11()
          |
  T=0.02s 调用 read_dht11_data()
          | 创建 QProcess 子进程
  T=0.03s 执行 "python3 python_dht11_read.py"
          | Python 脚本通过 GPIO 读取 DHT11 传感器
  T=0.2s  Python 输出 "humidity=52 temperature=26" 并退出
          | QProcess 读取标准输出
  T=0.21s 调用 analysis_string_dht11_data() 解析出 humidity=52, temperature=26
          |
  T=0.22s 更新界面 label: "52%", "26°C"
          |
  T=0.23s 更新折线图：添加新数据点，删除超出5个的旧点
          |
  T=0.24s 调用 analysis_cur_status(52, 26)
          | 计算状态码：52>20(湿度超) -> +1，26<40(温度没超) -> +0，结果=1
  T=0.25s 执行 SQL: INSERT INTO ht_data VALUES(52, 26, 20, 40, 1)
          |
  T=0.26s 调用 setup_warning_device(1)
          | 创建新子进程
  T=0.27s 执行 "python3 python_status_setup.py 1"
          | Python 脚本点亮红灯（湿度超标），蜂鸣器响
  T=0.5s  Python 脚本执行完毕
          |
  T=1.0s  下一次定时器触发...（循环）
```

---

## 6. 各模块如何拼接成一个项目

### 6.1 依赖关系图

```
                    +----------------+
                    | software_env.h |  <-- 所有人都依赖它
                    | (全局常量)      |
                    +-------+--------+
                            |
              +-------------+-------------+
              v             v             v
        +-----------+ +----------+ +--------------+
        | main.cpp  | |mainwindow| |mainwindow_init|
        | (入口)    | |  .cpp    | |    .cpp      |
        +-----+-----+ +----+-----+ +------+-------+
              |             |              |
              v             v              v
        +------------------------------------------+
        |          mainwindow.h                     |
        |     (MainWindow 类声明)                   |
        +------------------+-----------------------+
                           |
                           v
        +------------------------------------------+
        |     common/sqlite_op/sqlite_op.h          |
        |     (数据库操作接口)                       |
        +------------------+-----------------------+
                           |
                           v
        +------------------------------------------+
        |     common/sqlite_op/sqlite_op.c          |
        |     (数据库操作实现)                       |
        +------------------+-----------------------+
                           |
                           v
        +------------------------------------------+
        |         common/tool/tool.h                |
        |         (工具宏)                          |
        +------------------------------------------+
```

### 6.2 编译时的拼接过程

```
源文件编译：
  main.cpp            --编译-->  main.o
  mainwindow.cpp      --编译-->  mainwindow.o
  mainwindow_init.cpp --编译-->  mainwindow_init.o
  sqlite_op.c         --编译-->  sqlite_op.o

链接：
  main.o + mainwindow.o + mainwindow_init.o + sqlite_op.o
  + libQt5Core + libQt5Gui + libQt5Charts + libQt5Widgets
  + libsqlite3
  =======================================================
  = supervision_system_demo (可执行文件)
```

### 6.3 运行时的拼接过程

```
程序启动
  |
  +---> main.cpp 创建 MainWindow
  |     |
  |     +---> 构造函数调用 init()
  |     |     |
  |     |     +---> init_read_warning_data()     <-- 读文件获取阈值
  |     |     |     +---> rw_warning_data_file_context_lock()  <-- 带锁文件读写
  |     |     |
  |     |     +---> init_humidity_charts()        <-- 创建湿度图表
  |     |     |     +---> init_general_charts()   <-- 通用图表创建
  |     |     |
  |     |     +---> init_temperature_charts()     <-- 创建温度图表
  |     |     |     +---> init_general_charts()
  |     |     |
  |     |     +---> init_sqlite_db()              <-- 初始化数据库
  |     |     |     +---> open_sqlite()           <-- 打开数据库
  |     |     |     +---> sqlite_exec(CREATE TABLE) <-- 创建表
  |     |     |
  |     |     +---> 启动两个定时器
  |     |
  |     +---> [每1秒] update_by_dht11()
  |     |     +---> read_dht11_data()             <-- 调用 Python 读传感器
  |     |     +---> analysis_string_dht11_data()  <-- 解析数据
  |     |     +---> 更新界面 labels 和 charts
  |     |     +---> analysis_cur_status()         <-- 判断状态
  |     |           +---> sqlite_exec(INSERT)     <-- 写数据库
  |     |           +---> setup_warning_device()  <-- 调用 Python 控制硬件
  |     |
  |     +---> [每200ms] update_warning_limit()
  |           +---> rw_warning_data_file_context_lock(读) <-- 检查阈值是否变化
  |
  +---> [退出时] ~MainWindow()
        +---> close_sqlite()
        +---> setup_warning_device(EXIT)          <-- 关闭所有硬件
        +---> 等待所有 Python 子进程结束
```

### 6.4 文件间的数据传递方式

| 传递方式 | 从哪里 | 到哪里 | 传什么 |
|---------|--------|--------|--------|
| **函数调用** | mainwindow.cpp | sqlite_op.c | SQL 语句 |
| **函数调用** | mainwindow_init.cpp | mainwindow.cpp | 初始化时调用各种函数 |
| **子进程 stdout** | Python 脚本 | mainwindow.cpp | "humidity=52 temperature=26" |
| **子进程参数** | mainwindow.cpp | Python 脚本 | 状态码 0/1/2/3/4 |
| **文件读写** | mainwindow.cpp | 阈值文件 | "humidity=20 temperature=40" |
| **头文件 #include** | .cpp 文件 | .h 文件 | 类型声明、函数声明 |
| **extern "C"** | mainwindow.h | sqlite_op.h | C/C++ 混合编程兼容 |

---

## 7. 关键设计思路总结

### 7.1 为什么 C++ 和 Python 混用？

| 语言 | 擅长的事 | 在本项目中的角色 |
|------|---------|----------------|
| C++ + Qt | 图形界面、事件处理、数据库 | 主程序框架 |
| Python | 硬件 GPIO 控制、快速原型 | 传感器读取、LED/蜂鸣器控制 |

**好处**：各取所长。Qt 做界面很成熟，Python 控制硬件很简单。通过子进程"搭桥"通信。

### 7.2 为什么用定时器而不是循环？

```cpp
// 错误做法：死循环
while (true) {
    read_sensor();
    update_ui();
    sleep(1);
}

// 正确做法：定时器
QTimer timer;
connect(&timer, &QTimer::timeout, this, &MainWindow::update_by_dht11);
timer.start(1000);
```

**原因**：Qt 是事件驱动的框架。死循环会阻塞事件处理，界面会"卡死"（无法响应按钮点击、无法重绘）。定时器把任务"挂"到事件循环里，每秒触发一次，其他时间界面可以正常响应。

### 7.3 为什么用文件锁？

阈值文件被两个地方操作：
1. 用户点"更新"按钮 -> 写文件
2. 200ms 定时器 -> 读文件

如果不加锁，可能出现"读到一半的数据"（比如只读了 `humidity=20 tem`）。加锁保证读写原子性。

### 7.4 为什么状态码用 0/1/2/3？

```
0 = 正常      (00 二进制) -> 绿灯
1 = 湿度超标   (01 二进制) -> 红灯
2 = 温度超标   (10 二进制) -> 黄灯
3 = 都超标     (11 二进制) -> 红灯+黄灯
```

这其实是一个**位掩码（bitmask）**设计：
- 第 0 位代表湿度（1=超标）
- 第 1 位代表温度（2=超标）

Python 脚本可以用位运算快速判断：
```python
if status & 1:  # 湿度超标
    turn_on_red_led()
if status & 2:  # 温度超标
    turn_on_yellow_led()
```

### 7.5 数据库表结构设计

```sql
CREATE TABLE ht_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 自增主键
    humidity INTEGER,                       -- 当前湿度
    temperature INTEGER,                    -- 当前温度
    humidity_warning INTEGER,               -- 当时的湿度阈值
    temperature_warning INTEGER,            -- 当时的温度阈值
    status INTEGER                          -- 当时的状态码
);
```

**为什么同时存"当前值"和"阈值"？**
> 因为阈值是可以随时修改的。如果只存当前值，以后查数据时就不知道当时是"正常"还是"超标"了。把阈值也存下来，就能完整还原历史状态。

---

## 附：快速上手指南

### 编译步骤

```bash
# 1. 确保安装了 Qt5 和 SQLite3 开发库
# Ubuntu/Debian:
sudo apt install qt5-default libqt5charts5-dev libsqlite3-dev

# 2. 用 qmake 生成 Makefile
qmake supervision_system_demo.pro

# 3. 编译
make

# 4. 运行（需要在龙芯开发板上，或有 DHT11 传感器连接）
./supervision_system_demo
```

### 文件阅读顺序建议

如果你想理解这个项目，建议按以下顺序阅读：

1. **software_env.h** —— 先知道有哪些常量和配置
2. **main.cpp** —— 知道程序从哪里开始
3. **mainwindow.h** —— 了解主窗口类有哪些成员
4. **mainwindow_init.cpp** —— 看初始化做了什么
5. **mainwindow.cpp** —— 理解核心业务逻辑
6. **sqlite_op.h / sqlite_op.c** —— 了解数据库操作细节
7. **mainwindow.ui** —— 最后看界面布局

---

> **总结**：这个项目虽然不大，但包含了嵌入式开发的典型要素——传感器数据采集、GUI 界面、数据库存储、硬件控制、跨语言调用。理解了这个项目，你就掌握了嵌入式 Linux 应用开发的基本套路。
