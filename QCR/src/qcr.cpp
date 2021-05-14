#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <QIODevice>

#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include "include/qcr.h"
#include <include/base64.h>
#include "include/helper.h"
#include "include/my_message_box.h"
#include "include/bd_ocr.h"
#include "include/tx_ocr.h"


QCR::QCR(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    ui.central_widget->setLayout(ui.hbox);
    setCentralWidget(ui.central_widget);

    open_action = new QAction(QIcon(":/images/act_open.svg"), QString::fromUtf8(u8"打开"));
    crop_action = new QAction(QIcon(":/images/act_crop.svg"), QString::fromUtf8(u8"校正"));
    undo_action = new QAction(QIcon(":/images/act_undo.svg"), QString::fromUtf8(u8"恢复"));
    ocr_action = new QAction(QIcon(":/images/act_ocr.svg"), QString::fromUtf8(u8"识别"));
    export_action = new QAction(QIcon(":/images/act_export.svg"), QString::fromUtf8(u8"导出"));
    config_action = new QAction(QIcon(":/images/act_config.svg"), QString::fromUtf8(u8"设置"));
    about_action = new QAction(QIcon(":/images/act_about.svg"), QString::fromUtf8(u8"关于"));

    ui.toolbar->addAction(open_action);
    ui.toolbar->addAction(crop_action);
    ui.toolbar->addAction(undo_action);
    ui.toolbar->addAction(ocr_action);
    ui.toolbar->addAction(export_action);
    ui.toolbar->addAction(config_action);
    ui.toolbar->addAction(about_action);

    connect(open_action, SIGNAL(triggered()), this, SLOT(openImage()));
    connect(crop_action, &QAction::triggered, this, &QCR::interceptImage);
    connect(undo_action, &QAction::triggered, this, &QCR::undo);
    connect(ocr_action, &QAction::triggered, this, &QCR::runOcr);
    connect(export_action, SIGNAL(triggered()), this, SLOT(exportTableData()));
    connect(config_action, &QAction::triggered, &config_dialog, &ConfigDialog::exec);
    connect(about_action, SIGNAL(triggered()), &about_dlg, SLOT(show()));

    // 测试按钮
    test1_action = new QAction(QIcon(":/images/logo.png"), QString::fromUtf8(u8"测试"));
    ui.toolbar->addAction(test1_action);
    connect(test1_action, &QAction::triggered, this, &QCR::recognize);

    // 设置表格样式
    ui.ui_table_widget->setStyleSheet(
        "QHeaderView::section{ background-color: whitesmoke }"
        "QTableWidget::item:selected{ color: black; background-color: lightcyan }");

    // Putting the more time-consuming initialization work during program startup
    // into a separate thread.
    initial_thread = QThread::create(
        [&]() {
            config_dialog.loadConfig();
            getBdAccessToken();
        });
    initial_thread->start();
}

void QCR::getBdAccessToken()
{
    QDateTime now = QDateTime::currentDateTime();
    QString token_path = QString::fromUtf8(u8"./data/bd_%1.token").arg(now.toString(QString("yyyyMMdd")));
    QFile file(token_path);
    if (file.exists() && file.open(QIODevice::ReadOnly))
    {
        printLog(QString::fromUtf8(u8"Access Token 文件存在: ") + token_path);
        QTextStream in(&file);
        QString token = in.readAll();
        bd_access_token = token.toLocal8Bit().data();
        file.close();
    }
    else
    {
        // 删除data目录下所有token的文件
        QDir dir("data");
        QFileInfoList list = dir.entryInfoList(QStringList({ "bd_*.token" }), QDir::Files);
        for (auto &file : list)
        {
            bool ret = QFile::remove(file.absoluteFilePath());
            if (ret)
                printLog(QString::fromUtf8(u8"删除过期百度Token: %1").arg(file.absoluteFilePath()));
        }
        
        // 获取token, 没过期就不需要重新获取
        QString tmp = config_dialog.ui.line_bd_get_token_url->text();
        std::string bd_get_token_url = tmp.toStdString();
        tmp = config_dialog.ui.line_bd_api_key->text();
        std::string bd_api_key = tmp.toStdString();
        tmp = config_dialog.ui.line_bd_secret_key->text();
        std::string bd_secret_key = tmp.toStdString();

        std::string ret;
        bdGetAccessToken(ret, bd_get_token_url, bd_api_key, bd_secret_key);
        bd_access_token = json::parse(ret).at("access_token");

        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out << bd_access_token.c_str();
            out.flush();
            file.close();
        }
        printLog(QString::fromUtf8(u8"已获取到 Access Token 并写入到: ") + token_path);
    }
}

