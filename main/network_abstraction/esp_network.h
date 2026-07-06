#pragma once
#include "network_interface.h"

class EspNetwork : public NetworkInterface {
public:
    EspNetwork();
    virtual ~EspNetwork();

    virtual Http* CreateHttp(int timeout_ms) override;
    virtual WebSocket* CreateWebSocket(int timeout_ms) override;
    virtual Mqtt* CreateMqtt(int timeout_ms) override;
    virtual Udp* CreateUdp(int timeout_ms) override;
};
