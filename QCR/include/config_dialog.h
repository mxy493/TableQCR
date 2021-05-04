#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H

#include <QDialog>
#include "ui_config_dialog.h"

const std::string CONFIG_FILE = "./data/config.cfg";


namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    Ui::ConfigDialog ui;

    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog();

    void loadConfig();
    void updateConfig();

public slots:
    void accept();

private:
};

#endif // CONFIG_DIALOG_H