void QCR::openImage()
{
    QString path = QFileDialog::getOpenFileName(this,
        QString::fromUtf8(u8"打开图片"), QString::fromUtf8(u8"."),
        QString::fromUtf8(u8"图片 (*.png *.bmp *.jpg *.tiff);;所有文件 (*.*)"));
    printLog(QString::fromUtf8(u8"Original: ") + path);
    if (!path.isEmpty())
    {
        resetTable();
        ui.ui_img_widget->setPix(QPixmap(path));
        QApplication::processEvents();

        compress_thread = QThread::create(
            [=]() {
                // 创建 tmp 文件夹
                QDir dir;
                if (!dir.exists("./tmp"))
                {
                    dir.mkdir("tmp");
                }

                // 复制并压缩图片
                QFile file(path);
                img_path = QString::fromUtf8(u8"./tmp/qcr_%1.jpg").arg(getCurTimeString());
                img_path_cropped.clear();
                file.copy(img_path);
                printLog(QString::fromUtf8(u8"已创建副本: ") + img_path);
                resizeImage(img_path);
            });
        compress_thread->start();

        edgeDetection();
    }
}

void QCR::runOcr()
{
    QString path = img_path_cropped.isEmpty() ? img_path : img_path_cropped;
    if (path.isEmpty())
    {
        MyMessageBox msg(QString::fromUtf8(u8"请先打开一张图片!"));
        msg.exec();
        return;
    }
    printLog(QString::fromUtf8(u8"开始识别图片: ") + path);

    QThread *ocr_thread = QThread::create(
        [&]() {
            if (compress_thread->isRunning())
                compress_thread->wait();

            // 由于API限制, 压缩图片, 避免图片过大无法识别
            resizeImage(path);

            std::string base64_img;
            image2base64(path.toLocal8Bit().data(), base64_img);

            QString service_provider = config_dialog.ui.combo_service_provider->currentText();
            if (service_provider.contains(QString::fromUtf8(u8"腾讯")))
            {
                printLog(QString::fromUtf8(u8"使用腾讯API识别表格"));
                runTxOcr(base64_img);
            }
            else if(service_provider.contains(QString::fromUtf8(u8"百度")))
            {
                printLog(QString::fromUtf8(u8"使用百度API识别表格"));
                runBdOcr(base64_img);
            }
        });
    connect(ocr_thread, &QThread::finished, &animation, &LoadingAnimation::stop);

    ocr_thread->start();
    // 此处打开加载动画, 模态对话框, 阻塞直到 t2 线程结束
    if (ocr_thread->isRunning())
        animation.start();

    // 将数据写入表格
    updateTable();

    if (config_dialog.ui.check_auto_optimize->isChecked())
        optimize();
}

void QCR::runTxOcr(const std::string &base64_img)
{
    QString tmp = config_dialog.ui.line_tx_url->text();
    std::string tx_request_url = tmp.toStdString();
    tmp = config_dialog.ui.line_tx_secret_id->text();
    std::string tx_secret_id = tmp.toStdString();
    tmp = config_dialog.ui.line_tx_secret_key->text();
    std::string tx_secret_key = tmp.toStdString();

    std::string response;
    txFormOcrRequest(response, tx_request_url, tx_secret_id, tx_secret_key, base64_img);
    printLog(response);

    txParseData(response);
}

void QCR::runBdOcr(const std::string &base64_img)
{
    QString tmp = config_dialog.ui.line_bd_request_url->text();
    std::string bd_request_url = tmp.toStdString();
    tmp = config_dialog.ui.line_bd_get_result_url->text();
    std::string bd_get_result_url = tmp.toStdString();

    std::string request;
    bdFormOcrRequest(request, bd_request_url, bd_access_token, base64_img);
    //printLog(request);

    json req = json::parse(request);
    std::string request_id = req.at("result").at(0).at("request_id");

    std::string response;
    while (true)
    {
        // 百度QPS限制, 查询不能过于频繁
        QThread::sleep(1);
        bdGetResult(response, bd_get_result_url, bd_access_token, request_id, "json");
        printLog(response);
        json ocr_result = json::parse(response);
        if (ocr_result.at("result").at("ret_code") == 3)
            break;
    }
    bdParseData(response);
}

void QCR::exportTableData()
{
    // 创建 output 文件夹
    QDir dir;
    if (!dir.exists("./output"))
    {
        dir.mkdir("output");
    }
    // 打开保存文件对话框
    QString file_name = QString::fromUtf8("QCR_%1.csv").arg(getCurTimeString());
    QString file_path = QFileDialog::getSaveFileName(
        this, QString::fromUtf8(u8"导出数据"),
        QString::fromUtf8(u8"./output/") + file_name,
        QString::fromUtf8(u8"表格 (*.csv)"));
    if (!file_path.isEmpty())
    {
        QFile file(file_path);
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out.setCodec("GB18030");
            int row = ui.ui_table_widget->rowCount();
            int col = ui.ui_table_widget->columnCount();
            for (int i = 0; i < row; i++)
            {
                QString line = "";
                for (int j = 0; j < col; j++)
                {
                    QTableWidgetItem *item = ui.ui_table_widget->item(i, j);
                    QString t = item != nullptr ? item->text() : QString("");
                    line += t + QString(",");
                }
                // 将末尾多余的一个','替换为'\n'并写入文件
                out << line.replace(line.length() - 1, 1, QString("\n"));
                out.flush();
            }
            file.close();  // 关闭文件，否则数据无法保存
            MyMessageBox msg(QMessageBox::Information, QString::fromUtf8(u8"保存成功"),
                QString::fromUtf8(u8"数据已导出到文件 ") + file_path);
            msg.exec();
        }
    }
}

