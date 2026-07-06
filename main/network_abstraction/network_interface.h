#pragma once
#include <string>
#include <memory>
#include <functional>
#include <vector>

class Http;
class WebSocket;
class Mqtt;
class Udp;

class NetworkInterface {
public:
    virtual ~NetworkInterface() = default;
    virtual Http* CreateHttp(int timeout_ms) = 0;
    virtual WebSocket* CreateWebSocket(int timeout_ms) = 0;
    virtual Mqtt* CreateMqtt(int timeout_ms) = 0;
    virtual Udp* CreateUdp(int timeout_ms) = 0;
    virtual bool IsConnected() { return true; }
};
