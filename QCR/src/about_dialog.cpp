#include "include/about_dialog.h"

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    setWindowFlags(Qt::WindowCloseButtonHint);
    connect(ui.btn_ok, SIGNAL(clicked()), this, SLOT(close()));
}

AboutDialog::~AboutDialog()
{
}
