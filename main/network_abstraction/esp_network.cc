#include "esp_network.h"
#include "http.h"
#include "web_socket.h"
#include "mqtt.h"
#include "udp.h"
#include <esp_log.h>

static const char* TAG = "EspNetwork";

EspNetwork::EspNetwork() {}
EspNetwork::~EspNetwork() {}

Http* EspNetwork::CreateHttp(int timeout_ms) {
    return new Http();
}

WebSocket* EspNetwork::CreateWebSocket(int timeout_ms) {
    return new WebSocket();
}

Mqtt* EspNetwork::CreateMqtt(int timeout_ms) {
    return new Mqtt();
}

Udp* EspNetwork::CreateUdp(int timeout_ms) {
    return new Udp();
}

// ========== Http implementation ==========

Http::Http() {}
Http::~Http() {
    Close();
}

void Http::SetHeader(const std::string& name, const std::string& value) {
    // Headers are set when opening the connection via esp_http_client_set_header
    // Store for now, apply in Open()
    ESP_LOGD(TAG, "SetHeader: %s: %s", name.c_str(), value.c_str());
}

void Http::SetContent(std::string data) {
    post_data_ = std::move(data);
}

bool Http::Open(const std::string& method, const std::string& url) {
    Close();

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = (method == "POST") ? HTTP_METHOD_POST : HTTP_METHOD_GET;
    config.timeout_ms = 10000;
    config.buffer_size = 4096;
    config.buffer_size_tx = 2048;

    client_ = esp_http_client_init(&config);
    if (!client_) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return false;
    }

    if (!post_data_.empty()) {
        esp_http_client_set_post_field(client_, post_data_.data(), post_data_.size());
    }

    esp_err_t err = esp_http_client_perform(client_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(err));
        return false;
    }

    status_code_ = esp_http_client_get_status_code(client_);
    return true;
}

int Http::GetStatusCode() {
    return status_code_;
}

int64_t Http::GetBodyLength() {
    if (!client_) return 0;
    return esp_http_client_get_content_length(client_);
}

int Http::Read(char* buffer, size_t buf_size) {
    if (!client_) return -1;
    return esp_http_client_read(client_, buffer, buf_size);
}

std::string Http::ReadAll() {
    if (!client_) return "";
    std::string result;
    char buf[512];
    int len;
    while ((len = esp_http_client_read(client_, buf, sizeof(buf))) > 0) {
        result.append(buf, len);
    }
    return result;
}

void Http::Close() {
    if (client_) {
        esp_http_client_cleanup(client_);
        client_ = nullptr;
    }
}

// ========== WebSocket implementation ==========

WebSocket::WebSocket() {}
WebSocket::~WebSocket() {
    if (sock_ >= 0) close(sock_);
}

bool WebSocket::IsConnected() { return connected_; }

bool WebSocket::Send(const std::string& text) {
    return Send((const uint8_t*)text.data(), text.size(), false);
}

bool WebSocket::Send(const uint8_t* data, size_t len, bool binary) {
    if (sock_ < 0) return false;
    // Simple WebSocket frame send (unmasked)
    uint8_t header[10];
    size_t header_len = 2;
    header[0] = binary ? 0x82 : 0x81; // FIN + opcode
    if (len < 126) {
        header[1] = len;
    } else if (len < 65536) {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        header_len = 4;
    } else {
        header[1] = 127;
        for (int i = 0; i < 8; i++) header[2 + i] = (len >> (56 - i * 8)) & 0xFF;
        header_len = 10;
    }
    if (send(sock_, header, header_len, 0) < 0) return false;
    return send(sock_, data, len, 0) >= 0;
}

void WebSocket::OnData(std::function<void(const char*, size_t, bool)> callback) {
    on_data_ = std::move(callback);
}

void WebSocket::OnDisconnected(std::function<void()> callback) {
    on_disconnected_ = std::move(callback);
}

void WebSocket::OnError(std::function<void()> callback) {
    on_error_ = std::move(callback);
}