void QCR::recognize()
{
    processImage();
    /*
    if (initial_thread->isRunning())
        initial_thread->wait();

    // If set, always convert image to the 3 channel BGR color image.
    cv::Mat img = cv::imread(img_path.toLocal8Bit().data(), cv::IMREAD_COLOR);

    auto start = std::chrono::system_clock::now();
    std::vector<std::vector<std::vector<int>>> boxes;
    det.Run(img, boxes);

    // boxes to boxes_map
    std::map<int, Box> boxes_map;
    Box b;
    for (size_t i = 0; i < boxes.size(); ++i)
    {
        b.p1 = cv::Point(boxes[i][0][0], boxes[i][0][1]);
        b.p2 = cv::Point(boxes[i][1][0], boxes[i][1][1]);
        b.p3 = cv::Point(boxes[i][2][0], boxes[i][2][1]);
        b.p4 = cv::Point(boxes[i][3][0], boxes[i][3][1]);
        boxes_map[i] = b;
    }

    // Inferring table structure
    std::vector<std::vector<std::vector<int>>> index_table;
    infer_table(boxes_map, index_table);

    std::map<int, TextBlock> results = rec.Run(boxes_map, img, cls);
    auto end = std::chrono::system_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double t = double(duration.count()) * std::chrono::microseconds::period::num
        / std::chrono::microseconds::period::den;
    printLog(QString::fromUtf8(u8"耗时 %1 秒").arg(QString::number(t, 'f', 3));

    // Fill in the data in the table
    ui.ui_table_widget->clearContents();
    for (size_t i = 0; i < index_table.size(); ++i)
    {
        for (size_t j = 0; j < index_table[i].size(); ++j)
        {
            std::vector<int> cell = index_table[i][j];
            int index = cell[0];
            if (index == -1)
                continue;
            QTableWidgetItem *item = ui.ui_table_widget->item(cell[1], cell[2]);
            if (cell[3] > 1 || cell[4] > 1)
                ui.ui_table_widget->setSpan(cell[1], cell[2], cell[3], cell[4]);
            if (item)
            {
                item->setText(results[index].text);
                ui.ui_table_widget->setItem(i, j, item);
            }
            else
            {
                ui.ui_table_widget->setItem(i, j,
                    new QTableWidgetItem(results[index].text));
            }
        }
    }

    // export data
    QFileInfo info(img_path);
    std::string det_name = "./tmp/" + info.baseName().toLocal8Bit() + "_DET.tiff";
    Utility::VisualizeBboxes(img, boxes, det_name);
    */
}

double getDistance(const cv::Vec4i &line, const cv::Point &point)
{
    double A = line[3] - line[1];
    double B = line[0] - line[2];
    double C = line[2] * line[1] - line[0] * line[3];
    return std::abs(A * point.x + B * point.y + C) / std::sqrt(A * A + B * B);
}

cv::Point getIntersection(const cv::Vec4i &l1, const cv::Vec4i &l2)
{
    double A1 = l1[3] - l1[1];
    double B1 = l1[0] - l1[2];
    double C1 = l1[2] * l1[1] - l1[0] * l1[3];
    double A2 = l2[3] - l2[1];
    double B2 = l2[0] - l2[2];
    double C2 = l2[2] * l2[1] - l2[0] * l2[3];

    int x = (B1 * C2 - B2 * C1) / (B2 * A1 - B1 * A2);
    int y = (A1 * C2 - C1 * A2) / (B1 * A2 - A1 * B2);
    return cv::Point(x, y);
}

