#pragma once

// Abstract interface for input devices (KMBox serial, KMBox .NET, etc.)
class IInputDevice {
public:
    virtual ~IInputDevice() = default;
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;
    virtual void MoveMouse(int dx, int dy) = 0;
    virtual void Click() = 0;
};
