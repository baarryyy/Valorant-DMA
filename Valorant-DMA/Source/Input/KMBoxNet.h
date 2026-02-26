#pragma once
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <mutex>
#include <cstdlib>
#include <ctime>
#include "InputDevice.h"
#include "../Core/Logger.h"

#pragma comment(lib, "ws2_32.lib")

class KMBoxNet : public IInputDevice {
public:
    KMBoxNet() = default;
    ~KMBoxNet() override { Disconnect(); }

    KMBoxNet(const KMBoxNet&) = delete;
    KMBoxNet& operator=(const KMBoxNet&) = delete;

    void SetConnectionInfo(const std::string& ip, const std::string& port, const std::string& mac) {
        m_ip = ip;
        m_port = port;
        m_mac = mac;
    }

    bool Connect() override;
    void Disconnect() override;
    bool IsConnected() const override { return m_socket > 0; }
    void MoveMouse(int dx, int dy) override;
    void Click() override;

private:
    static constexpr uint32_t CMD_CONNECT    = 0xAF3C2828;
    static constexpr uint32_t CMD_MOUSE_MOVE = 0xAEDE7345;
    static constexpr uint32_t CMD_MOUSE_LEFT = 0x9823AE8D;
    static constexpr uint32_t CMD_REBOOT     = 0xAA8855AA;

#pragma pack(push, 1)
    struct CmdHead {
        uint32_t mac;
        uint32_t rand;
        uint32_t indexpts;
        uint32_t cmd;
    };

    struct SoftMouse {
        int button;
        int x;
        int y;
        int wheel;
        int point[10];
    };

    struct ClientPacket {
        CmdHead head;
        union {
            uint8_t rawBuff[1024];
            SoftMouse mouse;
        };
    };
#pragma pack(pop)

    static uint32_t ParseMac(const char* src, int nLen);

    SOCKET m_socket = 0;
    SOCKADDR_IN m_serverAddr = {};
    ClientPacket m_tx = {};
    ClientPacket m_rx = {};
    SoftMouse m_softMouse = {};
    std::string m_ip;
    std::string m_port;
    std::string m_mac;
    std::mutex m_mutex;
    bool m_wsaInitialized = false;
};