void QCR::getVertexes(std::vector<cv::Vec4i> &lines, std::vector<cv::Point> &points)
{
    // ---------- 按斜率分类横向线段和竖向线段 ---------- //
    std::vector<cv::Vec4i> h_lines;
    std::vector<cv::Vec4i> v_lines;
    for (auto line : lines)
    {
        double k = std::abs(static_cast<double>(line[3] - line[1]) / (line[2] - line[0]));
        if (k < 1.0)
        {
            // 确保 x1 < x2
            if (line[0] > line[2])
            {
                int tmp = line[0];
                line[0] = line[2];
                line[2] = tmp;
                tmp = line[1];
                line[1] = line[3];
                line[3] = tmp;
            }
            h_lines.push_back(line);
        }
        else
        {
            // 确保 y1 < y2
            if (line[1] > line[3])
            {
                int tmp = line[0];
                line[0] = line[2];
                line[2] = tmp;
                tmp = line[1];
                line[1] = line[3];
                line[3] = tmp;
            }
            v_lines.push_back(line);
        }
    }
    if (h_lines.size() < 2 || v_lines.size() < 2)
        return;

    // ---------- 合并横向线段 ---------- //
    // 按端点y坐标从小到大排序
    std::sort(h_lines.begin(), h_lines.end(),
        [&](const cv::Vec4i &l1, const cv::Vec4i &l2) {
            return l1[1] < l2[1];
        });
    cv::Vec4i hu_line = h_lines.front();
    cv::Vec4i hb_line = h_lines.back();

    double d1;
    double d2;
    // 判断是否同一条直线的距离阈值
    double threshold = 10.0;
    printLog(QString::fromUtf8(u8"Distance threshold: %1").arg(threshold));
    
    for (size_t i = 1; i < h_lines.size() -1; ++i)
    {
        cv::Vec4i line = h_lines[i];
        d1 = getDistance(line, cv::Point(hu_line[0], hu_line[1]));
        d2 = getDistance(line, cv::Point(hb_line[0], hb_line[1]));
        if (d1 < threshold)
        {
            if (line[0] < hu_line[0])
            {
                hu_line[0] = line[0];
                hu_line[1] = line[1];
            }
            if (line[2] > hu_line[2])
            {
                hu_line[2] = line[2];
                hu_line[3] = line[3];
            }
        }
        else if (d2 < threshold)
        {
            if (line[0] < hb_line[0])
            {
                hb_line[0] = line[0];
                hb_line[1] = line[1];
            }
            if (line[2] > hb_line[2])
            {
                hb_line[2] = line[2];
                hb_line[3] = line[3];
            }
        }
    }

    // ---------- 合并竖向线段 ---------- //
    // 按端点x坐标从小到大排序
    std::sort(v_lines.begin(), v_lines.end(),
        [&](const cv::Vec4i &l1, const cv::Vec4i &l2) {
            return l1[0] < l2[0];
        });
    cv::Vec4i vl_line = v_lines.front();
    cv::Vec4i vr_line = v_lines.back();

    for (size_t i = 1; i < v_lines.size() - 1; ++i)
    {
        cv::Vec4i line = v_lines[i];
        d1 = getDistance(line, cv::Point(vl_line[0], vl_line[1]));
        d2 = getDistance(line, cv::Point(vr_line[0], vr_line[1]));
        if (d1 < threshold)
        {
            if (line[1] < vl_line[1])
            {
                vl_line[0] = line[0];
                vl_line[1] = line[1];
            }
            if (line[3] > vl_line[3])
            {
                vl_line[2] = line[2];
                vl_line[3] = line[3];
            }
        }
        else if (d2 < threshold)
        {
            if (line[1] < vr_line[1])
            {
                vr_line[0] = line[0];
                vr_line[1] = line[1];
            }
            if (line[3] > vr_line[3])
            {
                vr_line[2] = line[2];
                vr_line[3] = line[3];
            }
        }
    }

    lines = { vl_line, hu_line, vr_line, hb_line };

    // ---------- 计算直线的交点 ---------- //
    // 多次转换导致还原后的值相对实际值要小一点
    // 所以通过减的少一些, 加的多一些来补偿
    cv::Point ul = getIntersection(vl_line, hu_line); // up_left
    ul.x -= 2;
    ul.y -= 2;
    cv::Point ur = getIntersection(vr_line, hu_line); // up_right 
    ur.x += 4;
    ur.y -= 2;
    cv::Point br = getIntersection(vr_line, hb_line); // bottom_right
    br.x += 4;
    br.y += 4;
    cv::Point bl = getIntersection(vl_line, hb_line); // bottom_left
    bl.x -= 2;
    bl.y += 4;

    points = { ul, ur, br, bl };
}

void QCR::resizeImage(const QString &path, int len, int sz)
{
    cv::Mat img = cv::imread(path.toLocal8Bit().data());
    double scale = 1.0;
    // 缩小宽高超过 len 像素的图片以加快处理速度
    if (img.rows > len || img.cols > len)
    {
        double r = static_cast<double>(img.rows);
        double c = static_cast<double>(img.cols);
        scale = len / r < len / c ?
            len / r : len / c;
    }
    QFileInfo info(path);
    if (info.size() > sz)
    {
        double _s = std::sqrt(static_cast<double>(sz) / info.size());
        if (_s < scale)
            scale = _s;
    }

    double _sz = info.size() / 1024.0 / 1024.0;
    QString s_sz = QString::number(_sz, 'f', 2);
    if (scale < 1.0)
    {
        printLog(QString::fromUtf8(u8"图片过大(%1 MB, %2x%3), 按比例 %4 压缩")
            .arg(s_sz).arg(img.cols).arg(img.rows).arg(scale));

        cv::resize(img, img, cv::Size(), scale, scale, cv::INTER_AREA);
        cv::imwrite(path.toLocal8Bit().data(), img);

        // 重新获取文件信息
        info.refresh();
        _sz = info.size() / 1024.0 / 1024.0;
        s_sz = QString::number(_sz, 'f', 2);
        printLog(QString::fromUtf8(u8"图片已压缩至: %1 MB, %2x%3").arg(s_sz).arg(img.cols).arg(img.rows));
    }
    else
    {
        printLog(QString::fromUtf8(u8"图片无需压缩: %1 MB, %2x%3").arg(s_sz).arg(img.cols).arg(img.rows));
    }
}

