#include <QFile>
#include <QDir>
#include <QTextStream>

#include "include/helper.h"
#include <include/config_dialog.h>
#include "include/my_message_box.h"

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent)
{
    ui.setupUi(this);
    setWindowFlags(Qt::WindowCloseButtonHint);

    this->setLayout(ui.vertical_layout);
    ui.gb_normal->setLayout(ui.grid_normal);
    ui.gb_tx->setLayout(ui.form_tx);
    ui.gb_bd->setLayout(ui.form_bd);
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::loadConfig()
{
    QString path = QString::fromLocal8Bit(CONFIG_FILE.c_str());
    QFile file(path);
    if (!file.exists())
    {
        printLog(QString::fromUtf8(u8"配置文件(%1)不存在!").arg(path));
        return;
    }
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        in.setCodec("UTF-8");
        while (true)
        {
            QString line = in.readLine();
            if (line.isEmpty())
                break;
            if (line.contains('='))
            {
                line.remove(' ');
                QStringList ls = line.split('=');
                // 第一个'='左侧为键, 右侧为值
                QString key = ls[0];
                QString value = ls[1];
                for (int i = 2; i < ls.size(); ++i)
                    value.append(QString::fromUtf8(u8"=") + ls[i]);

                // 更新界面值
                if (key == QString::fromUtf8(u8"service_provider"))
                {
                    int index = ui.combo_service_provider->findText(value);
                    ui.combo_service_provider->setCurrentIndex(index);
                }
                else if (key == QString::fromUtf8(u8"img_length"))
                    ui.spin_img_length->setValue(value.toInt());
                else if (key == QString::fromUtf8(u8"img_size"))
                    ui.spin_img_size->setValue(value.toInt());

                else if (key == QString::fromUtf8(u8"tx_url"))
                    ui.line_tx_url->setText(value);
                else if (key == QString::fromUtf8(u8"tx_secret_id"))
                    ui.line_tx_secret_id->setText(value);
                else if (key == QString::fromUtf8(u8"tx_secret_key"))
                    ui.line_tx_secret_key->setText(value);

                else if (key == QString::fromUtf8(u8"bd_get_token_url"))
                    ui.line_bd_get_token_url->setText(value);
                else if (key == QString::fromUtf8(u8"bd_request_url"))
                    ui.line_bd_request_url->setText(value);
                else if (key == QString::fromUtf8(u8"bd_get_result_url"))
                    ui.line_bd_get_result_url->setText(value);
                else if (key == QString::fromUtf8(u8"bd_api_key"))
                    ui.line_bd_api_key->setText(value);
                else if (key == QString::fromUtf8(u8"bd_secret_key"))
                    ui.line_bd_secret_key->setText(value);
            }
        }
        file.close();
        printLog(QString::fromUtf8(u8"配置已加载: ") + path);
    }
}

void ConfigDialog::updateConfig()
{
    QString service_provider = ui.combo_service_provider->currentText();
    QString img_length = QString::number(ui.spin_img_length->value());
    QString img_size = QString::number(ui.spin_img_size->value());

    QString bd_get_token_url = ui.line_bd_get_token_url->text();
    QString bd_request_url = ui.line_bd_request_url->text();
    QString bd_get_result_url = ui.line_bd_get_result_url->text();
    QString bd_api_key = ui.line_bd_api_key->text();
    QString bd_secret_key = ui.line_bd_secret_key->text();

    QString tx_url = ui.line_tx_url->text();
    QString tx_secret_id = ui.line_tx_secret_id->text();
    QString tx_secret_key = ui.line_tx_secret_key->text();

    // 写入配置文件
    QDir dir;
    if (!dir.exists("./data"))
    {
        dir.mkdir("data");
    }
    // 打开保存文件对话框
    QString path = QString::fromLocal8Bit(CONFIG_FILE.c_str());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream out(&file);
        out.setCodec("UTF-8");

        out << "[normal]\n";
        out << "service_provider=" << service_provider << "\n";
        out << "img_length=" << img_length << "\n";
        out << "img_size=" << img_size << "\n";

        out << "[bd]\n";
        out << "bd_get_token_url=" << bd_get_token_url << "\n";
        out << "bd_request_url=" << bd_request_url << "\n";
        out << "bd_get_result_url=" << bd_get_result_url << "\n";
        out << "bd_api_key=" << bd_api_key << "\n";
        out << "bd_secret_key=" << bd_secret_key << "\n";

        out << "[tx]\n";
        out << "tx_url=" << tx_url << "\n";
        out << "tx_secret_id=" << tx_secret_id << "\n";
        out << "tx_secret_key=" << tx_secret_key << "\n";

        out.flush();
        file.close();
        printLog(QString::fromUtf8(u8"配置文件已更新: ") + path);
    }

}

void ConfigDialog::accept()
{
    updateConfig();
    this->close();
}
