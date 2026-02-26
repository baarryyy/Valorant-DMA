#include "KMBoxSerial.h"

std::string KMBoxSerial::FindPort(const std::string& targetDescription) {
    HDEVINFO hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) return "";

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &deviceInfoData); ++i) {
        char buf[512];
        DWORD nSize = 0;

        if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_FRIENDLYNAME,
                NULL, (PBYTE)buf, sizeof(buf), &nSize) && nSize > 0) {
            buf[nSize] = '\0';
            std::string desc = buf;

            size_t comPos = desc.find("COM");
            size_t endPos = desc.find(")", comPos);

            if (comPos != std::string::npos && endPos != std::string::npos &&
                desc.find(targetDescription) != std::string::npos) {
                SetupDiDestroyDeviceInfoList(hDevInfo);
                return desc.substr(comPos, endPos - comPos);
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return "";
}

bool KMBoxSerial::OpenPort(const char* portName, DWORD baudRate) {
    m_serial = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (m_serial == INVALID_HANDLE_VALUE) return false;

    DCB dcb = {};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(m_serial, &dcb)) {
        CloseHandle(m_serial);
        m_serial = INVALID_HANDLE_VALUE;
        return false;
    }

    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;

    if (!SetCommState(m_serial, &dcb)) {
        CloseHandle(m_serial);
        m_serial = INVALID_HANDLE_VALUE;
        return false;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 100;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(m_serial, &timeouts)) {
        CloseHandle(m_serial);
        m_serial = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

bool KMBoxSerial::SendCommand(const std::string& command) {
    if (m_serial == INVALID_HANDLE_VALUE) return false;
    DWORD bytesWritten;
    return WriteFile(m_serial, command.c_str(), (DWORD)command.length(), &bytesWritten, NULL);
}

bool KMBoxSerial::Connect() {
    Disconnect();

    std::string port = FindPort(m_driverName);
    if (port.empty()) {
        LOG_ERROR("KMBox B+ serial port not found (driver: {})", m_driverName);
        return false;
    }

    std::string fullPort = "\\\\.\\" + port;
    if (!OpenPort(fullPort.c_str(), m_baudRate)) {
        LOG_ERROR("Failed to open KMBox B+ on {}", port);
        return false;
    }

    LOG_INFO("KMBox B+ connected on {} @ {}", port, m_baudRate);
    return true;
}

void KMBoxSerial::Disconnect() {
    if (m_serial != INVALID_HANDLE_VALUE) {
        CloseHandle(m_serial);
        m_serial = INVALID_HANDLE_VALUE;
    }
}

void KMBoxSerial::MoveMouse(int dx, int dy) {
    std::ostringstream cmd;
    cmd << "km.move(" << dx << "," << dy << ")\r\n";
    SendCommand(cmd.str());
}

void KMBoxSerial::Click() {
    SendCommand("km.left(1)\r\n");
    SendCommand("km.left(0)\r\n");
}
