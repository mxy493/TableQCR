#include <iostream>

#include <QFile>
#include <fdeep/fdeep.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "../include/digits_classify.h"
#include "../include/helper.h"

std::unique_ptr<fdeep::model> model;

// Image size in MNIST database
const int width = 28;
const int height = 28;

void loadModel(const std::string &file_name)
{
    printLog(QString::fromUtf8(u8"加载数字识别模型: %1").arg(file_name.c_str()));
    if (QFile(file_name.c_str()).exists())
    {
        model = std::make_unique<fdeep::model>(fdeep::load_model(file_name));
    }
    else
    {
        printLog(QString::fromUtf8(u8"模型文件不存在: %1").arg(file_name.c_str()));
    }
}

void stdProcImg(cv::Mat &img)
{
    if (img.channels() == 3)
        cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
    // 压缩到24×24, 然后添加两三个像素的边框
    double scale = std::min(static_cast<double>(width - 4) / img.cols,
        static_cast<double>(height - 4) / img.rows);
    cv::resize(img, img, cv::Size(), scale, scale);
    int top = (height - img.rows) / 2;
    int bottom = height - img.rows - top;
    int left = (width - img.cols) / 2;
    int right = width - img.cols - left;
    cv::copyMakeBorder(img, img, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0));
}

int predict(const cv::Mat &src)
{
    cv::Mat img = src.clone();
    stdProcImg(img);
    if (!img.isContinuous())
        img = img.clone();

    const auto input = fdeep::tensor_from_bytes(img.ptr(), 28, 28, 1, 0.0f, 1.0f);
    const auto result = model->predict({ input });
    //std::cout << fdeep::show_tensors(result) << std::endl;

    const std::vector<float> vec = result.front().to_vector();
    auto it = std::max_element(vec.begin(), vec.end());
    int predict = std::distance(vec.begin(), it);

    // 保存图片并以识别结果命名用于查看识别效果
    //static int index = 1;
    //std::string file = "classify/" + std::to_string(predict) + "_" + std::to_string(index) + ".jpg";
    //cv::imwrite(file, img);
    //++index;

    return predict;
}
