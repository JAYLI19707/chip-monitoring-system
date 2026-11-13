<p align="center">
  <a href="TUTORIAL.md">简体中文</a> |
  <strong>English</strong>
</p>

# Smart Monitoring and Data Acquisition System — Project Tutorial

> This tutorial is for beginners. It explains this project from the ground up: its **overall functions, the purpose of each file, the core code logic, and how the modules fit together into a complete system**.

---

## Table of Contents

1. [What does this project actually do?](#1-what-does-this-project-actually-do)
2. [Hardware and Runtime Environment](#2-hardware-and-runtime-environment)
3. [Project File Overview](#3-project-file-overview)
4. [Detailed Explanation of Core Files](#4-detailed-explanation-of-core-files)
   - [4.1 software_env.h —— Global Configuration Center](#41-software_envh--global-configuration-center)
   - [4.2 main.cpp —— Program Entry Point](#42-maincpp--program-entry-point)
   - [4.3 mainwindow.h —— Main Window Class Declaration](#43-mainwindowh--main-window-class-declaration)
   - [4.4 mainwindow_init.cpp —— Initialize Everything](#44-mainwindow_initcpp--initialize-everything)
   - [4.5 mainwindow.cpp —— Core Business Logic](#45-mainwindowcpp--core-business-logic)
   - [4.6 mainwindow.ui —— Interface Layout](#46-mainwindowui--interface-layout)
   - [4.7 sqlite_op.h / sqlite_op.c —— Database Utility Library](#47-sqlite_oph--sqlite_opc--database-utility-library)
   - [4.8 tool.h —— Utility Macros](#48-toolh--utility-macros)
   - [4.9 supervision_system_demo.pro —— Build Configuration](#49-supervision_system_demopro--build-configuration)
5. [Data Flow: The Complete Journey from Sensor to Screen](#5-data-flow-the-complete-journey-from-sensor-to-screen)
6. [How the Modules Fit Together](#6-how-the-modules-fit-together)
7. [Summary of Key Design Ideas](#7-summary-of-key-design-ideas)

---

## 1. What does this project actually do?

Imagine a **smart temperature-and-humidity monitor**; its workflow is:

```
┌─────────────┐    ┌──────────────┐    ┌─────────────┐    ┌─────────────┐
│  DHT11 sensor │───>│  Python script  │───>│  C++ main program  │───>│  Qt interface  │
│  (collect data)   │    │  (read GPIO)  │    │  (analyze and decide)  │    │  (charts + numbers)  │
└─────────────┘    └──────────────┘    └──────┬──────┘    └─────────────┘
                                              │
                                    ┌─────────┼─────────┐
                                    v         v         v
                              ┌─────────┐ ┌───────┐ ┌──────────┐
                              │ SQLite  │ │ LEDs  │ │ Buzzer    │
                              │ database storage│ │ status indication│ │ over-limit alert  │
                              └─────────┘ └───────┘ └──────────┘
```

**In one sentence**：read the temperature and humidity sensor once per second, display the values on screen, turn on the lights and sound the buzzer when a threshold is exceeded, and store the data in the database.

### Specific features:

| Feature | Description |
|------|------|
| **Real-time acquisition** | Read temperature and humidity from the DHT11 sensor every second |
| **Interface display** | Display current values and real-time line charts in a Qt5 GUI |
| **Threshold alerts** | Users can set temperature/humidity upper limits; exceeding them triggers an alert |
| **LED indicators** | Green = normal, yellow = high temperature, red = high humidity, red + yellow = both high |
| **Buzzer** | Sounds when a limit is exceeded and is off otherwise |
| **Data storage** | Every collected reading is written to the SQLite database |
| **Touchscreen support** | Supports touchscreen operation with a virtual keyboard |

---

## 2. Hardware and Runtime Environment

```
Hardware platform:Loongson 2K300 development board (MIPS architecture)
Operating system:OpenHarmony Linux (customized version with Qt5 support)
Sensor:  DHT11 temperature and humidity sensor (connected to a Pi HAT board)
Peripherals:    green, yellow, and red LEDs, plus a buzzer
Languages:C++ (main program) + Python (hardware control scripts)
GUI framework:Qt5 (including the Charts module)
Database:  SQLite3
```

**Why use two languages?**
- **C++** handles interface display, data analysis, and database operations — tasks at which Qt excels
- **Python** directly accesses GPIO pins to read the sensor and controls LEDs and the buzzer —— Python’s `RPi.GPIO` library is the simplest to use

The two are connected through **QProcess (child process)** connection:C++  program calls `python3 xxx.py`;Python  script prints the result to standard output, C++ then reads it.

---

## 3. Project File Overview

```
chipchip-master/
|-- main.cpp                    <-- Program Entry Point(12 lines;starts the Qt application)
|-- mainwindow.h                <-- Main Window Class Declaration(72 lines;defines the available features)
|-- mainwindow.cpp              <-- main-window core logic(438 lines;all business logic)
|-- mainwindow_init.cpp         <-- initialization code(150 lines;startup preparation)
|-- mainwindow.ui               <-- interface layout file（XML;generated by Qt Designer）
|-- software_env.h              <-- global configuration constants(28 lines;source of all magic numbers)
|-- supervision_system_demo.pro <-- Qt project file（Build Configuration）
|-- common/
|   |-- sqlite_op/
|   |   |-- sqlite_op.h         <-- database-operation function declarations
|   |   |-- sqlite_op.c         <-- database-operation function implementation
|   |-- tool/
|       |-- tool.h              <-- utility macro definitions
|-- README.md                   <-- project description document
|-- compiled programs and system/             <-- precompiled system images and deployment packages
```

**File size comparison (lines of code)**：

```
mainwindow.cpp         ████████████████████████████████████████  438 lines  <-- most central
mainwindow_init.cpp    ██████████████                            150 lines
sqlite_op.c            ████████████                              234 lines
sqlite_op.h            ████████                                  94 lines
mainwindow.h           ████████                                  72 lines
mainwindow.ui          █████████                                 165 lines
software_env.h         ███                                       28 lines
main.cpp               ██                                        12 lines
tool.h                 █                                         13 lines
```

---

## 4. Detailed Explanation of Core Files

### 4.1 software_env.h —— Global Configuration Center

**Purpose**：the entire project’s"configuration file";all important constants are defined here .

```cpp
// File path configuration
#define SOFTWARE_INSTALL_PATH "/opt/supervision_system_demo/"  // Program installation directory
#define SOFTWARE_PYTHON_READ_DHT11_SCRIPT "python_dht11_read.py"      // sensor-reading script
#define SOFTWARE_PYTHON_SETUP_STATUS_SCRIPT "python_status_setup.py"   // LED/buzzer control script
#define SOFTWARE_WARNING_DATA_FILE "humidity_temperature_warning_data" // threshold storage file
#define SOFTWARE_SQLITE_DB "ss_sample_value.db"                        // database filename

// Status code definitions (arguments passed to Python scripts)
#define SOFTWARE_STATUS_OK 0                    // everything normal -> green LED
#define SOFTWARE_STATUS_HUMIDITY_WARNING 1      // humidity exceeded -> red LED
#define SOFTWARE_STATUS_TEMPERATURE_WARNING 2   // temperature exceeded -> yellow LED
#define SOFTWARE_STATUS_ALL_WARNING 3           // both exceeded   -> red LED+yellow LED
#define SOFTWARE_STATUS_EXIT 4                  // program exit -> all off

// Default thresholds
#define WARNING_DEFAULT_HUMIDITY 20       // default humidity threshold 20%
#define WARNING_DEFAULT_TEMPERATURE 40    // default temperature threshold 40°C

// Sensor range limits
#define DHT11_HUMIDITY_MAX_LIMIT 90       // maximum humidity 90%
#define DHT11_TEMPERATURE_MAX_LIMIT 50    // maximum temperature 50°C
```

**Why have a separate file?**
> Putting all "magic numbers" in one place.If you later need to change a threshold, file path, or status code, you only need to edit this file instead of searching everywhere.This is an embedded-development best practice .

---

### 4.2 main.cpp —— Program Entry Point

**Purpose**：The entire program starts executing here; it has only 12 lines.

```cpp
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // Enable the virtual keyboard (for touchscreen input)
    qputenv("QT_IM_MODULE", QByteArray("Qt5Input"));

    QApplication a(argc, argv);  // Create the Qt application
    MainWindow w;                // Create the main window
    w.show();                    // Show the window
    return a.exec();             // Enter the event loop (wait for user actions)
}
```

**Line-by-line explanation**：
1. `qputenv(...)` — Set an environment variable so Qt knows to use the virtual keyboard (embedded touchscreens have no physical keyboard)
2. `QApplication a(argc, argv)` — The Qt application object, which manages the program lifecycle
3. `MainWindow w` — Create the main window object;**the constructor calls init() to initialize everything**
4. `w.show()` — Display the window
5. `a.exec()` — Enter the **event loop**;the program waits here for events (timer triggers, button clicks, and so on) until the user closes the window

---

### 4.3 mainwindow.h —— Main Window Class Declaration

**Purpose**：Define what the main window “has”—member variables and member functions.

```cpp
class MainWindow : public QMainWindow
{
    Q_OBJECT  // Qt macro that enables signals and slots for this class

public:
    MainWindow(QWidget *parent = nullptr);  // constructor
    ~MainWindow();                          // destructor

private:
    // ========== Function declarations ==========
    void init();                           // Overall initialization function
    void init_read_warning_data();         // read the threshold file
    void init_humidity_charts();           // Create the humidity chart
    void init_temperature_charts();        // Create the temperature chart
    void init_sqlite_db();                 // initialize database

    QString read_dht11_data();             // read the DHT11 sensor
    void analysis_cur_status(int cur_humidity, int cur_temperature);  // analyze whether limits are exceeded
    void setup_warning_device(int status); // Control LEDs and the buzzer

    // ========== Variable declarations ==========
    Ui::MainWindow *ui;           // interface pointer (to the interface generated from the .ui file)

    int m_humidity_warning;       // Humidity threshold
    int m_temperature_warning;    // Temperature threshold
    QChart* m_humidity_chart;     // humidity line chart
    QChart* m_temperature_chart;  // temperature line chart

    QList<QProcess*> m_setup_status_process_list;  // Python child-process list
    QTimer m_update_map_timer;           // 1-second timer (read sensor)
    QTimer m_update_warning_data_timer;  // 200 ms timer (check threshold changes)

    sqlite_db m_sqlite_handle;   // database connection handle

private slots:
    void update_by_dht11();       // timer trigger: read sensor + update interface
    void update_warning_limit();  // timer trigger: check threshold-file changes
    void on_pushButton_exit_clicked();              // exit button
    void on_pushButton_humidity_update_clicked();   // Update humidity threshold button
    void on_pushButton_temperature_update_clicked();// Update temperature threshold button
};
```

**Key concepts**：

| Concept | Plain-language explanation |
|------|---------|
| `Q_OBJECT` | Tell Qt “this class uses signals and slots,” similar to saying “this class supports event handling” |
| `private slots` | "slot functions"——Qt calls these functions automatically when an event occurs (a timer fires or a button is clicked) |
| `QTimer` | timer;triggers the corresponding slot function every N milliseconds |
| `QProcess` | A child-process manager used to call external Python scripts |
| `QChart*` | A pointer to a line-chart object |
| `sqlite_db` | A custom database connection structure |

---

### 4.4 mainwindow_init.cpp —— Initialize Everything

**Purpose**：the “preparation work” at program startup; set everything up.

#### init() —— Overall initialization function

```cpp
void MainWindow::init()
{
    // 1. Hide the unnecessary menu and status bars
    this->ui->menubar->hide();
    this->ui->statusbar->hide();

    // 2. Initialize each module in sequence
    init_read_warning_data();      // Read thresholds
    init_humidity_charts();        // Create the humidity chart
    init_temperature_charts();     // Create the temperature chart
    init_sqlite_db();              // initialize database

    // 3. Set numeric limits for input fields (only values within the sensor range are accepted)
    auto update_lineedit_limit = [](QLineEdit* lineedit, int min, int max) {
        QIntValidator* intValidator = new QIntValidator(min, max, lineedit);
        lineedit->setValidator(intValidator);
    };
    update_lineedit_limit(ui->lineEdit_humidity, 0, 90);     // humidity 0–90
    update_lineedit_limit(ui->lineEdit_temperature, 0, 50);  // temperature 0–50

    // 4. Display the initial thresholds in the interface
    ui->label_cur_humidity_warning->setText(QString::number(m_humidity_warning) + "%");
    ui->label_cur_temperature_warning->setText(QString::number(m_temperature_warning) + "°C");

    // 5. start two timers
    // Timer 1: read the sensor once per second
    connect(&m_update_map_timer, &QTimer::timeout, this, &MainWindow::update_by_dht11);
    m_update_map_timer.start(1000);

    // Timer 2: check every 200 ms whether the threshold file was modified externally
    connect(&m_update_warning_data_timer, &QTimer::timeout, this, &MainWindow::update_warning_limit);
    m_update_warning_data_timer.start(200);

    // 6. Read the sensor once immediately (without waiting for the timer)
    update_by_dht11();
}
```

#### init_read_warning_data() —— Read thresholds

```cpp
void MainWindow::init_read_warning_data()
{
    // Set default values
    m_humidity_warning = 20;     // default humidity threshold 20%
    m_temperature_warning = 40;  // default temperature threshold 40°C

    // Try to read thresholds from the file
    QString file_data;
    int ret = rw_warning_data_file_context_lock(FILE_RW_R, file_data);

    // Parse the file contents（format: "humidity=20 temperature=40"）
    ret += analysis_string_dht11_data(file_data, m_humidity_warning, m_temperature_warning);

    // If reading fails (the file is missing or malformed), write the default values to the file
    if (ret) {
        file_data = "humidity=20 temperature=40";
        rw_warning_data_file_context_lock(FILE_RW_W, file_data);
    }
}
```

#### init_sqlite_db() —— initialize database

```cpp
void MainWindow::init_sqlite_db()
{
    // Open (or create) the database file
    QString db_path = file_select(SOFTWARE_SQLITE_DB);
    open_sqlite(&m_sqlite_handle, db_path.toLatin1().data());

    // Create the table (if it does not exist)
    // Table schema:id | humidity | temperature | humidity_warning | temperature_warning | status
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

#### init_general_charts() —— Create the line chart

```cpp
QChart* MainWindow::init_general_charts(QString target, QString target_unit,
                                         int x_count, int y_count,
                                         qreal y_min, qreal y_max)
{
    // Create the chart object
    QChart* chart = new QChart();
    chart->setTitle(target + "变化图");          // title, such as "humidity chart"
    chart->setAnimationOptions(QChart::SeriesAnimations);  // Enable animations

    // X axis: time axis
    QDateTimeAxis *dateTimeAxisX = new QDateTimeAxis;
    dateTimeAxisX->setTickCount(x_count);        // tick count
    dateTimeAxisX->setFormat("hh:mm:ss");        // time format
    dateTimeAxisX->setTitleText("时间");

    // Y axis: value axis
    QValueAxis *valueAxisY = new QValueAxis;
    valueAxisY->setTickCount(y_count);
    valueAxisY->setRange(y_min, y_max);          // value range
    valueAxisY->setTitleText(target + "(" + target_unit + ")");

    // data series
    QLineSeries *series = new QLineSeries();
    series->setName(target);

    // Assemble: add the data series and axes to the chart
    chart->addSeries(series);
    chart->addAxis(dateTimeAxisX, Qt::AlignBottom);
    chart->addAxis(valueAxisY, Qt::AlignLeft);
    series->attachAxis(dateTimeAxisX);
    series->attachAxis(valueAxisY);

    // Create the chart view and add it to the interface layout
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);  // antialiasing
    this->ui->horizontalLayout_charts->addWidget(chartView);

    return chart;
}
```

---

### 4.5 mainwindow.cpp —— Core Business Logic

This is the **most important file**; it contains all business logic.

#### read_dht11_data() —— Read the sensor

```cpp
QString MainWindow::read_dht11_data()
{
    // 1. Find the Python script path
    QString file_path = file_select(SOFTWARE_PYTHON_READ_DHT11_SCRIPT);
    if (file_path.isEmpty()) return QString();

    // 2. Create a child process and execute "python3 python_dht11_read.py"
    QProcess process;
    process.setProgram("python3");
    QStringList arguments;
    arguments << file_path;
    process.setArguments(arguments);

    // 3. Start it and wait for completion
    process.start();
    process.waitForFinished();

    // 4. Read the Python script output (standard output)
    QByteArray output = process.readAllStandardOutput();
    return QString::fromLocal8Bit(output);
    // Return format: "humidity=52 temperature=26"
}
```

**In plain language**：
> This is like entering `python3 python_dht11_read.py` ; then read the text it prints. The Python script reads the sensor and then prints("humidity=52 temperature=26") .

#### update_by_dht11() —— Core function executed once per second

```cpp
void MainWindow::update_by_dht11()
{
    // Use a lambda function to update the line chart
    auto update_chart = [&currentDateTime](QChart* chart, int value) {
        QLineSeries *series = static_cast<QLineSeries*>(chart->series().at(0));
        series->append(currentDateTime.toMSecsSinceEpoch(), value);  // Add a data point

        // Keep at most 5 points and delete the oldest when this is exceeded
        if (series->count() > 5) {
            series->removePoints(0, series->count() - 5);
        }

        // Update the X-axis range (so the time axis follows the data)
        chart->axes(Qt::Horizontal).back()->setRange(
            QDateTime::fromMSecsSinceEpoch(series->at(0).x()),
            QDateTime::fromMSecsSinceEpoch(series->at(series->count() - 1).x()));
    };

    // Ensure more than 1 second has elapsed since the previous sample
    if (当前时间 - 上次采样时间 >= 995毫秒) {
        // 1. Read the sensor
        QString python_run_data = read_dht11_data();

        // 2. Parse the data (extract numbers from "humidity=52 temperature=26"  )）
        int humidity, temperature;
        if (analysis_string_dht11_data(python_run_data, humidity, temperature)) {
            return;  // Parsing failed; skip this iteration
        }

        // 3. Update the displayed values
        ui->label_humidity->setText(QString::number(humidity) + "%");
        ui->label_temperature->setText(QString::number(temperature) + "°C");

        // 4. Update the line charts
        update_chart(m_humidity_chart, humidity);
        update_chart(m_temperature_chart, temperature);

        // 5. Analyze the status, write to the database, and control the hardware
        analysis_cur_status(humidity, temperature);
    }
}
```

#### analysis_cur_status() —— Determine whether limits are exceeded

```cpp
void MainWindow::analysis_cur_status(int cur_humidity, int cur_temperature)
{
    // Calculate the status code (combine with addition)
    int cur_status = SOFTWARE_STATUS_OK;  // 0

    // If humidity is exceeded, add 1
    cur_status += (cur_humidity > m_humidity_warning) ? SOFTWARE_STATUS_HUMIDITY_WARNING : 0;
    // If temperature is exceeded, add 2
    cur_status += (cur_temperature > m_temperature_warning) ? SOFTWARE_STATUS_TEMPERATURE_WARNING : 0;

    // Status-code combinations:
    // 0 = neither exceeded -> green LED
    // 1 = humidity exceeded -> red LED
    // 2 = temperature exceeded -> yellow LED
    // 3 = both exceeded   -> red LED+yellow LED

    // Write to the database
    QString sql = "INSERT INTO ht_data (humidity, temperature, humidity_warning, "
                  "temperature_warning, status) VALUES(" +
        QString::number(cur_humidity) + ", " +
        QString::number(cur_temperature) + ", " +
        QString::number(m_humidity_warning) + ", " +
        QString::number(m_temperature_warning) + ", " +
        QString::number(cur_status) + ");";
    sqlite_exec(&m_sqlite_handle, sql.toLatin1().data());

    // Control the hardware
    setup_warning_device(cur_status);
}
```

**Why the status-code design is clever**：
> The four states are represented by 0, 1, 2, and 3, with 3 = 1 + 2 (humidity exceeded + temperature exceeded). Simple addition combines every case without deeply nested if-else statements.

#### setup_warning_device() —— Control LEDs and the buzzer

```cpp
void MainWindow::setup_warning_device(int status)
{
    // 1. Clean up completed Python child processes (free memory)
    for (int i = m_setup_status_process_list.length() - 1; i >= 0; --i) {
        if (m_setup_status_process_list.at(i)->waitForFinished(30)) {
            delete m_setup_status_process_list.at(i);
            m_setup_status_process_list.removeAt(i);
        }
    }

    // 2. Create a new child process and execute: python3 python_status_setup.py <status>
    QProcess *new_process = new QProcess();
    new_process->setProgram("python3");
    QStringList arguments;
    arguments << file_path << QString::number(status);  // Pass in the status code
    new_process->setArguments(arguments);
    new_process->start();

    // 3. Add the process to the list for later management
    m_setup_status_process_list.append(new_process);
}
```

**In plain language**：
> This is equivalent to executing `python3 python_status_setup.py 2`; the Python script receives argument 2 and therefore turns on the yellow LED (temperature exceeded). The script uses the RPi.GPIO library to control hardware pins.

#### rw_warning_data_file_context_lock() —— Locked file I/O

```cpp
int MainWindow::rw_warning_data_file_context_lock(int rw, QString& buffer)
{
    char ori_data[64];

    // Open the file (create it if it does not exist)
    int fd = open(SOFTWARE_WARNING_DATA_FILE, O_RDWR | O_CREAT, 0666);

    if (rw) {  // read operation
        flock(fd, LOCK_EX | LOCK_NB);    // Acquire an exclusive lock (prevent concurrent writes)
        int len = read(fd, ori_data, 63); // Read the contents
        // Remove trailing newline characters
        if (ori_data[len-1] == '\n') ori_data[len-1] = 0;
        if (ori_data[len-2] == '\r') ori_data[len-2] = 0;
        buffer = QString::fromLatin1(ori_data);
    } else {   // write operation
        flock(fd, LOCK_EX);               // Acquire an exclusive lock
        write(fd, buffer.toLatin1().data(), buffer.length());  // Write the contents
        write(fd, "\0", 1);               // Write the terminator
    }

    flock(fd, LOCK_UN);  // Unlock
    close(fd);
    return 0;
}
```

**Why is a file lock needed?**
> Because two places access the threshold file:(1) the user clicks the “Update” button in the interface(2) the 200 ms timer polls it. Concurrent reads and writes could corrupt the data. A lock is like locking a restroom door: while one person is using it, everyone else waits.

#### analysis_string_dht11_data() —— Parse sensor data

```cpp
int MainWindow::analysis_string_dht11_data(QString data, int& humidity, int& temperature)
{
    // Input: "humidity=52 temperature=26"
    QStringList data_group = data.split(" ");  // Split on spaces
    // result: ["humidity=52", "temperature=26"]

    if (data_group.length() != 2) return 1;  // malformed format

    QStringList humidity_data_group = data_group.at(0).split("=");  // Split on the equals sign
    // result: ["humidity", "52"]

    QStringList temperature_data_group = data_group.at(1).split("=");
    // result: ["temperature", "26"]

    humidity = humidity_data_group.at(1).toInt();       // 52
    temperature = temperature_data_group.at(1).toInt();  // 26

    return 0;  // Success
}
```

#### Destructor — cleanup when the program exits

```cpp
MainWindow::~MainWindow()
{
    close_sqlite(&m_sqlite_handle);           // Close the database
    m_update_map_timer.stop();                // Stop the timer
    m_update_warning_data_timer.stop();
    setup_warning_device(SOFTWARE_STATUS_EXIT); // Tell the hardware “I am exiting” (turn off all LEDs and the buzzer)

    // wait for all Python child processes to finish
    for (int i = m_setup_status_process_list.length() - 1; i >= 0; --i) {
        if (m_setup_status_process_list.at(i)->waitForFinished()) {
            delete m_setup_status_process_list.at(i);
        }
    }

    delete ui;  // Release interface resources
}
```

---

### 4.6 mainwindow.ui —— Interface Layout

This is an XML file generated by the Qt Designer visual design tool. It defines the window’s visual layout:

```
+-------------------------------------------------------------------+
|  Current humidity: 52%    Humidity threshold: 20%    Current temperature: 26°C   Temperature threshold: 40°C   [Exit application]  |  <-- first row
+-------------------------------------------------------------------+
|  Humidity threshold: [____] [Update]        Temperature threshold: [____] [Update]                  |  <-- second row
+-------------------------------------------------------------------+
|                                                                   |
|  +------------------------+    +------------------------+         |
|  |    Humidity chart            |    |    Temperature chart            |         |  <-- third row
|  |   (real-time line chart)           |    |   (real-time line chart)           |         |    (added dynamically at runtime)
|  |                        |    |                        |         |
|  +------------------------+    +------------------------+         |
|                                                                   |
+-------------------------------------------------------------------+
```

**Key UI elements**：

| Widget name | Type | Purpose |
|--------|------|------|
| `label_humidity` | QLabel | Display the current humidity value |
| `label_temperature` | QLabel | Display the current temperature value |
| `label_cur_humidity_warning` | QLabel | Display the current humidity threshold |
| `label_cur_temperature_warning` | QLabel | Display the current temperature threshold |
| `lineEdit_humidity` | QLineEdit | Enter a new humidity threshold |
| `lineEdit_temperature` | QLineEdit | Enter a new temperature threshold |
| `pushButton_humidity_update` | QPushButton | Update humidity threshold button |
| `pushButton_temperature_update` | QPushButton | Update temperature threshold button |
| `pushButton_exit` | QPushButton | Exit the program |
| `horizontalLayout_charts` | QHBoxLayout | Container for line charts（added dynamically at runtime） |

---

### 4.7 sqlite_op.h / sqlite_op.c —— Database Utility Library

**Purpose**：A wrapper around the SQLite3 C API that simplifies database operations.

#### Data structure

```c
typedef struct sqlite_db {
    char file_name[128];   // Database file path
    sqlite3* database;     // SQLite3 connection pointer
    int open_target;       // Flag indicating whether it is open（53 = open）
} sqlite_db;
```

#### Provided functions

```c
// Check whether the database file exists
int check_sqlite_exist(char* file_name);

// Open/close the database
int open_sqlite(sqlite_db* db, char* file_name);
int close_sqlite(sqlite_db* db);

// Execute SQL (INSERT/UPDATE/DELETE/CREATE)
int sqlite_exec(sqlite_db* db, char* sql);

// Query data (SELECT)
int sqlite_get_data(sqlite_db* db, char* sql, char*** data, int* row, int* col);

// Get the last inserted row ID
int sqlite_get_last_id(sqlite_db* db, char* table_name, char** id);

// Clear a table
int truncate_table(sqlite_db* db, char* table_name);

// Free query-result memory
int free_data(char** data);
```

**Why use a wrapper?**
> The raw SQLite3 API is cumbersome (error messages, memory allocation, and so on must be managed manually). With this wrapper, the main program only needs to call `sqlite_exec(&db, "INSERT ...")` on one line, without worrying about implementation details.

#### Core function implementation examples

```c
// Execute an SQL statement
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

// open database
int open_sqlite(sqlite_db* db, char* file_name)
{
    // Close it first if it is already open
    if (db->open_target == 53) {
        sqlite3_close(db->database);
    }

    // Save the filename
    strncpy(db->file_name, file_name, 127);

    // open database
    int res = sqlite3_open(db->file_name, &db->database);
    if (res) {
        printf("Cannot open database: %s\n", sqlite3_errmsg(db->database));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

---

### 4.8 tool.h —— Utility Macros

```c
// If a function returns failure, immediately return failure (simplifies error handling)
#define IF_RES_FAIL_RETURN if(res == EXIT_FAILURE) { return EXIT_FAILURE; }

// Calculate the actual usable length of a string buffer (subtract the trailing '\0'）
#define STRING_TRUE_LEN(buf_max) (buf_max - 1)
```

**Usage example**：
```c
int sqlite_get_last_id(sqlite_db* db, char* table_name, char** id)
{
    int res = sqlite_get_data(db, sql, &data, &row, &col);
    IF_RES_FAIL_RETURN;  // If the query fails, return immediately without writing an if statement
    // ... continue processing
}
```

---

### 4.9 supervision_system_demo.pro —— Build Configuration

```qmake
# Qt modules to use
QT += core gui           # Core modules
QT += charts             # Charts module (line charts)
QT += virtualkeyboard    # Virtual keyboard module

# C++ standard
CONFIG += c++11

# Source file list
SOURCES += common/sqlite_op/sqlite_op.c main.cpp mainwindow.cpp mainwindow_init.cpp

# Header file list
HEADERS += common/sqlite_op/sqlite_op.h common/tool/tool.h mainwindow.h software_env.h

# Include paths (so #include "sqlite_op.h" can find the file)
INCLUDEPATH += common/sqlite_op common/tool

# Form file
FORMS += mainwindow.ui

# Link the SQLite3 library
unix:!macx: LIBS += -lsqlite3
```

**Purpose of this file**：
> Tell Qt’s qmake build tool："which files are sources, which are headers, and which libraries are required" .qmake then generates a Makefile automatically, and make compiles it .

---

## 5. Data Flow: The Complete Journey from Sensor to Screen

Let us trace one complete data-acquisition cycle:

```
Timeline：
  T=0s    timer m_update_map_timer fires
          |
  T=0.01s call update_by_dht11()
          |
  T=0.02s call read_dht11_data()
          | create a QProcess child process
  T=0.03s execute "python3 python_dht11_read.py"
          | the Python script reads the DHT11 sensor through GPIO
  T=0.2s  Python output "humidity=52 temperature=26" and exits
          | QProcess reads standard output
  T=0.21s call analysis_string_dht11_data() parse out humidity=52, temperature=26
          |
  T=0.22s Update the interface label: "52%", "26°C"
          |
  T=0.23s Update the line charts：Add a new data point and delete old points beyond five
          |
  T=0.24s call analysis_cur_status(52, 26)
          | Calculate status code:52>20(humidity exceeded) -> +1;26<40 (temperature not exceeded) -> +0;result=1
  T=0.25s execute SQL: INSERT INTO ht_data VALUES(52, 26, 20, 40, 1)
          |
  T=0.26s call setup_warning_device(1)
          | Create a new child process
  T=0.27s execute "python3 python_status_setup.py 1"
          | the Python script turns on the red LED (humidity exceeded) and the buzzer sounds
  T=0.5s  the Python script finishes
          |
  T=1.0s  the next timer fires... (loop)
```

---

## 6. How the Modules Fit Together

### 6.1 Dependency diagram

```
                    +----------------+
                    | software_env.h |  <-- everyone depends on it
                    | (global constants)      |
                    +-------+--------+
                            |
              +-------------+-------------+
              v             v             v
        +-----------+ +----------+ +--------------+
        | main.cpp  | |mainwindow| |mainwindow_init|
        | (entry point)    | |  .cpp    | |    .cpp      |
        +-----+-----+ +----+-----+ +------+-------+
              |             |              |
              v             v              v
        +------------------------------------------+
        |          mainwindow.h                     |
        |     (MainWindow class declaration)                   |
        +------------------+-----------------------+
                           |
                           v
        +------------------------------------------+
        |     common/sqlite_op/sqlite_op.h          |
        |     (database-operation interface)                       |
        +------------------+-----------------------+
                           |
                           v
        +------------------------------------------+
        |     common/sqlite_op/sqlite_op.c          |
        |     (database-operation implementation)                       |
        +------------------+-----------------------+
                           |
                           v
        +------------------------------------------+
        |         common/tool/tool.h                |
        |         (Utility Macros)                          |
        +------------------------------------------+
```

### 6.2 How the pieces are assembled at compile time

```
Source compilation：
  main.cpp            --Compile-->  main.o
  mainwindow.cpp      --Compile-->  mainwindow.o
  mainwindow_init.cpp --Compile-->  mainwindow_init.o
  sqlite_op.c         --Compile-->  sqlite_op.o

Linking：
  main.o + mainwindow.o + mainwindow_init.o + sqlite_op.o
  + libQt5Core + libQt5Gui + libQt5Charts + libQt5Widgets
  + libsqlite3
  =======================================================
  = supervision_system_demo (executable)
```

### 6.3 How the pieces are assembled at runtime

```
Program startup
  |
  +---> main.cpp creates MainWindow
  |     |
  |     +---> constructorcall init()
  |     |     |
  |     |     +---> init_read_warning_data()     <-- read the file to obtain thresholds
  |     |     |     +---> rw_warning_data_file_context_lock()  <-- locked file I/O
  |     |     |
  |     |     +---> init_humidity_charts()        <-- Create the humidity chart
  |     |     |     +---> init_general_charts()   <-- generic chart creation
  |     |     |
  |     |     +---> init_temperature_charts()     <-- Create the temperature chart
  |     |     |     +---> init_general_charts()
  |     |     |
  |     |     +---> init_sqlite_db()              <-- initialize database
  |     |     |     +---> open_sqlite()           <-- open database
  |     |     |     +---> sqlite_exec(CREATE TABLE) <-- create table
  |     |     |
  |     |     +---> start two timers
  |     |
  |     +---> [every second] update_by_dht11()
  |     |     +---> read_dht11_data()             <-- call Python Read the sensor
  |     |     +---> analysis_string_dht11_data()  <-- parse data
  |     |     +---> update the interface labels and charts
  |     |     +---> analysis_cur_status()         <-- determine status
  |     |           +---> sqlite_exec(INSERT)     <-- write to database
  |     |           +---> setup_warning_device()  <-- call Python Control the hardware
  |     |
  |     +---> [every 200 ms] update_warning_limit()
  |           +---> rw_warning_data_file_context_lock(read) <-- check whether thresholds changed
  |
  +---> [On exit] ~MainWindow()
        +---> close_sqlite()
        +---> setup_warning_device(EXIT)          <-- turn off all hardware
        +---> wait for all Python child processes to finish
```

### 6.4 Data-transfer methods between files

| Transfer method | From | To | Data |
|---------|--------|--------|--------|
| **Function call** | mainwindow.cpp | sqlite_op.c | SQL statement |
| **Function call** | mainwindow_init.cpp | mainwindow.cpp | call various functions during initialization |
| **Child-process stdout** | Python script | mainwindow.cpp | "humidity=52 temperature=26" |
| **Child-process arguments** | mainwindow.cpp | Python script | status code 0/1/2/3/4 |
| **File I/O** | mainwindow.cpp | threshold file | "humidity=20 temperature=40" |
| **Header file #include** | .cpp files | .h files | type declarations and function declarations |
| **extern "C"** | mainwindow.h | sqlite_op.h | C/C++ interoperability |

---

## 7. Summary of Key Design Ideas

### 7.1 Why mix C++ and Python?

| Language | Strengths | Role in this project |
|------|---------|----------------|
| C++ + Qt | GUI, event handling, and database operations | main-program framework |
| Python | hardware GPIO control and rapid prototyping | sensor reading and LED/buzzer control |

**Benefit**: Each language is used for what it does best. Qt provides a mature GUI framework, while Python makes hardware control simple. They communicate through a child-process “bridge”.

### 7.2 Why use a timer instead of a loop?

```cpp
// Wrong approach: infinite loop
while (true) {
    read_sensor();
    update_ui();
    sleep(1);
}

// Correct approach: timer
QTimer timer;
connect(&timer, &QTimer::timeout, this, &MainWindow::update_by_dht11);
timer.start(1000);
```

**Reason**: Qt is event-driven. An infinite loop blocks event processing and freezes the interface (buttons cannot respond and the window cannot repaint). A timer schedules the task in the event loop once per second, leaving the interface responsive at other times.

### 7.3 Why use a file lock?

The threshold file is accessed in two places:
1. 1. The user clicks “Update” -> write the file
2. 2. The 200 ms timer -> read the file

Without a lock, a partial value could be read (for example, only `humidity=20 tem`). The lock makes reads and writes atomic.

### 7.4 Why use 0/1/2/3 as status codes?

```
0 = normal      (00 binary) -> green LED
1 = humidity exceeded   (01 binary) -> red LED
2 = temperature exceeded   (10 binary) -> yellow LED
3 = both exceeded     (11 binary) -> red LED+yellow LED
```

This is actually a **bitmask** design:
- bit 0 represents humidity (1 = exceeded)
- bit 1 represents temperature (2 = exceeded)

Python The script can use bitwise operations for quick checks：
```python
if status & 1:  # humidity exceeded
    turn_on_red_led()
if status & 2:  # temperature exceeded
    turn_on_yellow_led()
```

### 7.5 Database table design

```sql
CREATE TABLE ht_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,  -- auto-incrementing primary key
    humidity INTEGER,                       -- Current humidity
    temperature INTEGER,                    -- Current temperature
    humidity_warning INTEGER,               -- the humidity threshold at that time
    temperature_warning INTEGER,            -- the temperature threshold at that time
    status INTEGER                          -- the status code at that time
);
```

**Why store both the “current value” and the “threshold”?**
> Because thresholds can be changed at any time. If only the current value were stored, later you would not know whether it was “normal” or “over the limit” at that time. Storing the threshold too lets you reconstruct the historical state.

---

## Appendix: Quick Start Guide

### Build steps

```bash
# 1. Make sure the Qt5 and SQLite3 development packages are installed
# Ubuntu/Debian:
sudo apt install qt5-default libqt5charts5-dev libsqlite3-dev

# 2. Generate the Makefile with qmake
qmake supervision_system_demo.pro

# 3. Compile
make

# 4. Run (on a Loongson development board or with a DHT11 sensor connected)
./supervision_system_demo
```

### Suggested reading order

To understand this project, read the files in the following order:

1. **software_env.h** —— learn the constants and configuration first
2. **main.cpp** —— see where the program starts
3. **mainwindow.h** —— learn which members the main-window class has
4. **mainwindow_init.cpp** —— see what initialization does
5. **mainwindow.cpp** —— understand the core business logic
6. **sqlite_op.h / sqlite_op.c** —— learn the database-operation details
7. **mainwindow.ui** —— look at the interface layout last

---

> **Summary**: Although this project is small, it contains the typical elements of embedded development—sensor data acquisition, a GUI, database storage, hardware control, and cross-language calls. Understanding it gives you the fundamentals of embedded Linux application development.
