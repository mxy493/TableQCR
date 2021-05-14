#pragma once

#include <QThread>

#include <opencv2/core.hpp>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>

#include "ui_qcr.h"
#include "include/config_dialog.h"
#include "include/about_dialog.h"
#include "include/loading_animation.h"

// 图片最大大小, 过大的图片需要先压缩
constexpr int IMG_LENGTH = 4096;
constexpr int IMG_SIZE = 4 * 1024 * 1024;


class QCR : public QMainWindow
{
    Q_OBJECT

public:
    QCR(QWidget *parent = Q_NULLPTR);
    void getBdAccessToken();
    void resizeImage(const QString &path, int len = IMG_LENGTH, int sz = IMG_SIZE);
    // 边缘检测主要是拿到4个顶点的相对坐标, 可以将图片缩小以加快处理速度
    void edgeDetection();
    void processImage();
    void updateTableCell(int row, int col, int row_span, int col_span, const QString &text);
    void updateTable();
    void resetTable();
    void txParseData(const std::string &str);
    void bdParseData(const std::string &str);
    // 根据某一列的文本内容判断其是否是分数列, 返回所有分数列的像素范围及其对应的列数[col, left, right, top, bottom]
    void getScoreColumn(std::vector<std::vector<int>> &rects);
    // 计算平均值和标准差
    void calAveSd(const std::vector<double> &vec, double &ave, double &sd);
    void calAveSd(const std::vector<int> &vec, double &ave, double &sd);
    void getVertexes(std::vector<cv::Vec4i> &lines, std::vector<cv::Point> &points);
    void closeEvent(QCloseEvent *event);

public slots:
    void openImage();
    void interceptImage();
    void undo();
    void runOcr();
    void runTxOcr(const std::string &base64_img);
    void runBdOcr(const std::string &base64_img);
    void exportTableData();
    void recognize();

private:
    Ui::QCRClass ui;

    QThread *initial_thread;
    QThread *compress_thread;
    QThread *detect_table_thread;

    ConfigDialog config_dialog;  // 设置对话框
    AboutDialog about_dlg;       // 关于对话框
    LoadingAnimation animation;  // 加载动画

    QAction *open_action;    // 打开按钮
    QAction *crop_action;    // 裁剪按钮
    QAction *undo_action;      // 恢复按钮
    QAction *ocr_action;     // 识别按钮
    QAction *export_action;  // 导出按钮
    QAction *config_action;  // 设置按钮
    QAction *about_action;   // 关于按钮

    QAction *test1_action;   // 测试1

    QString img_path;        // 图片路径
    QString img_path_cropped;  // 透视变换后的图片路径

    std::string bd_access_token;

    /*
    * 将数据统一格式化为
    * {
    *   "[row]": {
    *     "[col]": {
    *       "row_span": 1,
    *       "col_span": 1,
    *       "text": "text",
    *       "polygon": [[x1, y1],[x2,y2],[x3,y3],[x4,y4]]
    *     }
    *   }
    * }
    * 方便直接根据行列坐标定位某单元格的信息
    */
    json ocr_result = json::object();
};
