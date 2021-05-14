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
    QPoint p = event->pos();
    inVertex(p);
    if (vertex_index != -1)
    {
        intercept_abs[vertex_index] = p;
        abs2rel();
        update();
    }
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    updatePos(event->pos());
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    updatePos(event->pos());
    vertex_index = -1;
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

void ImageWidget::updatePos(const QPoint &p)
{
    if (vertex_index != -1)
    {
        if (isOk(p))
        {
            intercept_abs[vertex_index] = p;
            abs2rel();
            update();
        }
        //else
        //    intercept_abs[vertex_index] = getPedal(p);
    }
}

bool ImageWidget::isOk(const QPoint &p)
{
    // 超出图片显示范围
    if (p.x() < h_margin || p.y() < v_margin ||
        p.x() > h_margin + scaled_pix.width() ||
        p.y() > v_margin + scaled_pix.height())
    {
        return false;
    }

    // 四条边向量
    int v01[2] = { intercept_abs[1].x() - intercept_abs[0].x(),
        intercept_abs[1].y() - intercept_abs[0].y() };
    int v12[2] = { intercept_abs[2].x() - intercept_abs[1].x(),
        intercept_abs[2].y() - intercept_abs[1].y() };
    int v23[2] = { intercept_abs[3].x() - intercept_abs[2].x(),
        intercept_abs[3].y() - intercept_abs[2].y() };
    int v30[2] = { intercept_abs[0].x() - intercept_abs[3].x(),
        intercept_abs[0].y() - intercept_abs[3].y() };
    // 两对角线向量
    int v02[2] = { intercept_abs[2].x() - intercept_abs[0].x(),
        intercept_abs[2].y() - intercept_abs[0].y() };
    int v13[2] = { intercept_abs[3].x() - intercept_abs[1].x(),
        intercept_abs[3].y() - intercept_abs[1].y() };

    bool ok = false;
    switch (vertex_index)
    {
    case 0:
    {
        if (v01[0] * v12[1] - v01[1] * v12[0] >= 0 &&
            v02[0] * v23[1] - v02[1] * v23[0] >= 0 &&
            v01[0] * v13[1] - v01[1] * v13[0] >= 0)
            ok = true;
        break;
    }
    case 1:
    {
        if (v12[0] * v23[1] - v12[1] * v23[0] >= 0 &&
            v13[0] * v30[1] - v13[1] * v30[0] >= 0 &&
            v12[0] * v02[1] - v12[1] * v02[0] <= 0)
            ok = true;
        break;
    }
    case 2:
    {
        if (v23[0] * v30[1] - v23[1] * v30[0] >= 0 &&
            v02[0] * v01[1] - v02[1] * v01[0] <= 0 &&
            v23[0] * v13[1] - v23[1] * v13[0] <= 0)
            ok = true;
        break;
    }
    case 3:
    {
        if (v30[0] * v01[1] - v30[1] * v01[0] >= 0 &&
            v13[0] * v12[1] - v13[1] * v12[0] <= 0 &&
            v30[0] * v02[1] - v30[1] * v02[0] >= 0)
            ok = true;
        break;
    }
    default:
        break;
    }
    return ok;
}

QPoint ImageWidget::getNeareastPoint(const QPoint &p)
{
    QPoint A;
    QPoint B;
    switch (vertex_index)
    {
    case 0:
    {
        A = intercept_abs[1];
        B = intercept_abs[3];
        break;
    }
    case 1:
    {
        A = intercept_abs[2];
        B = intercept_abs[0];
        break;
    }
    case 2:
    {
        A = intercept_abs[3];
        B = intercept_abs[1];
        break;
    }
    case 3:
    {
        A = intercept_abs[0];
        B = intercept_abs[2];
        break;
    }
    default:
        break;
    }
    double k1 = static_cast<double>(B.y() - A.y()) / (B.x() - A.x());
    double k2 = -1 / k1;

    // 直线AB的方程
    double A1 = B.y() - A.y();
    double B1 = A.x() - B.x();
    double C1 = B.x() * A.y() - A.x() * B.y();

    // 垂线的方程
    double A2 = k2;
    double B2 = -1;
    double C2 = -k2 * p.x() + p.y();

    // 两直线的交点, 即垂点
    int x = (B1 * C2 - B2 * C1) / (B2 * A1 - B1 * A2);
    int y = (A1 * C2 - C1 * A2) / (B1 * A2 - A1 * B2);

    double dab = std::sqrt(std::pow(B.x() - A.x(), 2) + std::pow(B.y() - A.y(), 2));
    double da = std::sqrt(std::pow(x - A.x(), 2) + std::pow(y - A.y(), 2));
    double db = std::sqrt(std::pow(x - B.x(), 2) + std::pow(y - B.y(), 2));

    // 加一点可接受的误差, 否则极有可能由于误差导致始终认为垂点没在线段上
    if (da + db > dab + 2)
    {
        if (da < db)
            return A;
        else
            return B;
    }
    else
    {
        return QPoint(x, y);
    }
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
        intercept_rel.clear();
        intercept_rel.append(QPointF(0, 0));
        intercept_rel.append(QPointF(1, 0));
        intercept_rel.append(QPointF(1, 1));
        intercept_rel.append(QPointF(0, 1));
    }
    // 必须转换, 否则可能界面大小已经改变而绝对坐标还是之前的坐标
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
