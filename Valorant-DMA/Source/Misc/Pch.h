#pragma once

// Preprocessor
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

// Windows core
#include <Windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windowsx.h>
#include <dwmapi.h>

// STL
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <algorithm>
#include <filesystem>
#include <cstdint>
#include <chrono>
#include <optional>
#include <functional>

// DMA
#include <vmmdll.h>

// JSON
#include <json.hpp>

// Logging
#include <spdlog/spdlog.h>
