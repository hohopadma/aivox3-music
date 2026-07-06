#pragma once
#include <string>
#include <functional>
#include <memory>
#include <mqtt_client.h>

class Mqtt {
public:
    Mqtt();
    ~Mqtt();

    void SetKeepAlive(int seconds);
    void OnDisconnected(std::function<void()> callback);
    void OnMessage(std::function<void(const std::string& topic, const std::string& payload)> callback);
    bool Connect(const std::string& address, int port, const std::string& client_id,
                 const std::string& username, const std::string& password);
    bool Publish(const std::string& topic, const std::string& text);
    bool IsConnected();

private:
    esp_mqtt_client_handle_t client_ = nullptr;
    std::function<void()> on_disconnected_;
    std::function<void(const std::string&, const std::string&)> on_message_;
    bool connected_ = false;
};
