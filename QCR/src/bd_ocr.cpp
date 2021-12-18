#include <curl/curl.h>

#include "include/bd_ocr.h"
#include "include/helper.h"

static size_t bdGetResponse(void *ptr, size_t sz, size_t nmemb, void *stream)
{
    std::string *result = static_cast<std::string *>(stream);
    *result += std::string((char *)ptr, sz * nmemb);
    return sz * nmemb;
}

int bdGetAccessToken(std::string &access_token, const std::string &access_token_url,
    const std::string &api_key, const std::string &secret_key)
{
    access_token.clear();
    CURL *curl;
    CURLcode result_code;
    int error_code = 0;
    curl = curl_easy_init();
    if (curl)
    {
        std::string url = access_token_url + "?grant_type=client_credentials"
            + "&client_id=" + api_key + "&client_secret=" + secret_key;
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); // 60s超时
        curl_easy_setopt(curl, CURLOPT_URL, url.data());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &access_token);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, bdGetResponse);
        result_code = curl_easy_perform(curl);
        if (result_code != CURLE_OK)
        {
            printLog(QString("[bd] curl_easy_perform() failed: %1")
                .arg(curl_easy_strerror(result_code)));
            return 1;
        }
        curl_easy_cleanup(curl);
        error_code = 0;
    }
    else
    {
        printLog("[bd] curl_easy_init() failed.");
        error_code = 1;
    }
    return error_code;
}

int bdFormOcrRequest(std::string &json_result, const std::string &request_url,
    const std::string &access_token, const std::string &base64_image)
{
    json_result.clear();
    std::string url = request_url + "?access_token=" + access_token;
    CURL *curl = NULL;
    CURLcode result_code;
    int is_success;
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); // 60s超时
        curl_easy_setopt(curl, CURLOPT_URL, url.data());
        // A parameter set to 1 tells libcurl to do a regular HTTP post.
        // This will also make the library use a
        // "Content-Type: application/x-www-form-urlencoded" header.
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_httppost *post = NULL;
        curl_httppost *last = NULL;
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "image",
            CURLFORM_COPYCONTENTS, base64_image.data(), CURLFORM_END);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
        // 设置回调用于写入收到的数据
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_result);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, bdGetResponse);
        result_code = curl_easy_perform(curl);
        if (result_code != CURLE_OK)
        {
            printLog(QString("[bd] curl_easy_perform() failed: %1")
                .arg(curl_easy_strerror(result_code)));
            is_success = 1;
            return is_success;
        }
        curl_easy_cleanup(curl);
        is_success = 0;
    }
    else
    {
        printLog("[bd] curl_easy_init() failed.");
        is_success = 1;
    }
    return is_success;
}

int bdGetResult(std::string &json_result, const std::string &request_url,
    const std::string &access_token, const std::string &request_id,
    const std::string &result_type)
{
    json_result.clear();
    std::string url = request_url + "?access_token=" + access_token;
    CURL *curl = NULL;
    CURLcode result_code;
    int is_success;
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30s超时
        curl_easy_setopt(curl, CURLOPT_URL, url.data());
        // A parameter set to 1 tells libcurl to do a regular HTTP post.
        // This will also make the library use a
        // "Content-Type: application/x-www-form-urlencoded" header.
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_httppost *post = NULL;
        curl_httppost *last = NULL;
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "request_id",
            CURLFORM_COPYCONTENTS, request_id.data(), CURLFORM_END);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "result_type",
            CURLFORM_COPYCONTENTS, result_type.data(), CURLFORM_END);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
        // 设置回调用于写入收到的数据
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_result);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, bdGetResponse);
        result_code = curl_easy_perform(curl);
        if (result_code != CURLE_OK)
        {
            printLog(QString("[bd] curl_easy_perform() failed: %1")
                .arg(curl_easy_strerror(result_code)));
            is_success = 1;
            return is_success;
        }
        curl_easy_cleanup(curl);
        is_success = 0;
    }
    else
    {
        printLog("[bd] curl_easy_init() failed.");
        is_success = 1;
    }
    return is_success;
}
