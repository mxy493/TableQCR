#include "include/qcr.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCR w;
    w.show();
    return a.exec();
}
