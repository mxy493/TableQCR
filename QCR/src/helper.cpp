#include <QDateTime>
#include <QtDebug>
#include <QDir>
#include <QFileInfo>

#include <opencv2/opencv.hpp>

#include <algorithm> // for std::min

#include <include/base64.h>
#include <include/helper.h>
#include "QPixmap"

QString getCurTimeString()
{
    QDateTime now = QDateTime::currentDateTime();
    return now.toString(QString("yyyy-MM-dd_hh-mm-ss"));
}

void printLog(const QString &log, bool save)
{
    // 不添加空格, 不添加引号, 但末尾自动添加换行
    QDateTime now = QDateTime::currentDateTime();
    QString time = now.toString(QString("yyyy-MM-dd_hh-mm-ss-zzz"));
    QString log_str = time + ": " + log;
    qDebug().nospace().noquote() << log_str;

    if (save)
    {
        if (!QDir("log").exists())
            QDir().mkdir("log");
        QFile file(QString("./log/") + now.toString(QString("yyyy-MM-dd")) + QString(".log"));
        if (file.open(QIODevice::Append))
        {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << log_str << "\n";
            out.flush();
            file.close();
        }
    }
}

void printLog(const std::string &log, bool save)
{
    printLog(QString::fromUtf8(log.c_str()), save);
}

void printLog(const char *log, bool save)
{
    printLog(QString(log), save);
}

void calAveSd(const std::vector<double> &vec, double &ave, double &sd)
{
    if (vec.empty())
        return;
    int n = vec.size();
    ave = std::accumulate(vec.begin(), vec.end(), 0.0) / n;
    sd = std::sqrt(std::accumulate(vec.begin(), vec.end(), 0.0,
        [=](double sum, double d) {return sum += std::pow(d - ave, 2); }) / n);
}

void calAveSd(const std::vector<int> &vec, double &ave, double &sd)
{
    std::vector<double> v;
    for (auto val : vec)
        v.push_back(static_cast<double>(val));
    return calAveSd(v, ave, sd);
}

std::string get_data(const int64_t &timestamp)
{
    std::string utcDate;
    char buff[20] = { 0 };
    // time_t timenow;
    struct tm sttime;
    sttime = *gmtime(&timestamp);
    strftime(buff, sizeof(buff), "%Y-%m-%d", &sttime);
    utcDate = std::string(buff);
    return utcDate;
}

QImage cvMatToQImage(const cv::Mat &mat)
{
    switch (mat.type())
    {
        // 8-bit, 4 channel
    case CV_8UC4:
    {
        QImage image(mat.data,
            mat.cols, mat.rows,
            static_cast<int>(mat.step),
            QImage::Format_ARGB32);
        return image;
    }

    // 8-bit, 3 channel
    case CV_8UC3:
    {
        QImage image(mat.data,
            mat.cols, mat.rows,
            static_cast<int>(mat.step),
            QImage::Format_RGB888);
        return image.rgbSwapped();
    }

    // 8-bit, 1 channel
    case CV_8UC1:
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
        QImage image(mat.data,
            mat.cols, mat.rows,
            static_cast<int>(mat.step),
            QImage::Format_Grayscale8);
#else
        static QVector<QRgb>  sColorTable;

        // only create our color table the first time
        if (sColorTable.isEmpty())
        {
            sColorTable.resize(256);
            for (int i = 0; i < 256; ++i)
            {
                sColorTable[i] = qRgb(i, i, i);
            }
        }

        QImage image(mat.data,
            mat.cols, mat.rows,
            static_cast<int>(mat.step),
            QImage::Format_Indexed8);
        image.setColorTable(sColorTable);
#endif
        return image;
    }

    default:
        printLog(QString("cvMatToQImage(): cv::Mat image type not handled in switch: %1").arg(mat.type()));
        break;
    }

    return QImage();
}

QPixmap cvMatToQPixmap(const cv::Mat &mat)
{
    return QPixmap::fromImage(cvMatToQImage(mat));
}