bool WebSocket::Connect(const std::string& url) {
    if (sock_ >= 0) close(sock_);

    // Parse URL (ws:// or wss://)
    bool ssl = url.find("wss://") == 0;
    std::string host_port = url.substr(ssl ? 6 : 5);
    size_t path_pos = host_port.find('/');
    std::string host = (path_pos != std::string::npos) ? host_port.substr(0, path_pos) : host_port;
    std::string path = (path_pos != std::string::npos) ? host_port.substr(path_pos) : "/";

    size_t colon = host.find(':');
    std::string hostname = (colon != std::string::npos) ? host.substr(0, colon) : host;
    int port = (colon != std::string::npos) ? std::stoi(host.substr(colon + 1)) : (ssl ? 443 : 80);

    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) return false;

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, hostname.c_str(), &addr.sin_addr);

    if (connect(sock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock_); sock_ = -1; return false;
    }

    // Send WebSocket upgrade request
    std::string upgrade = "GET " + path + " HTTP/1.1\r\n"
                          "Host: " + host + "\r\n"
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                          "Sec-WebSocket-Version: 13\r\n\r\n";

    if (send(sock_, upgrade.data(), upgrade.size(), 0) < 0) {
        close(sock_); sock_ = -1; return false;
    }

    // Read HTTP 101 response
    char resp[1024];
    int n = recv(sock_, resp, sizeof(resp) - 1, 0);
    if (n <= 0) { close(sock_); sock_ = -1; return false; }
    resp[n] = 0;

    connected_ = strstr(resp, "101") != nullptr;
    return connected_;
}

// ========== Mqtt implementation ==========

Mqtt::Mqtt() {}
Mqtt::~Mqtt() {
    if (client_) {
        esp_mqtt_client_stop(client_);
        esp_mqtt_client_destroy(client_);
    }
}

void Mqtt::SetKeepAlive(int seconds) {
    // Keep alive is set during connect via config
    ESP_LOGD(TAG, "SetKeepAlive: %d", seconds);
}

void Mqtt::OnDisconnected(std::function<void()> callback) {
    on_disconnected_ = std::move(callback);
}

void Mqtt::OnMessage(std::function<void(const std::string&, const std::string&)> callback) {
    on_message_ = std::move(callback);
}

bool Mqtt::Connect(const std::string& address, int port, const std::string& client_id,
                   const std::string& username, const std::string& password) {
    esp_mqtt_client_config_t config = {};
    config.broker.address.uri = address.c_str();
    config.broker.address.port = port;
    config.credentials.client_id = client_id.c_str();
    config.credentials.username = username.empty() ? nullptr : username.c_str();
    config.credentials.authentication.password = password.empty() ? nullptr : password.c_str();

    client_ = esp_mqtt_client_init(&config);
    if (!client_) return false;

    return esp_mqtt_client_start(client_) == ESP_OK;
}

bool Mqtt::Publish(const std::string& topic, const std::string& text) {
    if (!client_) return false;
    int msg_id = esp_mqtt_client_publish(client_, topic.c_str(), text.c_str(), 0, 0, 0);
    return msg_id >= 0;
}

bool Mqtt::IsConnected() {
    return client_ && esp_mqtt_client_get_state(client_) == MQTT_CONNECTED;
}

// ========== Udp implementation ==========

Udp::Udp() {}
Udp::~Udp() {
    if (sock_ >= 0) {
        close(sock_);
    }
}

void Udp::OnMessage(std::function<void(const std::string&)> callback) {
    on_message_ = std::move(callback);
}

void Udp::Connect(const std::string& host, int port) {
    if (sock_ >= 0) close(sock_);
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) return;

    dest_addr_.sin_family = AF_INET;
    dest_addr_.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &dest_addr_.sin_addr);
    connected_ = true;
}

int Udp::Send(const std::vector<uint8_t>& data) {
    if (sock_ < 0 || !connected_) return -1;
    return sendto(sock_, data.data(), data.size(), 0,
                  (struct sockaddr*)&dest_addr_, sizeof(dest_addr_));
}