void QCR::edgeDetection()
{
    if (compress_thread->isRunning())
        compress_thread->wait();

    QString path = img_path_cropped.isEmpty() ? img_path : img_path_cropped;
    cv::Mat img = cv::imread(path.toLocal8Bit().data());

    // 缩小图片到宽高不超过 len 像素以加快处理速度
    int len = 1000;
    if (img.rows > len || img.cols > len)
    {
        double r = static_cast<double>(img.rows);
        double c = static_cast<double>(img.cols);
        double scale = len / r < len / c ? len / r : len / c;
        cv::resize(img, img, cv::Size(), scale, scale, cv::INTER_AREA);
    }

    // 检查是否为灰度图，如果不是，转化为灰度图
    cv::Mat gray = img.clone();
    if (img.channels() == 3)
        cvtColor(img, gray, CV_BGR2GRAY);

    // 双边滤波
    cv::Mat blured;
    cv::bilateralFilter(gray, blured, 5, 70, 70);

    // 自动计算阈值
    cv::Mat _tmp;
    double otsu_thresh_val = cv::threshold(
        blured, _tmp, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
    printLog("Otsu thresh value: " + std::to_string(otsu_thresh_val));

    // 边缘检测
    cv::Mat canny;
    Canny(blured, canny, 0.5 * otsu_thresh_val, otsu_thresh_val);
    // 如果表格外围没有更多文字或其他干扰因素, 加上以下两行应该可以获得更好的轮廓
    //cv::dilate(canny, canny, cv::Mat());
    //Canny(canny, canny, 50, 150);

    std::vector<std::vector<cv::Point>> contours;
    // 只检测外围轮廓, 内轮廓被忽略
    findContours(canny, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
    double area = 0;
    int index = 0;
    for (int i=0; i < contours.size(); ++i)
    {
        double a = cv::contourArea(contours[i]);
        if (a > area)
        {
            // 折线化
            approxPolyDP(contours[i], contours[i], 10, true);
            area = a;
            index = i;
        }
    }

    // 创建纯黑灰度图
    cv::Mat black = cv::Mat::zeros(gray.size(), CV_8U);
    cv::drawContours(black, contours, index, cv::Scalar(255, 255, 255), 1);

    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(black, lines, 1, CV_PI / 180, 50, 100, 50);
    printLog(QString::fromUtf8(u8"Detected lines: %1").arg(lines.size()));

    // 四个顶点: 左上、右上、右下、左下
    std::vector<cv::Point> points;
    getVertexes(lines, points);
    if (points.size() != 4)
        return;
    
    //cv::Mat dst = img.clone();
    //for (size_t i = 0; i < lines.size(); i++)
    //{
    //    cv::Vec4i l = lines[i];
    //    cv::line(dst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]),
    //        cv::Scalar(0, 0, 255), 1);
    //}

    std::vector<std::vector<double>> points_rel;
    for (const auto &p : points)
    {
        double _x = static_cast<double>(p.x) / img.cols;
        double _y = static_cast<double>(p.y) / img.rows;
        printLog(QString::fromUtf8(u8"x_rel = %1, y = %2").arg(_x).arg(_y));
        points_rel.push_back({ _x, _y });
    }

    ui.ui_img_widget->setInterceptBox(points_rel);
}

void QCR::processImage()
{
    QString path = img_path_cropped.isEmpty() ? img_path : img_path_cropped;
    cv::Mat img = cv::imread(path.toLocal8Bit().data());

    // 检查是否为灰度图，如果不是，转化为灰度图
    cv::Mat gray = img.clone();
    if (img.channels() == 3)
        cvtColor(img, gray, CV_BGR2GRAY);

    // 均值滤波
    cv::Mat blured;
    //blur(proc, b1, cv::Size(3, 3));
    //GaussianBlur(proc, b2, cv::Size(3, 3), 0);
    cv::bilateralFilter(gray, blured, 7, 75, 75);

    // 自适应均衡化，提高对比度，裁剪效果更好
    cv::Mat proc;
    cv::Ptr<cv::CLAHE> clahe = createCLAHE(1, cv::Size(10, 10));
    clahe->apply(blured, proc);

    // 阈值化，转化为黑白图片
    adaptiveThreshold(proc, proc, 255,
        CV_ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 15, 10);

    // 形态学, 保留较长的横竖线条
    // 黑底白字, 腐蚀掉白字
    cv::Mat struct_h = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(40, 1));
    cv::Mat mat_h;
    erode(proc, mat_h, struct_h, cv::Size(-1, -1), 2);
    dilate(mat_h, mat_h, struct_h, cv::Size(-1, -1), 2);

    // 边缘检测
    cv::Mat canny_h;
    Canny(mat_h, canny_h, 50, 200);

    std::vector<cv::Vec4i> h_lines;
    // 距离精度1像素、角度精度1°、最少交点数200、最小长度200、最长断连50像素
    HoughLinesP(canny_h, h_lines, 1, CV_PI / 180, 50, 100, 50);
    std::vector<cv::Vec3d> lines_h = mergeLines(h_lines, true);
    printLog(QString::fromUtf8("rows = %1").arg(lines_h.size()));

    cv::Mat struct_v = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(1, 40));
    cv::Mat mat_v;
    erode(proc, mat_v, struct_v, cv::Size(-1, -1), 2);
    dilate(mat_v, mat_v, struct_v, cv::Size(-1, -1), 2);

    // 边缘检测
    cv::Mat canny_v;
    Canny(mat_v, canny_v, 50, 200);

    std::vector<cv::Vec4i> v_lines;
    // 距离精度1像素、角度精度1°、最少交点数200、最小长度200、最长断连50像素
    HoughLinesP(canny_v, v_lines, 1, CV_PI / 180, 50, 100, 50);
    std::vector<cv::Vec3d> lines_v = mergeLines(v_lines, false);
    printLog(QString::fromUtf8("cols = %1").arg(lines_v.size()));

    cv::Mat mat_table;
    bitwise_and(mat_h, mat_v, mat_table);

    //提取轮廓
    std::vector<cv::Vec4i> hierarchy;
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mat_table, contours, hierarchy,
        CV_RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    // 计算中点坐标
    std::vector<cv::Point> points_list;
    for (auto &contour : contours)
    {
        int sum_x = 0;
        int sum_y = 0;
        for (auto &p : contour)
        {
            sum_x += p.x;
            sum_y += p.y;
        }
        int x = sum_x / contour.size();
        int y = sum_y / contour.size();
        points_list.push_back(cv::Point(x, y));
    }
    printLog(QString::fromUtf8("Point count = ").arg(points_list.size()));

    return;
}

