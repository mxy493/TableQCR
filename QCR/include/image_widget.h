#ifndef IMAGE_WIDGET_H
#define IMAGE_WIDGET_H

#include <QFrame>
#include <QMouseEvent>
#include <include/helper.h>

constexpr int CIRCLE_SIZE = 10;

class ImageWidget : public QFrame
{
    Q_OBJECT

public:
    ImageWidget(QWidget *parent = nullptr);
    ~ImageWidget();

    void setPix(QPixmap pix);
    QPixmap getPix();   // 获取src_pix
    void rotateImage();
    void inVertex(const QPoint &pos);
    void getVertex(std::vector<std::vector<double>> &points);
    void abs2rel();
    void rel2abs();
    void updatePos(const QPoint &p);
    bool isOk(const QPoint &p);
    // 获取垂足坐标
    QPoint getNeareastPoint(const QPoint &p);
    void drawImage(QPainter &painter);
    void setInterceptBox(const std::vector<std::vector<double>> &points);
    void drawInterceptBox(QPainter &painter);
    void setSelectedRect(const std::vector<std::vector<double>> &points);
    void clearSelectedRect();
    void drawSelectedRect(QPainter &painter);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    QPixmap src_pix;             // 原始图片
    QPixmap scaled_pix;          // 缩放后的图片
    double img_scale = 1.0;      // scaled/src
    QVector<QPoint> intercept_abs;   // 以 widget 为参照的左上、右上、右下、左下
    QVector<QPointF> intercept_rel;  // 以图片为参照的相对坐标
    QVector<QPointF> selected_rect;  // 选中的格子的相对坐标
    bool initial = true;
    int vertex_index = -1;
    int h_margin = 0;
    int v_margin = 0;
};

#endif  // IMAGE_WIDGET_H
