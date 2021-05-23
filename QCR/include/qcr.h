﻿#pragma once

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
    /*
    * @brief 合并霍夫变换检测的线段并计算四个交点坐标
    * @param lines 检测到的线段
    * @param points 四个交点坐标, 分别是[left-up, right-up, right-bottom, left-bottom]
    */
    void getVertexes(std::vector<cv::Vec4i> &lines, std::vector<cv::Point> &points);
    // 边缘检测主要是拿到4个顶点的相对坐标, 可以将图片缩小以加快处理速度
    void edgeDetection();
    void runTxOcr(const std::string &base64_img);
    void runBdOcr(const std::string &base64_img);
    void updateTableCell(int row, int col, int row_span, int col_span, const QString &text);
    void updateTable();
    void resetTable();
    void txParseData(const std::string &str);
    void bdParseData(const std::string &str);
    // 根据某一列的文本内容判断其是否是分数列, 返回所有分数列的像素范围及其对应的列数[col, left, right, top, bottom]
    void getScoreColumn(std::vector<std::vector<int>> &rects);
    // 按获取到的分数列像素范围切割图片并以"col_left_right_top_bottom.jpg"格式保存图片
    void cropScoreColumn(const std::vector<std::vector<int>> &rects);
    // 获取提取到的每个字符的坐标及其识别结果, 相对于切割前的图片[col, left, right, top, bottom, num]
    void extractWords(std::vector<std::vector<std::vector<int>>> &words);
    /*
    * @brief 拼接识别出的同一行的多个数字
    */
    void combine(std::vector<std::vector<int>> &words, std::vector<int> &word);
    // 将同一行识别到的多个数字拼接在一起
    void spliceWords(std::vector<std::vector<std::vector<int>>> &words);
    // 融合OCR和数字识别的结果
    void fusion(std::vector<std::vector<std::vector<int>>> &words);
    void closeEvent(QCloseEvent *event);

signals:
    void msg_signal(QString msg);

public slots:
    void msg_box(QString msg);

    void openImage();
    void rotateImage();
    void interceptImage();
    void restore();
    void runOcr();
    void optimize();
    void exportTableData();

    void drawSelectedCell(int row, int col);

private:
    Ui::QCRClass ui;

    QThread *initial_thread;
    QThread *detect_table_thread;

    ConfigDialog config_dialog;  // 设置对话框
    AboutDialog about_dlg;       // 关于对话框
    LoadingAnimation animation;  // 加载动画

    QAction *act_open;    // 打开按钮
    QAction *act_rotate;  // 旋转按钮
    QAction *act_crop;    // 裁剪按钮
    QAction *act_restore; // 恢复按钮
    QAction *act_ocr;     // 识别按钮
    QAction *act_optimize;// 识别按钮
    QAction *act_export;  // 导出按钮
    QAction *act_config;  // 设置按钮
    QAction *act_about;   // 关于按钮

    QString img_path;           // 图片路径
    QString img_path_cropped;   // 透视变换后的图片路径

    bool ocr_success; // 执行OCR识别是否成功
    std::string bd_access_token; // 百度Access Token

    /*
    * 将数据统一格式化为:
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