void QCR::updateTableCell(int row, int col, int row_span, int col_span, const QString &text)
{
    // 确保行列数足够
    if (ui.ui_table_widget->rowCount() < row + row_span)
        ui.ui_table_widget->setRowCount(row + row_span);
    if (ui.ui_table_widget->columnCount() < col + col_span)
        ui.ui_table_widget->setColumnCount(col + col_span);

    // 设置该格子内容
    QTableWidgetItem *item = ui.ui_table_widget->item(row, col);
    if (item)
    {
        item->setText(text);
        ui.ui_table_widget->setItem(row, col, item);
    }
    else
    {
        ui.ui_table_widget->setItem(row, col, new QTableWidgetItem(text));
    }

    // 设置跨度
    if (row_span > 1 || col_span > 1)
        ui.ui_table_widget->setSpan(row, col, row_span, col_span);
}

void QCR::updateTable()
{
    resetTable();
    // C++17
    for (auto &[srow, cells] : ocr_result.items())
    {
        int row = std::stoi(srow);
        for (auto &[scol, cell] : cells.items())
        {
            int col = std::stoi(scol);
            int row_span = cell.at("row_span");
            int col_span = cell.at("col_span");
            std::string _s = cell.at("text");
            updateTableCell(row, col, row_span, col_span, QString::fromUtf8(_s.c_str()));
        }
    }
}

void QCR::resetTable()
{
    ui.ui_table_widget->clearContents();
    ui.ui_table_widget->clearSpans();
}

void QCR::txParseData(const std::string &str)
{
    json result_data = json::parse(str);
    std::vector<json> tables = result_data.at("Response").at("TableDetections");
    if (tables.size() == 0)
        return;
    
    // 默认图片仅含一个表格, 选取返回数据中最多单元格数的作为识别结果
    std::vector<json> cells;
    for (int index = 0; index < tables.size(); ++index)
    {
        std::vector<json> tmp = tables[index].at("Cells");
        if (tmp.size() > cells.size())
            cells = tmp;
    }

    ocr_result.clear();
    for (const auto &cell : cells)
    {
        // 最小值作为该格子的左上角单元格坐标
        int row = cell.at("RowTl");
        std::string srow = std::to_string(row);
        int col = cell.at("ColTl");
        std::string scol = std::to_string(col);

        int row_span = cell.at("RowBr") - row;
        int col_span = cell.at("ColBr") - col;

        // 去除非中英文和数字
        std::string text = cell.at("Text");
        QString _text = QString::fromUtf8(text.c_str());
        _text.remove(QRegularExpression(u8"[^一-龥a-zA-Z0-9]+"));
        text = _text.toStdString();

        std::vector<std::vector<int>> polygon;
        for (json point : cell.at("Polygon"))
            polygon.push_back({ point.at("X"), point.at("Y") });

        // 存到 ocr_result
        if(!ocr_result.contains(srow))
            ocr_result += {srow, json::object()};
        json _cell = {
            {"row_span", row_span},
            {"col_span", col_span},
            {"text", text},
            {"polygon", polygon}
        };
        ocr_result[srow] += {scol, _cell};
    }
}

