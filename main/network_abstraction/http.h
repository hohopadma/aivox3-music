#pragma once
#include <string>
#include <functional>
#include <memory>
#include <esp_http_client.h>

class Http {
public:
    Http();
    ~Http();

    void SetHeader(const std::string& name, const std::string& value);
    void SetContent(std::string data);
    bool Open(const std::string& method, const std::string& url);
    int GetStatusCode();
    std::string ReadAll();
    int64_t GetBodyLength();
    int Read(char* buffer, size_t buf_size);
    void Close();

private:
    esp_http_client_handle_t client_ = nullptr;
    std::string post_data_;
    std::string response_data_;
    int status_code_ = 0;
};
