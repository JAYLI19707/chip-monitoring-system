#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "software_env.h"

#include <QtCharts>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QChart>
#include <QTimer>

#include <QIntValidator>

void MainWindow::init()
{
    this->ui->menubar->hide();
    this->ui->statusbar->hide();

    init_read_warning_data();
    init_humidity_charts();
    init_temperature_charts();
    init_sqlite_db();

    // 设置阈值设置的 lineedit 的可输入范围，为传感器的量程
    auto update_lineedit_limit = [](QLineEdit* lineedit, int min, int max) -> void {
        QIntValidator* intValidator = new QIntValidator(min, max, lineedit);
        lineedit->setValidator(intValidator);
    };
    update_lineedit_limit(this->ui->lineEdit_humidity, DHT11_HUMIDITY_MIN_LIMIT, DHT11_HUMIDITY_MAX_LIMIT);
    update_lineedit_limit(this->ui->lineEdit_temperature, DHT11_TEMPERATURE_MIN_LIMIT, DHT11_TEMPERATURE_MAX_LIMIT);

    // 初始化阈值
    this->ui->label_cur_humidity_warning->setText(QString::number(this->m_humidity_warning) + "%");
    this->ui->label_cur_temperature_warning->setText(QString::number(this->m_temperature_warning) + "°C");
    this->ui->lineEdit_humidity->setText(QString::number(this->m_humidity_warning));
    this->ui->lineEdit_temperature->setText(QString::number(this->m_temperature_warning));

    m_setup_status_process_list.clear();

    // 设置定时器，定时读取传感器数据，然后更新信息到界面
    connect(&m_update_map_timer, &QTimer::timeout, this, &MainWindow::update_by_dht11);
    m_update_map_timer.start(1000); // 每秒更新一次
    update_by_dht11();

    // 设置定时器，定时读取有没有新的警告阈值设定
    connect(&m_update_warning_data_timer, &QTimer::timeout, this, &MainWindow::update_warning_limit);
    m_update_warning_data_timer.start(200); // 每秒更新5次
}

void MainWindow::init_read_warning_data()
{
    int ret;
    QString warning_file_path;
    QString file_data;

    ret = 0;
    this->m_humidity_warning = WARNING_DEFAULT_HUMIDITY;
    this->m_temperature_warning = WARNING_DEFAULT_TEMPERATURE;

    warning_file_path = file_select(SOFTWARE_WARNING_DATA_FILE);
    if (warning_file_path.isEmpty()) {
        warning_file_path = SOFTWARE_WARNING_DATA_FILE;
    }

    ret += rw_warning_data_file_context_lock(FILE_RW_R, file_data);
    ret += analysis_string_dht11_data(file_data, this->m_humidity_warning, this->m_temperature_warning);

    if (ret) {
        file_data = "humidity=" + QString::number(WARNING_DEFAULT_HUMIDITY) + " temperature=" + QString::number(WARNING_DEFAULT_TEMPERATURE);
        rw_warning_data_file_context_lock(FILE_RW_W, file_data);
    }
}

void MainWindow::init_humidity_charts()
{
    this->m_humidity_chart = init_general_charts("湿度", "%", 5, 10, DHT11_HUMIDITY_MIN_LIMIT, DHT11_HUMIDITY_MAX_LIMIT);
}

void MainWindow::init_temperature_charts()
{
    this->m_temperature_chart = init_general_charts("温度", "°C", 5, 10, DHT11_TEMPERATURE_MIN_LIMIT, DHT11_TEMPERATURE_MAX_LIMIT);
}

void MainWindow::init_sqlite_db()
{
    QString db_path = file_select(SOFTWARE_SQLITE_DB);
    if (db_path.isEmpty()) {
        db_path = SOFTWARE_SQLITE_DB;
    }
    open_sqlite(&m_sqlite_handle, db_path.toLatin1().data());
    sqlite_exec(&m_sqlite_handle,
                (char*)"CREATE TABLE IF NOT EXISTS ht_data "
                       "(id INTEGER PRIMARY KEY AUTOINCREMENT,humidity INTEGER,"
                       "temperature INTEGER,humidity_warning INTEGER,temperature_warning INTEGER,"
                       "status INTEGER);");
}

//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////

QChart* MainWindow::init_general_charts(QString target, QString target_unit, int x_count, int y_count, qreal y_min, qreal y_max)
{
    QString title;
    QString target_y_title;
    QFont axisFont;

    title = target + "变化图";
    target_y_title = target + "(" + target_unit + ")";

    QChart* chart = new QChart();
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QDateTimeAxis *dateTimeAxisX = new QDateTimeAxis;
    dateTimeAxisX->setTickCount(x_count);
    dateTimeAxisX->setFormat("hh:mm:ss");
    dateTimeAxisX->setTitleText("时间");
    axisFont = dateTimeAxisX->labelsFont();
    axisFont.setPointSize(10); // 字体小一点，不然都看不到文字
    dateTimeAxisX->setLabelsFont(axisFont);

    QValueAxis *valueAxisY = new QValueAxis;
    valueAxisY->setTickCount(y_count);
    valueAxisY->setLabelFormat("%.2f");
    valueAxisY->setTitleText(target_y_title);
    valueAxisY->setRange(y_min, y_max);
    axisFont = valueAxisY->labelsFont();
    axisFont.setPointSize(10); // 字体小一点，不然都看不到文字
    valueAxisY->setLabelsFont(axisFont);

    QLineSeries *series = new QLineSeries();
    series->setName(target);

    chart->addSeries(series);
    chart->addAxis(dateTimeAxisX, Qt::AlignBottom); // 将X轴添加到图表的底部
    chart->addAxis(valueAxisY, Qt::AlignLeft); // 将Y轴添加到图表的左侧
    series->attachAxis(dateTimeAxisX); // 将数据序列与Y轴关联
    series->attachAxis(valueAxisY); // 将数据序列与Y轴关联

    // 设置图表视图
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // 将图表视图添加到布局中
    this->ui->horizontalLayout_charts->addWidget(chartView);

    return chart;
}
