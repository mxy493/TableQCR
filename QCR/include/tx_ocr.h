#include <string>

/*
* @brief 获取腾讯Authorization
*/
std::string get_authorization(const std::string &secret_id,
    const std::string &secret_key, const int64_t &timestamp, const std::string &payload);

/*
* @brief 回调函数, 获取返回的数据
*/
static size_t txGetResponse(void *ptr, size_t size, size_t nmemb, void *stream);

/*
* @brief 识别带表格的图片
*/
int txFormOcrRequest(std::string &json_result, const std::string &request_url,
    const std::string &secret_id, const std::string &secret_key,
    const std::string &base64_image);