cv::Mat QImageToCvMat(const QImage &inImage, bool inCloneImageData)
{
    switch (inImage.format())
    {
        // 8-bit, 4 channel
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    {
        cv::Mat mat(inImage.height(), inImage.width(),
            CV_8UC4,
            const_cast<uchar *>(inImage.bits()),
            static_cast<size_t>(inImage.bytesPerLine())
        );
        return (inCloneImageData ? mat.clone() : mat);
    }

    // 8-bit, 3 channel
    case QImage::Format_RGB32:
    {
        cv::Mat mat(inImage.height(), inImage.width(),
            CV_8UC4,
            const_cast<uchar *>(inImage.bits()),
            static_cast<size_t>(inImage.bytesPerLine())
        );
        cv::Mat matNoAlpha;
        cv::cvtColor(mat, matNoAlpha, cv::COLOR_BGRA2BGR);   // drop the all-white alpha channel
        return matNoAlpha;
    }

    // 8-bit, 3 channel
    case QImage::Format_RGB888:
    {
        QImage swapped = inImage.rgbSwapped();
        return cv::Mat(swapped.height(), swapped.width(),
            CV_8UC3,
            const_cast<uchar *>(swapped.bits()),
            static_cast<size_t>(swapped.bytesPerLine())
        ).clone();
    }

    // 8-bit, 1 channel
    case QImage::Format_Indexed8:
    {
        cv::Mat  mat(inImage.height(), inImage.width(),
            CV_8UC1,
            const_cast<uchar *>(inImage.bits()),
            static_cast<size_t>(inImage.bytesPerLine())
        );
        return (inCloneImageData ? mat.clone() : mat);
    }

    default:
        printLog(QString("ASM::QImageToCvMat() - QImage format not handled in switch:").arg(inImage.format()));
        break;
    }

    return cv::Mat();
}

cv::Mat QPixmapToCvMat(const QPixmap &inPixmap, bool inCloneImageData)
{
    return QImageToCvMat(inPixmap.toImage(), inCloneImageData);
}

bool are_same_column(int l1, int r1, int l2, int r2)
{
    bool ret = false;
    int cmp = (std::min)(r1 - l1, r2 - l2) / 4;
    // Keep l1 <= l2
    if (l1 > l2)
    {
        int tmp = l1;
        l1 = l2;
        l2 = tmp;
        tmp = r1;
        r1 = r2;
        r2 = tmp;
    }
    if (r1 - l2 > cmp)
        ret = true;
    return ret;
}

std::vector<std::vector<cv::Point>> recognizeTable(const cv::String path)
{
    // 读取图片并检查是否成功
    cv::Mat mat = cv::imread(path);
    if (mat.empty())
    {
        printLog("Error opening image.");
        return std::vector<std::vector<cv::Point>>();
    }

    // 检查是否为灰度图，如果不是，转化为灰度图
    cv::Mat proc = mat.clone();
    if (mat.channels() == 3)
        cvtColor(mat, proc, CV_BGR2GRAY);

    // 降噪
    blur(proc, proc, cv::Size(3, 3));

    // 自适应均衡化，提高对比度，裁剪效果更好
    cv::Ptr<cv::CLAHE> clahe = createCLAHE(1, cv::Size(10, 10));
    clahe->apply(proc, proc);

    // 阈值化，转化为黑白图片
    adaptiveThreshold(proc, proc, 255,
        CV_ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 15, 10);

    // 膨胀, 以方便用更细长的矩形过滤掉文字
    cv::Mat struct_square = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(3, 3));
    dilate(proc, proc, struct_square);

    // 形态学, 保留较长的横竖线条
    // 黑底白字, 腐蚀掉白字
    cv::Mat struct_h = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(30, 1));
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
    printLog("rows = " + std::to_string(lines_h.size()));

    cv::Mat struct_v = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(1, 30));
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
    printLog("cols = " + std::to_string(lines_v.size()));

    /*
    // Draw the lines
    cv::Mat m = mat.clone();
    for (size_t i = 0; i < lines_h.size(); ++i)
    {
        int x1 = 0;
        int y1 = -lines_h[i][2] / lines_h[i][1];
        int x2 = mat.cols;
        int y2 = -(lines_h[i][0] * x2 + lines_h[i][2]) / lines_h[i][1];
        line(m, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 1, LINE_AA);
    }
    for (size_t i = 0; i < lines_v.size(); ++i)
    {
        int y1 = 0;
        int x1 = -lines_v[i][2] / lines_v[i][0];
        int y2 = mat.rows;
        int x2 = -(lines_v[i][1] * y2 + lines_v[i][2]) / lines_v[i][0];
        line(m, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 1, LINE_AA);
    }
    imshow("m", m);
    while (uchar(waitKey()) == 'q') {}
    */

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
    printLog("Point count = " + std::to_string(points_list.size()));

    /* points_matrix:
    * p1 p4 p7
    * p2 p5 p8
    * p3 p6 p9
    */
    std::vector<std::vector<cv::Point>> points_matrix;
    if (!points_list.empty())
    {
        points_matrix = formatPointMatrix(points_list, lines_h, lines_v);
    }

    /* 返回标准格点矩阵(转置):
    * p1 p2 p3
    * p4 p5 p6
    * p7 p8 p9
    */
    return transpose(points_matrix);
}

