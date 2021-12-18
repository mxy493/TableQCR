#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QCloseEvent>

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
    printLog(QString::fromUtf8(u8"开始加载配置: %1").arg(path));
    QFile file(path);
    if (!file.exists())
    {
        printLog(QString::fromUtf8(u8"配置文件(%1)不存在!").arg(path));
        return;
    }
    try
    {
        config_table = toml::parse_file(CONFIG_FILE);
    }
    catch (const toml::parse_error& err)
    {
        printLog(QString::fromUtf8(u8"解析配置文件失败: %1").arg(err.description().data()));
        return;
    }
    // 更新界面值
    toml::table *tbl;
    int integer;
    bool bl;
    std::string str;
    QString qstr;
    if (config_table.contains(CFG_SECTION_NORMAL))
    {
        tbl = config_table.get(CFG_SECTION_NORMAL)->as_table();

        str = (*tbl)[CFG_NORMAL_SERVICE_PROVIDER].value_or("");
        qstr = QString::fromUtf8(str.c_str());
        int index = ui.combo_service_provider->findText(qstr);
        ui.combo_service_provider->setCurrentIndex(index);

        integer = (*tbl)[CFG_NORMAL_IMG_LENGTH].value_or(4000);
        ui.spin_img_length->setValue(integer);

        integer = (*tbl)[CFG_NORMAL_IMG_SIZE].value_or(4);
        ui.spin_img_size->setValue(integer);

        bl = (*tbl)[CFG_NORMAL_AUTO_EDGE_DETECTION].value_or(true);
        ui.check_auto_edge_detection->setChecked(bl);

        bl = (*tbl)[CFG_NORMAL_AUTO_OPTIMIZE].value_or(false);    
        ui.check_auto_optimize->setChecked(bl);
    }
    if (config_table.contains(CFG_SECTION_TX))
    {
        tbl = config_table.get(CFG_SECTION_TX)->as_table();

        str =  (*tbl)[CFG_TX_URL].value_or("");
        qstr = QString::fromUtf8(str.c_str());
        ui.line_tx_url->setText(qstr);

        str = (*tbl)[CFG_TX_SECRET_ID].value_or("");
        qstr = QString::fromUtf8(str.c_str());
        ui.line_tx_secret_id->setText(qstr);
        
        str = (*tbl)[CFG_TX_SECRET_KEY].value_or("");
        qstr = QString::fromUtf8(str.c_str());
        ui.line_tx_secret_key->setText(qstr);
    }
    if (config_table.contains(CFG_SECTION_BD))
    {
        tbl = config_table.get(CFG_SECTION_BD)->as_table();

        str = (*tbl)[CFG_BD_GET_TOKEN_URL].value_or("");
        qstr = QString::fromUtf8(str.c_str());
        ui.line_bd_get_token_url->setText(qstr);
        
        str = (*tbl)[CFG_BD_REQUEST_URL].value_or("");
        qstr = QString::fromUtf8(str.c_str());
        ui.line_bd_request_url->setText(qstr);
        
        str = (*tbl)[CFG_BD_GET_RESULT_URL].value_or("");
        qstr = QString::fromUtf8(str.c_str());
        ui.line_bd_get_result_url->setText(qstr);
        
        str = (*tbl)[CFG_BD_API_KEY].value_or("");
        qstr = QString::fromUtf8(str.c_str());
        ui.line_bd_api_key->setText(qstr);
        
        str = (*tbl)[CFG_BD_SECRET_KEY].value_or("");
        qstr = QString::fromUtf8(str.c_str());
        ui.line_bd_secret_key->setText(qstr);
    }
}

void ConfigDialog::saveConfig()
{
    QDir dir;
    if (!dir.exists("./data"))
    {
        dir.mkdir("data");
    }
    std::ofstream outfile;
    outfile.open("./data/config.toml", std::ios::out);
    outfile << config_table;
    outfile.close();
    printLog(QString::fromUtf8(u8"配置文件已更新"));
}

void ConfigDialog::updateConfig()
{
    QString str;
    int integer;
    bool bl;

    toml::table normal_table;
    str = ui.combo_service_provider->currentText();
    normal_table.insert_or_assign(CFG_NORMAL_SERVICE_PROVIDER, str.toUtf8().data());
    integer = ui.spin_img_length->value();
    normal_table.insert_or_assign(CFG_NORMAL_IMG_LENGTH, integer);
    integer = ui.spin_img_size->value();
    normal_table.insert_or_assign(CFG_NORMAL_IMG_SIZE, integer);
    bl = ui.check_auto_edge_detection->isChecked();
    normal_table.insert_or_assign(CFG_NORMAL_AUTO_EDGE_DETECTION, bl);
    bl = ui.check_auto_optimize->isChecked();
    normal_table.insert_or_assign(CFG_NORMAL_AUTO_OPTIMIZE, bl);
    config_table.insert_or_assign(CFG_SECTION_NORMAL, normal_table);

    toml::table bd_table;
    str = ui.line_bd_get_token_url->text();
    bd_table.insert_or_assign(CFG_BD_GET_TOKEN_URL, str.toUtf8().data());
    str = ui.line_bd_request_url->text();
    bd_table.insert_or_assign(CFG_BD_REQUEST_URL, str.toUtf8().data());
    str = ui.line_bd_get_result_url->text();
    bd_table.insert_or_assign(CFG_BD_GET_RESULT_URL, str.toUtf8().data());
    str = ui.line_bd_api_key->text();
    bd_table.insert_or_assign(CFG_BD_API_KEY, str.toUtf8().data());
    str = ui.line_bd_secret_key->text();
    bd_table.insert_or_assign(CFG_BD_SECRET_KEY, str.toUtf8().data());
    config_table.insert_or_assign(CFG_SECTION_BD, bd_table);

    toml::table tx_table;
    str = ui.line_tx_url->text();
    tx_table.insert_or_assign(CFG_TX_URL, str.toUtf8().data());
    str = ui.line_tx_secret_id->text();
    tx_table.insert_or_assign(CFG_TX_SECRET_ID, str.toUtf8().data());
    str = ui.line_tx_secret_key->text();
    tx_table.insert_or_assign(CFG_TX_SECRET_KEY, str.toUtf8().data());
    config_table.insert_or_assign(CFG_SECTION_TX, tx_table);

    saveConfig();
}

void ConfigDialog::setConfig(const char * const section, const char * const key, const char * const value)
{
    if (!config_table.contains(section))
    {
        toml::table tbl = toml::table{ {{key, value}} };
        config_table.insert_or_assign(section, tbl);
    }
    else
    {
        config_table[section].as_table()->insert_or_assign(key, value);
    }

    saveConfig();
}

void ConfigDialog::getConfig(const char * const section, const char * const key, char * value)
{
    std::string tmp = config_table[section][key].value_or("");
    memcpy(value, tmp.c_str(), tmp.length());
}

void ConfigDialog::accept()
{
    updateConfig();
    this->hide();
}

void ConfigDialog::reject()
{
    loadConfig();
    this->hide();
}
