#pragma once
#include <string>
#include <functional>
#include <memory>
#include <lwip/sockets.h>
#include <mbedtls/ssl.h>

class WebSocket {
public:
    WebSocket();
    ~WebSocket();

    bool IsConnected();
    bool Send(const std::string& text);
    bool Send(const uint8_t* data, size_t len, bool binary);
    void SetHeader(const std::string& name, const std::string& value) { /* stored, applied on connect */ }
    void OnData(std::function<void(const char* data, size_t len, bool binary)> callback);
    void OnDisconnected(std::function<void()> callback);
    void OnError(std::function<void()> callback);
    bool Connect(const std::string& url);

private:
    int sock_ = -1;
    bool connected_ = false;
    std::function<void(const char*, size_t, bool)> on_data_;
    std::function<void()> on_disconnected_;
    std::function<void()> on_error_;
    std::string url_;
};
