#include "include/my_message_box.h"

MyMessageBox::MyMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text)
{
    this->setWindowIcon(QIcon(":/images/logo.png"));
    this->setIcon(icon);
    this->setWindowTitle(title);
    this->setText(text);
}

MyMessageBox::MyMessageBox(const QString &text)
{
    this->setWindowIcon(QIcon(":/images/logo.png"));
    this->setIcon(QMessageBox::Warning);
    this->setWindowTitle(tr(u8"注意"));
    this->setText(text);
}
