#include <string>

/**
 * 用以获取access_token的函数，使用时需要先在百度云控制台申请相应功能的应用，获得对应的API Key和Secret Key
 * @param access_token 获取得到的access token，调用函数时需传入该参数
 * @param api_key 应用的API key
 * @param secret_key 应用的Secret key
 * @return 返回0代表获取access token成功，其他返回值代表获取失败
 */
int bdGetAccessToken(std::string &access_token,
    const std::string &access_token_url,
    const std::string &api_key, const std::string &secret_key);

/**
* curl发送http请求调用的回调函数，回调函数中对返回的json格式的body进行了解析，解析结果储存在全局的静态变量当中
* @param 参数定义见libcurl文档
* @return 返回值定义见libcurl文档
*/
static size_t bdGetResponse(void *ptr, size_t size, size_t nmemb, void *stream);

/**
* 表格文字识别(异步接口)
* @return 调用成功返回0，发生错误返回其他错误码
*/
int bdFormOcrRequest(std::string &json_result,
    const std::string &request_url,
    const std::string &base64_image, const std::string &access_token);

/*
* @brief 获取表格识别结果
* @param json_result 获取到的返回值, json 格式字符串
* @return 成功返回0，否则返回其他错误码
*/
int bdGetResult(std::string &json_result, const std::string &request_url,
    const std::string &access_token, const std::string &request_id,
    const std::string &result_type);
