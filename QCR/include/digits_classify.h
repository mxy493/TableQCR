#include <opencv2/core/mat.hpp>

/*
* @brief 加载模型
* @param file_name 数字分类模型文件名"xxx.json"
*/
void loadModel(const std::string &file_name);

/*
* @brief 将cv::Mat的图片处理为标准的28×28像素
* @param img 将img缩放至最长边28像素, 再补全边框使其成为标准的28×28的灰度图
*/
void stdProcImg(cv::Mat &img);

/*
* @brief 识别传入的图像并返回识别出的数字
* @param 图片无限制, 函数内部将自动使图片标准化后用于识别
* @return 返回识别到的数字
*/
int predict(const cv::Mat &src);

