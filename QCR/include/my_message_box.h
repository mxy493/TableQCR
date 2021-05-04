#ifndef MESSAGE_BOX_H
#define MESSAGE_BOX_H

#include <qmessagebox.h>
#include <qicon.h>

class MyMessageBox : public QMessageBox
{
public:
    MyMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text);
    MyMessageBox(const QString &text);
};

#endif  // MESSAGE_BOX_H
