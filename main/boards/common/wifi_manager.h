#pragma once

// Minimal stub for WifiManager - provided by 78/esp-wifi-connect in v2.x
class WifiManager {
public:
    void StartConfigAp() {}
    void StopConfigAp() {}
    bool IsConfigApRunning() { return false; }
};
