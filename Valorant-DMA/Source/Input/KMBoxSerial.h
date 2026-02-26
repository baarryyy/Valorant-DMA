#pragma once
#include <Windows.h>
#include <string>
#include <setupapi.h>
#include <devguid.h>
#include <sstream>
#include "InputDevice.h"
#include "../Core/Logger.h"

#pragma comment(lib, "setupapi.lib")

class KMBoxSerial : public IInputDevice {
public:
    KMBoxSerial() = default;
    ~KMBoxSerial() override { Disconnect(); }

    KMBoxSerial(const KMBoxSerial&) = delete;
    KMBoxSerial& operator=(const KMBoxSerial&) = delete;

    void SetBaudRate(DWORD rate) { m_baudRate = rate; }
    void SetDriverName(const std::string& name) { m_driverName = name; }

    bool Connect() override;
    void Disconnect() override;
    bool IsConnected() const override { return m_serial != INVALID_HANDLE_VALUE; }
    void MoveMouse(int dx, int dy) override;
    void Click() override;

private:
    std::string FindPort(const std::string& targetDescription);
    bool OpenPort(const char* portName, DWORD baudRate);
    bool SendCommand(const std::string& command);

    HANDLE m_serial = INVALID_HANDLE_VALUE;
    DWORD m_baudRate = 115200;
    std::string m_driverName = "USB-SERIAL CH340";
};