std::vector<cv::Vec3d> mergeLines(std::vector<cv::Vec4i> &lines, bool horizontal)
{
    // 返回值 (A, B, C), 直线一般方程参数: Ax + By + C = 0
    std::vector<cv::Vec3d> ret;
    std::sort(lines.begin(), lines.end(),
        [=](const cv::Vec4i &l1, const cv::Vec4i &l2) {
            return horizontal ? l1[1] < l2[1] : l1[0] < l2[0];
        });
    double d = sqrt((lines[0][0] - lines[0][2]) * (lines[0][0] - lines[0][2])
        + (lines[0][1] - lines[0][3]) * (lines[0][1] - lines[0][3]));
    // 最长线的索引和长度
    std::pair<int, double> mark = { 0,  d };
    for (int i = 1; i < lines.size(); ++i)
    {
        int x1 = lines[i][0];
        int y1 = lines[i][1];
        int x2 = lines[i][2];
        int y2 = lines[i][3];
        double distance = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
        // 先判断是否新的一行（列）
        if ((horizontal && abs(y1 - lines[i - 1][1]) > 10)
            || (!horizontal && abs(x1 - lines[i - 1][0]) > 10)
            || i == lines.size() - 1)
        {
            int x11 = lines[mark.first][0];
            int y11 = lines[mark.first][1];
            int x22 = lines[mark.first][2];
            int y22 = lines[mark.first][3];
            double A = y22 - y11;  // A = y2 - y1
            double B = x11 - x22;  // B = x1 - x2
            double C = x22 * y11 - x11 * y22;  // C = x2*y1 - x1*y2
            ret.push_back({ A, B, C });

            // 更新mark为当前元素
            mark = { i, distance };
            continue;
        }
        if (distance > mark.second)
            mark = { i, distance };
    }
    return ret;
}