void QCR::bdParseData(const std::string &str)
{
    json result = json::parse(str);
    std::string result_data_str = result.at("result").at("result_data");
    json result_data = json::parse(result_data_str);
    if (result_data.at("form_num") == 0)
        return;
    auto table = result_data.at("forms").at(0).at("body");

    ocr_result.clear();
    for (const auto &cell : table)
    {
        std::cout << cell << endl;
        // 占据的行列数列表
        std::vector<int> rols = cell.at("row");
        std::vector<int> cols = cell.at("column");

        // 最小值作为该格子的左上角单元格坐标
        int row = *std::min_element(rols.begin(), rols.end()) - 1;
        std::string srow = std::to_string(row);
        int col = *std::min_element(cols.begin(), cols.end()) - 1;
        std::string scol = std::to_string(col);

        int row_span = *std::max_element(rols.begin(), rols.end()) - row;
        int col_span = *std::max_element(cols.begin(), cols.end()) - col;

        std::string text = cell.at("word");
        QString _text = QString::fromUtf8(text.c_str());
        _text.remove(QRegularExpression(u8"[^一-龥a-zA-Z0-9]+"));
        text = _text.toStdString();

        int left = cell.at("rect").at("left");
        int top = cell.at("rect").at("top");
        int right = left + cell.at("rect").at("width");
        int bottom = top + cell.at("rect").at("height");

        std::vector<std::vector<int>> polygon = {
            {left, top}, {right, top}, {right, bottom}, {left, bottom} };

        // 存到 ocr_result
        if (!ocr_result.contains(srow))
            ocr_result += {srow, json::object()};
        json _cell = {
            {"row_span", row_span},
            {"col_span", col_span},
            {"text", text},
            {"polygon", polygon}
        };
        ocr_result[srow] += {scol, _cell};
    }
}

void QCR::getScoreColumn(std::vector<std::vector<int>>& rects)
{
    for (int j = 0; j < ui.ui_table_widget->columnCount(); ++j)
    {
        // 分数列的像素范围
        std::vector<int> ls;
        std::vector<int> rs;
        std::vector<int> ts;
        std::vector<int> bs;

        int cnt_all = 0;
        int cnt_score = 0;
        bool is_score_column = false;
        for (int i = 0; i < ui.ui_table_widget->rowCount(); ++i)
        {
            std::string srow = std::to_string(i);
            std::string scol = std::to_string(j);

            if (!ocr_result.contains(srow))
                continue;
            if (!ocr_result.at(srow).contains(scol))
                continue;

            ++cnt_all;

            auto &rect = ocr_result.at(srow).at(scol).at("polygon");

            int l = (*std::min_element(rect.begin(), rect.end(),
                [=](auto p1, auto p2) { return p1.at(0) < p2.at(0); })).at(0);
            int r = (*std::max_element(rect.begin(), rect.end(),
                [=](auto p1, auto p2) { return p1.at(0) < p2.at(0); })).at(0);
            int t = (*std::min_element(rect.begin(), rect.end(),
                [=](auto p1, auto p2) { return p1.at(1) < p2.at(1); })).at(1);
            int b = (*std::max_element(rect.begin(), rect.end(),
                [=](auto p1, auto p2) { return p1.at(1) < p2.at(1); })).at(1);

            ls.push_back(l);
            rs.push_back(r);
            ts.push_back(t);
            bs.push_back(b);

            std::string _text = ocr_result.at(srow).at(scol).at("text");
            QString text = QString::fromUtf8(_text.c_str());

            // 如果包含"平"、"时"、"成"、"绩"任意一个字，则认为这一行以下为分数区域
            // 表头为印刷体, 一般都能识别出并匹配到相关字符
            if (text.contains(QRegularExpression(u8"[平时成绩]+")))
            {
                printLog(QString::fromUtf8(u8"匹配到某个字符(平、时、成、绩): %1").arg(text));
                if (text.contains(u8"平时") || text.contains(u8"成绩"))
                    is_score_column = true;
                // 重新开始计数
                cnt_all = 0;
                cnt_score = 0;
                ls.clear();
                rs.clear();
                ts.clear();
                bs.clear();
                ls.push_back(l);
                rs.push_back(r);
                ts.push_back(b); // 不包括该单元格
                bs.push_back(b);
            }

            int has_zh = text.contains(QRegularExpression(u8"[一-龥]+"));
            int has_en = text.contains(QRegularExpression(u8"[a-zA-Z]+"));
            int has_num = text.contains(QRegularExpression(u8"[0-9]+"));
            // 文本中包含的类型数量, 范围[0-3], 即[空、中文、英文、数字]
            int type = has_zh + has_en + has_num;
            int sz = text.size();

            // 类型不止一种且字符数不超过2两个
            if (sz <= 2 && type > 1)
                ++cnt_score;
            // 只有数字一种类型且字符数不超过两个
            if (sz <= 2 && type == 1 && has_num)
                ++cnt_score;
        }
        if (is_score_column || (!is_score_column && 4 * cnt_score > cnt_all))
        {
            // 删除偏差较大的right像素值
            double ave;
            double sd;
            calAveSd(rs, ave, sd);
            for (int i = 0; i < rs.size(); ++i)
            {
                if (rs[i] < ave - sd || rs[i] > ave + sd)
                {
                    ls.erase(ls.begin() + i);
                    rs.erase(rs.begin() + i);
                    ts.erase(ts.begin() + i);
                    bs.erase(bs.begin() + i);
                }
            }
            int left = *std::min_element(ls.begin(), ls.end());
            int right = *std::max_element(rs.begin(), rs.end());
            int top = *std::min_element(ts.begin(), ts.end());
            int bottom = *std::max_element(bs.begin(), bs.end());
            rects.push_back({ j, left, right,top,bottom });
            printLog(QString::fromUtf8(u8"分数列: { %1, %2, %3, %4, %5 }").arg(j).arg(left).arg(right).arg(top).arg(bottom));
        }
    }

    if (rects.empty())
        return;
    // 从左到右排序
    std::sort(rects.begin(), rects.end(), [=](auto rc1, auto rc2) { return rc1[1] < rc2[1]; });
    for (int i = 1; i < rects.size(); ++i)
    {
        if (rects[i - 1][2] - rects[i][1] >
            std::min(rects[i - 1][2] - rects[i - 1][1], rects[i][2] - rects[i][1]) / 2)
        {
            rects[i - 1][2] = std::max(rects[i - 1][2], rects[i][2]);
            rects[i - 1][3] = std::min(rects[i - 1][3], rects[i][3]);
            rects[i - 1][4] = std::max(rects[i - 1][4], rects[i][4]);
            printLog(QString::fromUtf8(u8"合并%1,%2列: { %3, %4, %5, %6}")
                .arg(rects[i - 1][0]).arg(rects[i][0])
                .arg(rects[i - 1][1]).arg(rects[i - 1][2]).arg(rects[i - 1][3]).arg(rects[i - 1][4]));
            rects.erase(rects.begin() + i);
            --i;
        }
    }
}

