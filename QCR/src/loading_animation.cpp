#include <include/loading_animation.h>

#include <QApplication>

LoadingAnimation::LoadingAnimation(QDialog *parent) : QDialog(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::BypassWindowManagerHint
        | Qt::Tool | Qt::WindowStaysOnTopHint);  // 置顶
    setFixedSize(w, h);

    color = new QColor(24, 189, 155);
    timer = new  QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));  // 定时刷新界面
}

void LoadingAnimation::start()
{
    timer->start(100);
    exec();
}

void LoadingAnimation::stop()
{
    timer->stop();
    this->close();
}

void LoadingAnimation::updateText(QString text)
{
    this->text = text;
}

void LoadingAnimation::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // 画文字
    painter.setPen(QColor(Qt::darkGray));
    painter.setFont(QFont(QString::fromUtf8(u8"微软雅黑"), 12, 400));
    painter.drawText(QRect(0, w, w, h - w), Qt::AlignCenter, this->text);

    // 画图形
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(w / 2, w / 2);
    painter.scale(w / 100.0, w / 100.0);
    painter.rotate(this->angle);
    painter.save();
    painter.setPen(Qt::NoPen);
    color->toRgb();
    for (int i = 0; i < 11; i++)
    {
        color->setAlphaF(1.0 * i / 10);
        painter.setBrush(QBrush(*color));
        painter.drawEllipse(30, -10, 20, 20);
        painter.rotate(36);
    }
    painter.restore();
    this->angle += this->clockwise ? this->delta : -this->delta;
    this->angle %= 360;
}
