#include "include/qcr.h"
#include <QtWidgets/QApplication>

#include "../include/helper.h"


int main(int argc, char *argv[])
{
    initSpdLogger();

    QApplication a(argc, argv);
    QCR w;
    w.show();
    return a.exec();
}
