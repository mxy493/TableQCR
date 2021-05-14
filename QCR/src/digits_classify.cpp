// Software: Testing Artificial Neural Network for MNIST database
// Author: Hy Truong Son
// Major: BSc. Computer Science
// Class: 2013 - 2016
// Institution: Eotvos Lorand University
// Email: sonpascal93@gmail.com
// Website: http://people.inf.elte.hu/hytruongson/
// Copyright 2015 (c). All rights reserved.
// 

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <set>
#include <iterator>
#include <algorithm>

#include "include/digits_classify.h"
#include "include/helper.h"


// Weights file name
const std::string model_fn = "digits.weights";

// Image size in MNIST database
const int width = 28;
const int height = 28;

// n1 = Number of input neurons
// n2 = Number of hidden neurons
// n3 = Number of output neurons

const int n1 = width * height; // = 784, without bias neuron 
const int n2 = 128;
const int n3 = 10; // Ten classes: 0 - 9

// From layer 1 to layer 2. Or: Input layer - Hidden layer
double *w1[n1 + 1], *out1;

// From layer 2 to layer 3. Or; Hidden layer - Output layer
double *w2[n2 + 1], *in2, *out2;

// Layer 3 - Output layer
double *in3, *out3;

// Image. In MNIST: 28x28 gray scale images.
int d[width + 1][height + 1];

// +-----------------------------------+
// | Memory allocation for the network |
// +-----------------------------------+

void init_array()
{
    // Layer 1 - Layer 2 = Input layer - Hidden layer
    for (int i = 1; i <= n1; ++i)
    {
        w1[i] = new double[n2 + 1];
    }

    out1 = new double[n1 + 1];

    // Layer 2 - Layer 3 = Hidden layer - Output layer
    for (int i = 1; i <= n2; ++i)
    {
        w2[i] = new double[n3 + 1];
    }

    in2 = new double[n2 + 1];
    out2 = new double[n2 + 1];

    // Layer 3 - Output layer
    in3 = new double[n3 + 1];
    out3 = new double[n3 + 1];
}

void init(const std::string &model)
{
    init_array();
    load_model(model);
}

// +----------------------------------------+
// | Load model of a trained Neural Network |
// +----------------------------------------+

void load_model(const std::string &file_name)
{
    std::ifstream file(file_name.c_str(), std::ios::in);

    // Input layer - Hidden layer
    for (int i = 1; i <= n1; ++i)
    {
        for (int j = 1; j <= n2; ++j)
        {
            file >> w1[i][j];
        }
    }

    // Hidden layer - Output layer
    for (int i = 1; i <= n2; ++i)
    {
        for (int j = 1; j <= n3; ++j)
        {
            file >> w2[i][j];
        }
    }

    file.close();
}

// +------------------+
// | Sigmoid function |
// +------------------+

double sigmoid(double x)
{
    return 1.0 / (1.0 + exp(-x));
}

// +------------------------------+
// | Forward process - Perceptron |
// +------------------------------+

void perceptron()
{
    for (int i = 1; i <= n2; ++i)
    {
        in2[i] = 0.0;
    }

    for (int i = 1; i <= n3; ++i)
    {
        in3[i] = 0.0;
    }

    for (int i = 1; i <= n1; ++i)
    {
        for (int j = 1; j <= n2; ++j)
        {
            in2[j] += out1[i] * w1[i][j];
        }
    }

    for (int i = 1; i <= n2; ++i)
    {
        out2[i] = sigmoid(in2[i]);
    }

    for (int i = 1; i <= n2; ++i)
    {
        for (int j = 1; j <= n3; ++j)
        {
            in3[j] += out2[i] * w2[i][j];
        }
    }

    for (int i = 1; i <= n3; ++i)
    {
        out3[i] = sigmoid(in3[i]);
    }
}

void stdProcImg(cv::Mat &img)
{
    if (img.channels() == 3)
        cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
    // 压缩到24×24, 然后添加两三个像素的边框
    double scale = std::min(static_cast<double>(width - 4) / img.cols, static_cast<double>(height - 4) / img.rows);
    cv::resize(img, img, cv::Size(), scale, scale);
    int top = (height - img.rows) / 2;
    int bottom = height - img.rows - top;
    int left = (width - img.cols) / 2;
    int right = width - img.cols - left;
    printLog("top: " + std::to_string(top) + ", bottom: " + std::to_string(bottom)
        + ", left: " + std::to_string(left) + ", right: " + std::to_string(right));
    cv::copyMakeBorder(img, img, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0));
}

void transform(const cv::Mat &img)
{
    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            d[i + 1][j + 1] = static_cast<int>(img.at<uchar>(j, i)) / 128;
        }
    }

    for (int j = 1; j <= height; ++j)
    {
        for (int i = 1; i <= width; ++i)
        {
            int pos = i + (j - 1) * width;
            out1[pos] = d[i][j];
        }
    }
}

int predict(const cv::Mat &src)
{
    cv::Mat img = src.clone();
    stdProcImg(img);
    transform(img);

    // Classification - Perceptron procedure
    perceptron();

    // Prediction
    int predict = 1;
    for (int i = 2; i <= n3; ++i)
    {
        if (out3[i] > out3[predict])
        {
            predict = i;
        }
    }
    --predict;

    std::cout << "Predict = " << predict << std::endl;
    static int index = 1;
    std::string file = "classify/" + std::to_string(predict) + "_" + std::to_string(index) + ".jpg";
    cv::imwrite(file, img);
    ++index;
    //std::cout << "Image:" << std::endl;
    //for (int j = 1; j <= height; ++j)
    //{
    //    for (int i = 1; i <= width; ++i)
    //    {
    //        std::cout << d[i][j];
    //    }
    //    std::cout << std::endl;
    //}
    //std::cout << std::endl;

    return predict;
}
