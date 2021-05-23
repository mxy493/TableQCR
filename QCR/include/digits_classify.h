#include <opencv2/core/mat.hpp>

/*
* @brief 加载模型
*/
void loadModel(const std::string &file_name);

/*
* @brief 将cv::Mat的图片处理为标准的28×28像素
*/
void stdProcImg(cv::Mat &img);

/*
* @brief 识别传入的图像并返回识别出的数字
*/
int predict(const cv::Mat &src);

