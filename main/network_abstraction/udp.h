#pragma once
#include <string>
#include <functional>
#include <vector>
#include <lwip/sockets.h>

class Udp {
public:
    Udp();
    ~Udp();

    void OnMessage(std::function<void(const std::string& data)> callback);
    void Connect(const std::string& host, int port);
    int Send(const std::vector<uint8_t>& data);
    int Send(const std::string& data) { return Send(std::vector<uint8_t>(data.begin(), data.end())); }

private:
    int sock_ = -1;
    struct sockaddr_in dest_addr_;
    bool connected_ = false;
    std::function<void(const std::string&)> on_message_;
};