void QCR::calAveSd(const std::vector<double> &vec, double &ave, double &sd)
{
    if (vec.empty())
        return;
    int n = vec.size();
    ave = std::accumulate(vec.begin(), vec.end(), 0.0) / n;
    sd = std::sqrt(std::accumulate(vec.begin(), vec.end(), 0.0,
        [=](double sum, double d) {return sum += std::pow(d - ave, 2); }) / n);
}

void QCR::calAveSd(const std::vector<int> &vec, double &ave, double &sd)
{
    std::vector<double> v;
    for (auto val : vec)
        v.push_back(static_cast<double>(val));
    return calAveSd(v, ave, sd);
}

void QCR::interceptImage()
{
    if (img_path.isEmpty())
    {
        MyMessageBox msg(QString::fromUtf8(u8"请先打开一张图片!"));
        msg.exec();
        return;
    }

    QString path = img_path_cropped.isEmpty() ? img_path : img_path_cropped;
    cv::Mat img = cv::imread(path.toLocal8Bit().data());
    std::vector<std::vector<double>> points_rel;
    ui.ui_img_widget->getVertex(points_rel);

    std::vector<std::vector<int>> points;
    for (const auto &p : points_rel)
    {
        int _x = static_cast<int>(p[0] * img.cols);
        int _y = static_cast<int>(p[1] * img.rows);
        points.push_back({ _x, _y });
    }

    // 选取区域的顶点
    cv::Point2f pointsf[4];
    pointsf[0] = cv::Point2f(points[0][0], points[0][1]);
    pointsf[1] = cv::Point2f(points[1][0], points[1][1]);
    pointsf[2] = cv::Point2f(points[2][0], points[2][1]);
    pointsf[3] = cv::Point2f(points[3][0], points[3][1]);

    int width = std::sqrt(std::pow(points[1][0] - points[0][0], 2) + std::pow(points[1][1] - points[0][1], 2));
    int height = std::sqrt(std::pow(points[3][0] - points[0][0], 2) + std::pow(points[3][1] - points[0][1], 2));

    // 变换后的顶点
    cv::Point2f pts_std[4];
    pts_std[0] = cv::Point2f(0., 0.);
    pts_std[1] = cv::Point2f(width, 0.);
    pts_std[2] = cv::Point2f(width, height);
    pts_std[3] = cv::Point2f(0., height);

    // 透视变换
    cv::Mat M = cv::getPerspectiveTransform(pointsf, pts_std);
    cv::Mat dst_img;
    cv::warpPerspective(img, dst_img, M, cv::Size(width, height), cv::BORDER_REPLICATE);

    QFileInfo info(img_path);
    img_path_cropped = QString::fromUtf8(u8"./tmp/%1_cropped.%2").arg(info.baseName()).arg(info.suffix());
    cv::imwrite(img_path_cropped.toLocal8Bit().data(), dst_img);
    ui.ui_img_widget->setPix(QPixmap(img_path_cropped));
}

void QCR::undo()
{
    QFile file(img_path_cropped);
    if (file.exists())
    {
        // 删除裁剪后的图片并显示原图片
        file.remove();
        img_path_cropped.clear();
        ui.ui_img_widget->setPix(QPixmap(img_path));
    }
}

void QCR::closeEvent(QCloseEvent *event)
{
    // 清理 tmp 文件夹
    QDir dir("./tmp");
    if (dir.exists())
    {
        if (!dir.removeRecursively())
        {
            MyMessageBox msg(QString::fromUtf8(u8"尝试清理 tmp 文件夹失败!"));
            msg.exec();
            event->accept();
        }
    }

    // 等待初始化线程结束
    if (initial_thread->isRunning())
    {
        initial_thread->quit();
        MyMessageBox msg(QString::fromUtf8(u8"程序将在初始化线程结束后关闭!"));
        msg.setWindowIcon(QIcon(":/images/logo.png"));
        connect(initial_thread, &QThread::finished, &msg, &QDialog::accept);
        msg.exec();
        initial_thread->wait();
        event->accept();
    }
}
