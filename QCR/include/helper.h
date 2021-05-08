#pragma once

#include <QString>

#include <opencv2/imgproc/types_c.h>

#include <string>


/*
* @brief 获取当前时间
* 
* @return 返回QString类型的2021-03-11_11-49-35格式的当前时间
*/
QString getCurTimeString();

/*
* @brief 打印日志
* 
* @param log 要打印的log字符串
*/
void printLog(const QString &log);

void printLog(const std::string &log);

void printLog(const char *log);

/*
* @brief 将int64_t类型的unix时间戳转换为字符串
*/
std::string get_data(const int64_t &timestamp);

/*
* @brief 将本地图片转换为base64格式字符串
*/
void image2base64(const std::string &img_file, std::string &base64);

/*
* @brief 判断两个格子是否处于同一列
* 
* @param l1 单元格1的left边坐标
* @param r1 单元格1的right边坐标
* @param l2 单元格2的left边坐标
* @param r2 单元格2的right边坐标
* 
* @return 返回一个bool值, 返回true认为两个单元格相交, 但注意, 返回false不一定
*  不相交, 一般需要结合右侧单元格判断是否属于右边一列
*/
bool are_same_column(int l1, int r1, int l2, int r2);


/*
* @brief 传入图片路径, 识别图片中的表格并返回识别到的格点坐标
* 
* @param path 图片路径
* 
* @return 返回 std::vector<std::vector<cv::Point>> 格式的格点坐标矩阵
*/
std::vector<std::vector<cv::Point>> recognizeTable(const cv::String path);

/*
* @brief 鉴于本程序仅用于识别表格, 因此仅存在横线和竖线, 因此可以简单的认为
*  检测到的斜率要么为0, 要么无穷大, 即判断多条直线是否画面中同一条线, 可以简
*  化为判断两条横(竖)线的y(x)坐标间距是否小于某个阈值
* 
* @param lines 实际检测到的直线
* @param horizontal 横线还是竖线(本程序的应用场景中仅这两种情况)
* 
* @return vector<Vec3d> 类型的直线一般方程系数, 即方程 Ax + By + C = 0
*  中的{A, B, C}列表
*/
std::vector<cv::Vec3d> mergeLines(std::vector<cv::Vec4i> &lines, bool horizontal);

/*
* @brief 将点列表格式化为矩阵形式, 可能需要剔除无意义的点以及补全缺失的点
* 
* @param points_list 点集
* @param lines_h 横线一般方程系数列表
* @param lines_v 竖线一般方程系数列表
* 
* @return std::vector<std::vector<cv::Point>> 格式化后的点矩阵
*/
std::vector<std::vector<cv::Point>> formatPointMatrix(
    std::vector<cv::Point> points_list,
    const std::vector<cv::Vec3d> &lines_h,
    const std::vector<cv::Vec3d> &lines_v);

/*
* @brief 二维数组转置
* 
* @param vec 原二维数组
* @return 返回转置后的二维数组
*/
template <typename T>
std::vector<std::vector<T>> transpose(std::vector<std::vector<T>> &vec);

/*
* @brief 取除文字, 保留表格
*/
void removeText(const cv::Mat &src);