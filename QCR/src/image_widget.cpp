#include "include/image_widget.h"

#include <QPainter>

ImageWidget::ImageWidget(QWidget *parent) : QFrame(parent)
{
}

ImageWidget::~ImageWidget()
{
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    inVertex(event->pos());
    if (vertex_index != -1)
    {
        intercept_abs[vertex_index] = event->pos();
        abs2rel();
        update();
    }
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint p = event->pos();
    if (vertex_index != -1)
    {
        if(isOk(p))
            intercept_abs[vertex_index] = p;
        abs2rel();
        update();
    }
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QPoint p = event->pos();
    if (vertex_index != -1)
    {
        if (isOk(p))
            intercept_abs[vertex_index] = p;
        update();
        abs2rel();
        vertex_index = -1;
    }
}

void ImageWidget::paintEvent(QPaintEvent *event)
{
    this->setStyleSheet("border:1px solid grey");  // 边框样式
    QPainter painter(this);
    if (!src_pix.isNull())
    {
        drawImage(painter);
        drawInterceptBox(painter);
    }
}

void ImageWidget::setPix(QPixmap pix)
{
    initial = true;
    this->src_pix = pix;
    this->update();
}

void ImageWidget::inVertex(const QPoint &pos)
{
    for (int i = 0; i < intercept_abs.size(); ++i)
    {
        QPoint box = intercept_abs[i];
        if (std::abs(pos.x() - box.x()) < CIRCLE_SIZE &&
            std::abs(pos.y() - box.y()) < CIRCLE_SIZE)
        {
            vertex_index = i;
            break;
        }
    }
}

void ImageWidget::getVertex(std::vector<std::vector<double>> &points)
{
    abs2rel();
    for (const auto &p : intercept_rel)
    {
        points.push_back({ p.x(), p.y() });
    }
}

void ImageWidget::abs2rel()
{
    intercept_rel.clear();
    for (const auto &p : intercept_abs)
    {
        double _x = static_cast<double>(p.x() - h_margin) / scaled_pix.width();
        double _y = static_cast<double>(p.y() - v_margin) / scaled_pix.height();
        intercept_rel.append(QPointF(_x, _y));
    }
}

void ImageWidget::rel2abs()
{
    intercept_abs.clear();
    for (const auto &p : intercept_rel)
    {
        int _x = h_margin + p.x() * scaled_pix.width();
        int _y = v_margin + p.y() * scaled_pix.height();
        intercept_abs.append(QPoint(_x, _y));
    }
}

bool ImageWidget::isOk(const QPoint &p)
{
    if (p.x() < h_margin || p.y() < v_margin
        || p.x() > h_margin + scaled_pix.width()
        || p.y() > v_margin + scaled_pix.height())
    {
        return false;
    }
    QPoint A;
    QPoint B;
    switch (vertex_index)
    {
    case 0:
    {
        A = intercept_abs[3];
        B = intercept_abs[1];
        break;
    }
    case 1:
    {
        A = intercept_abs[0];
        B = intercept_abs[2];
        break;
    }
    case 2:
    {
        A = intercept_abs[1];
        B = intercept_abs[3];
        break;
    }
    case 3:
    {
        A = intercept_abs[2];
        B = intercept_abs[0];
        break;
    }
    default:
        break;
    }
    int a[2] = { B.x() - A.x(), B.y() - A.y() };
    int b[2] = { p.x() - A.x(), p.y() - A.y() };

    if (a[0] * b[1] - a[1] * b[0] <= 0)
        return true;
    else
        return false;
}

void ImageWidget::drawImage(QPainter &painter)
{
    // 平滑缩放图片
    scaled_pix = src_pix.scaled(this->width() - 2, this->height() - 2,
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    img_scale = static_cast<double>(scaled_pix.height())
        / static_cast<double>(src_pix.height());
    h_margin = scaled_pix.width() < this->width() ?
        (this->width() - scaled_pix.width()) / 2 : 1;
    v_margin = scaled_pix.height() < this->height() ?
        (this->height() - scaled_pix.height()) / 2 : 1;
    painter.drawPixmap(h_margin, v_margin, scaled_pix);
}

void ImageWidget::setInterceptBox(const std::vector<std::vector<double>> &points)
{
    initial = false;
    intercept_rel.clear();
    for (const auto &p : points)
        intercept_rel.append(QPointF(p[0], p[1]));
    rel2abs();
    update();
}

void ImageWidget::drawInterceptBox(QPainter &painter)
{
    if (initial)
    {
        initial = false;
        intercept_abs.clear();
        intercept_abs.append(QPoint(h_margin, v_margin));
        intercept_abs.append(QPoint(h_margin + scaled_pix.width(), v_margin));
        intercept_abs.append(QPoint(
            h_margin + scaled_pix.width(), v_margin + scaled_pix.height()));
        intercept_abs.append(QPoint(h_margin, v_margin + scaled_pix.height()));
        abs2rel();
    }
    else
        rel2abs();

    // 画圈和连接线
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setPen(QPen(QBrush(Qt::red), 3));
    for (int i = 0; i < intercept_abs.size(); ++i)
    {
        painter.drawEllipse(intercept_abs[i], CIRCLE_SIZE, CIRCLE_SIZE);
        painter.drawLine(intercept_abs[i], intercept_abs[(i + 1) % 4]);
    }
}