std::vector<std::vector<cv::Point>> formatPointMatrix(
    std::vector<cv::Point> points_list,
    const std::vector<cv::Vec3d> &lines_h,
    const std::vector<cv::Vec3d> &lines_v)
{
    std::vector<std::vector<cv::Point>> matrix;
    // matrix 的行数列数
    int rows = lines_h.size();
    int cols = lines_v.size();

    if (points_list.size() < 4 || rows < 2 || cols < 2)
    {
        printLog("Unable to form a table.");
        return matrix;
    }

    // 按横坐标 x 排序
    std::sort(points_list.begin(), points_list.end(),
        [](const cv::Point &p1, const cv::Point &p2) {
            return p1.x < p2.x;
        });

    // 找出同一列的点, 一般两列之间间距大一些便于区分
    for (auto &v : lines_v)
    {
        // 1. 找出距离一条竖线一定范围内的点, 认为是同一列的点
        std::vector<cv::Point> vec;
        double A = v[0];
        double B = v[1];
        double C = v[2];
        size_t i = 0;
        while (i != points_list.size())
        {
            cv::Point p = points_list[i];
            double d = abs(A * p.x + B * p.y + C) / sqrt(A * A + B * B);
            if (d < 20)
            {
                vec.push_back(p);
                points_list.erase(points_list.begin() + i);
                continue;
            }
            ++i;
        }
        // 按纵坐标 y 从小到大排序
        std::sort(vec.begin(), vec.end(),
            [](const cv::Point &p1, const cv::Point &p2) {
                return p1.y < p2.y;
            });

        // 2. 这一列点的数量大于横线数, 即存在多余点, 需要删除
        if (vec.size() > rows)
        {
            std::vector<cv::Point> copy = vec;
            vec.clear();

            // 对每条横线找距离最近的点并保留, 遍历完所有横线后剩余的点丢弃
            for (auto &ll : lines_h)
            {
                double AA = ll[0];
                double BB = ll[1];
                double CC = ll[2];
                // 距离横线ll最近的点的索引
                size_t min_index = 0;
                double dist = abs(AA * copy[0].x + BB * copy[0].y + CC)
                    / sqrt(AA * AA + BB * BB);

                size_t i = 1;
                while (i != copy.size())
                {
                    double dd = abs(AA * copy[i].x + BB * copy[i].y + CC)
                        / sqrt(AA * AA + BB * BB);
                    if (dd < dist)
                    {
                        min_index = i;
                        dist = dd;
                    }
                    ++i;
                }
                vec.push_back(copy[min_index]);
                copy.erase(copy.begin() + min_index);
            }
        }

        // 3. 点的数量小于行数, 即缺失部分点, 需要补全
        else if (vec.size() < rows)
        {
            std::vector<cv::Point> copy = vec;
            vec.clear();
            vec.resize(rows, cv::Point(-1. - 1));

            // 把已有的点填到距离最近的横线的行数
            for (auto &p : copy)
            {
                size_t min_index = 0;
                double dist = abs(lines_h[0][0] * p.x + lines_h[0][1] * p.y + lines_h[0][2])
                    / sqrt(lines_h[0][0] * lines_h[0][0] + lines_h[0][1] * lines_h[0][1]);
                size_t i = 0;
                while (i != lines_h.size())
                {
                    double AA = lines_h[i][0];
                    double BB = lines_h[i][1];
                    double CC = lines_h[i][2];
                    double dd = abs(AA * p.x + BB * p.y + CC)
                        / sqrt(AA * AA + BB * BB);
                    if (dd < dist)
                    {
                        min_index = i;
                        dist = dd;
                    }
                    ++i;
                }
                vec[min_index] = p;
            }

            // 缺失的点用两直线交点补齐
            for (size_t i = 0; i < vec.size(); ++i)
            {
                if (vec[i] == cv::Point(-1. - 1))
                {
                    double A2 = lines_h[i][0];
                    double B2 = lines_h[i][1];
                    double C2 = lines_h[i][2];
                    int x0 = static_cast<int>((B * B2 - B2 * C) / (B2 * A - B * A2));
                    int y0 = static_cast<int>((A * C2 - C * A2) / (B * A2 - A * B2));
                    vec[i] = { x0, y0 };
                }
            }
        }
        matrix.push_back(vec);
    }
    return matrix;
}

void removeText(const cv::Mat &src)
{
    cv::Mat proc = src.clone();

    // 检查是否为灰度图，如果不是，转化为灰度图
    if (proc.channels() == 3)
        cvtColor(proc, proc, CV_BGR2GRAY);

    // 降噪
    blur(proc, proc, cv::Size(3, 3));

    // 自适应均衡化，提高对比度，裁剪效果更好
    cv::Ptr<cv::CLAHE> clahe = createCLAHE(1, cv::Size(10, 10));
    clahe->apply(proc, proc);

    // 阈值化，转化为黑白图片
    adaptiveThreshold(proc, proc, 255, CV_ADAPTIVE_THRESH_MEAN_C,
        cv::THRESH_BINARY_INV, 15, 10);

    // 膨胀, 以方便用更细长的矩形过滤掉文字
    cv::Mat struct_square = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(3, 3));
    dilate(proc, proc, struct_square);

    // 形态学, 保留较长的横竖线条
    // 黑底白字, 腐蚀掉白字
    cv::Mat struct_h = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(30, 1));
    cv::Mat mat_h;
    erode(proc, mat_h, struct_h, cv::Size(-1, -1), 2);
    dilate(mat_h, mat_h, struct_h, cv::Size(-1, -1), 2);

    cv::Mat struct_v = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(1, 30));
    cv::Mat mat_v;
    erode(proc, mat_v, struct_v, cv::Size(-1, -1), 2);
    dilate(mat_v, mat_v, struct_v, cv::Size(-1, -1), 2);

    cv::Mat mat_table;
    bitwise_or(mat_h, mat_v, mat_table);

    cv::Mat canny;
    Canny(mat_table, canny, 50, 200);

    return;
}

template<typename T>
std::vector<std::vector<T>> transpose(std::vector<std::vector<T>> &vec)
{
    std::vector<std::vector<T>> trans;
    for (int i = 0; i < vec[0].size(); ++i)
    {
        std::vector<T> v;
        for (int j = 0; j < vec.size(); ++j)
        {
            v.push_back(vec[j][i]);
        }
        trans.push_back(v);
    }
    return trans;
}
