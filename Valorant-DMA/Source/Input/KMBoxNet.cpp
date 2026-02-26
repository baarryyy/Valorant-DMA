#include "KMBoxNet.h"

uint32_t KMBoxNet::ParseMac(const char* src, int nLen) {
    unsigned int pbDest[16] = { 0 };
    for (int i = 0; i < nLen && i < 16; i++) {
        char h1 = src[2 * i];
        char h2 = src[2 * i + 1];
        unsigned char s1 = (unsigned char)(toupper(h1) - 0x30);
        if (s1 > 9) s1 -= 7;
        unsigned char s2 = (unsigned char)(toupper(h2) - 0x30);
        if (s2 > 9) s2 -= 7;
        pbDest[i] = s1 * 16 + s2;
    }
    return pbDest[0] << 24 | pbDest[1] << 16 | pbDest[2] << 8 | pbDest[3];
}

bool KMBoxNet::Connect() {
    Disconnect();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("KMBox .NET: WSAStartup failed");
        return false;
    }
    m_wsaInitialized = true;

    srand((unsigned)time(NULL));
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket == INVALID_SOCKET) {
        LOG_ERROR("KMBox .NET: Failed to create socket");
        WSACleanup();
        m_wsaInitialized = false;
        return false;
    }

    // Set receive timeout (5 seconds)
    DWORD timeout = 5000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    m_serverAddr.sin_addr.S_un.S_addr = inet_addr(m_ip.c_str());
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(atoi(m_port.c_str()));

    m_tx.head.mac = ParseMac(m_mac.c_str(), 4);
    m_tx.head.rand = rand();
    m_tx.head.indexpts = 0;
    m_tx.head.cmd = CMD_CONNECT;

    memset(&m_softMouse, 0, sizeof(m_softMouse));

    int err = sendto(m_socket, (const char*)&m_tx, sizeof(CmdHead), 0,
                     (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
    if (err < 0) {
        LOG_ERROR("KMBox .NET: Failed to send connect packet");
        Disconnect();
        return false;
    }

    Sleep(20);

    int clen = sizeof(m_serverAddr);
    err = recvfrom(m_socket, (char*)&m_rx, 1024, 0, (struct sockaddr*)&m_serverAddr, &clen);
    if (err < 0) {
        LOG_ERROR("KMBox .NET: Connection timeout");
        Disconnect();
        return false;
    }

    if (m_rx.head.cmd != m_tx.head.cmd || m_rx.head.indexpts != m_tx.head.indexpts) {
        LOG_ERROR("KMBox .NET: Invalid connection response");
        Disconnect();
        return false;
    }

    LOG_INFO("KMBox .NET connected to {}:{}", m_ip, m_port);
    return true;
}

void KMBoxNet::Disconnect() {
    if (m_socket > 0) {
        closesocket(m_socket);
        m_socket = 0;
    }
    if (m_wsaInitialized) {
        WSACleanup();
        m_wsaInitialized = false;
    }
}

void KMBoxNet::MoveMouse(int dx, int dy) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_socket <= 0) return;

    m_tx.head.indexpts++;
    m_tx.head.cmd = CMD_MOUSE_MOVE;
    m_tx.head.rand = rand();
    m_softMouse.x = dx;
    m_softMouse.y = dy;
    memcpy(&m_tx.mouse, &m_softMouse, sizeof(SoftMouse));
    m_softMouse.x = 0;
    m_softMouse.y = 0;

    int length = sizeof(CmdHead) + sizeof(SoftMouse);
    sendto(m_socket, (const char*)&m_tx, length, 0, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));

    SOCKADDR_IN from;
    int fromLen = sizeof(from);
    recvfrom(m_socket, (char*)&m_rx, 1024, 0, (struct sockaddr*)&from, &fromLen);
}

void KMBoxNet::Click() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_socket <= 0) return;

    // Mouse down
    m_tx.head.indexpts++;
    m_tx.head.cmd = CMD_MOUSE_LEFT;
    m_tx.head.rand = rand();
    m_softMouse.button |= 0x01;
    memcpy(&m_tx.mouse, &m_softMouse, sizeof(SoftMouse));
    int length = sizeof(CmdHead) + sizeof(SoftMouse);
    sendto(m_socket, (const char*)&m_tx, length, 0, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));

    SOCKADDR_IN from;
    int fromLen = sizeof(from);
    recvfrom(m_socket, (char*)&m_rx, 1024, 0, (struct sockaddr*)&from, &fromLen);

    Sleep(10);

    // Mouse up
    m_tx.head.indexpts++;
    m_tx.head.rand = rand();
    m_softMouse.button &= ~0x01;
    memcpy(&m_tx.mouse, &m_softMouse, sizeof(SoftMouse));
    sendto(m_socket, (const char*)&m_tx, length, 0, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
    recvfrom(m_socket, (char*)&m_rx, 1024, 0, (struct sockaddr*)&from, &fromLen);
}
