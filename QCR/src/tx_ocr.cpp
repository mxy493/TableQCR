#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <stdio.h>
#include <time.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <curl/curl.h>

#include "include/helper.h"
#include "include/tx_ocr.h"

using namespace std;

string int2str(int64_t n)
{
    std::stringstream ss;
    ss << n;
    return ss.str();
}

string sha256Hex(const string &str)
{
    char buf[3];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::string NewString = "";
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        snprintf(buf, sizeof(buf), "%02x", hash[i]);
        NewString = NewString + buf;
    }
    return NewString;
}

string HmacSha256(const string &key, const string &input)
{
    unsigned char hash[32];

    HMAC_CTX *h;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX hmac;
    HMAC_CTX_init(&hmac);
    h = &hmac;
#else
    h = HMAC_CTX_new();
#endif

    HMAC_Init_ex(h, &key[0], key.length(), EVP_sha256(), NULL);
    HMAC_Update(h, ( unsigned char* )&input[0], input.length());
    unsigned int len = 32;
    HMAC_Final(h, hash, &len);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX_cleanup(h);
#else
    HMAC_CTX_free(h);
#endif

    std::stringstream ss;
    ss << std::setfill('0');
    for (int i = 0; i < len; i++)
    {
        ss  << hash[i];
    }

    return (ss.str());
}

string HexEncode(const string &input)
{
    static const char* const lut = "0123456789abcdef";
    size_t len = input.length();

    string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

std::string get_authorization(const std::string &secret_id,
    const std::string &secret_key, const int64_t &timestamp, const std::string &payload)
{
    string service = "ocr";
    string host = "ocr.tencentcloudapi.com";
    string region = "ap-beijing";
    string action = "RecognizeTableOCR";
    string version = "2018-11-19";
    string date = get_data(timestamp);

    // ************* 步骤 1：拼接规范请求串 *************
    string httpRequestMethod = "POST";
    string canonicalUri = "/";
    string canonicalQueryString = "";
    string canonicalHeaders = "content-type:application/json\nhost:" + host + "\n";
    string signedHeaders = "content-type;host";
    //string payload = "{\"Limit\": 1, \"Filters\": [{\"Values\": [\"\\u672a\\u547d\\u540d\"], \"Name\": \"instance-name\"}]}";
    string hashedRequestPayload = sha256Hex(payload);
    string canonicalRequest = httpRequestMethod + "\n" + canonicalUri + "\n"
        + canonicalQueryString + "\n" + canonicalHeaders + "\n"
        + signedHeaders + "\n" + hashedRequestPayload;
    cout << canonicalRequest << endl;
    cout << "-----------------------" << endl;

    // ************* 步骤 2：拼接待签名字符串 *************
    string algorithm = "TC3-HMAC-SHA256";
    string RequestTimestamp = int2str(timestamp);
    string credentialScope = date + "/" + service + "/" + "tc3_request";
    string hashedCanonicalRequest = sha256Hex(canonicalRequest);
    string stringToSign = algorithm + "\n" + RequestTimestamp + "\n" + credentialScope + "\n" + hashedCanonicalRequest;
    cout << stringToSign << endl;
    cout << "-----------------------" << endl;

    // ************* 步骤 3：计算签名 ***************
    string kKey = "TC3" + secret_key;
    string kDate = HmacSha256(kKey, date);
    string kService = HmacSha256(kDate, service);
    string kSigning = HmacSha256(kService, "tc3_request");
    string signature = HexEncode(HmacSha256(kSigning, stringToSign));
    cout << signature << endl;
    cout << "-----------------------" << endl;

    // ************* 步骤 4：拼接 Authorization *************
    string authorization = algorithm + " " + "Credential=" + secret_id + "/" + credentialScope + ", "
            + "SignedHeaders=" + signedHeaders + ", " + "Signature=" + signature;
    cout << authorization << endl;
    cout << "------------------------" << endl;

    return authorization;
};

static size_t txGetResponse(void *ptr, size_t sz, size_t nmemb, void *stream)
{
    std::string *result = static_cast<std::string *>(stream);
    *result += std::string((char *)ptr, sz * nmemb);
    return sz * nmemb;
}

int txFormOcrRequest(std::string &result, const std::string &request_url,
    const std::string &secret_id, const std::string &secret_key,
    const std::string &base64_image)
{
    result.clear();

    std::string bd_data = "{\"ImageBase64\":\"" + base64_image + "\"}";
    int64_t timestamp = std::time(nullptr);
    std::string authorization = get_authorization(secret_id, secret_key, timestamp, bd_data);

    std::string hd_timestamp = "X-TC-Timestamp:" + int2str(timestamp);
    std::string hd_authorization = "Authorization:" + authorization;

    CURL *curl = NULL;
    CURLcode result_code;
    int is_success;
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, request_url.data());
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        // 添加表头信息
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type:application/json");
        headers = curl_slist_append(headers, "X-TC-Action:RecognizeTableOCR");
        headers = curl_slist_append(headers, "X-TC-Region:ap-beijing");
        headers = curl_slist_append(headers, hd_timestamp.data());
        headers = curl_slist_append(headers, "X-TC-Version:2018-11-19");
        headers = curl_slist_append(headers, hd_authorization.data());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 添加表体数据
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bd_data.c_str());

        // 设置回调用于写入收到的数据
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, txGetResponse);

        // 打印详细的调试信息
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        result_code = curl_easy_perform(curl);
        if (result_code != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(result_code));
            is_success = 1;
            return is_success;
        }
        curl_easy_cleanup(curl);
        is_success = 0;
    }
    else
    {
        fprintf(stderr, "curl_easy_init() failed.");
        is_success = 1;
    }
    return is_success;
}
