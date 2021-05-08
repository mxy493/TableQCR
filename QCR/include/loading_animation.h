/*
* 加载动画对话框类, 使用方法如下:
* LoadingAnimation animation;
* animation.start();
* // 执行的耗时操作, 期间可通过槽函数 updateText() 更新文字
* animation.stop();
*/

#ifndef LOADING_ANIMATION_H
#define LOADING_ANIMATION_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QColor>
#include <QLabel>
#include <QDialog>

class LoadingAnimation : public QDialog
{
    Q_OBJECT

public:
    explicit LoadingAnimation(QDialog *parent = nullptr);
    void paintEvent(QPaintEvent *event);
    void start();

    QTimer *timer;

public slots:
    void updateText(QString text);
    void stop();

private:
    QString text = QString::fromUtf8(u8"正在识别");
    int angle = 0;
    const int delta = 36;
    const int w = 120;  // 窗口宽度
    const int h = 200;  // 窗口高度
    bool clockwise = true;  // 顺时针方向
    QColor *color;  // 圆圈颜色
};

#endif // LOADING_ANIMATION_H
