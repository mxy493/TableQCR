#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H

#include <QDialog>
#include <toml++/toml.h>

#include "ui_config_dialog.h"

const std::string CONFIG_FILE = "./data/config.toml";

const std::string CFG_SECTION_NORMAL = "normal";
const std::string CFG_NORMAL_SERVICE_PROVIDER = "service_provider";
const std::string CFG_NORMAL_IMG_LENGTH = "img_length";
const std::string CFG_NORMAL_IMG_SIZE = "img_size";
const std::string CFG_NORMAL_AUTO_EDGE_DETECTION = "auto_edge_detection";
const std::string CFG_NORMAL_AUTO_OPTIMIZE = "auto_optimize";

const std::string CFG_SECTION_TX = "tx";
const std::string CFG_TX_URL = "url";
const std::string CFG_TX_SECRET_ID = "secret_id";
const std::string CFG_TX_SECRET_KEY = "secret_key";

const std::string CFG_SECTION_BD = "bd";
const std::string CFG_BD_GET_TOKEN_URL = "get_token_url";
const std::string CFG_BD_REQUEST_URL = "request_url";
const std::string CFG_BD_GET_RESULT_URL = "get_result_url";
const std::string CFG_BD_API_KEY = "api_key";
const std::string CFG_BD_SECRET_KEY = "secret_key";

const std::string CFG_SECTION_OTHERS = "others";
const std::string CFG_OTHERS_OPEN_IMG_PATH = "open_img_path";

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
    void saveConfig();
    void updateConfig();
    void setConfig(const char * const section, const char * const key, const char * const value);
    void getConfig(const char * const section, const char * const key, const char *value);

public slots:
    void accept();
    void reject();

private:
    toml::table config_table;
};

#endif // CONFIG_DIALOG_H
