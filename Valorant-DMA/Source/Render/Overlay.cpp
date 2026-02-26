#include "Overlay.h"
#include "Font.h"
#include "Style.h"

// Static instance pointer for the WndProc callback
static Overlay* s_instance = nullptr;

LRESULT CALLBACK Overlay::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (s_instance && s_instance->m_device && wParam != SIZE_MINIMIZED) {
            if (s_instance->m_renderTarget) {
                s_instance->m_renderTarget->Release();
                s_instance->m_renderTarget = nullptr;
            }
            s_instance->m_swapChain->ResizeBuffers(0, s_instance->m_width, s_instance->m_height, DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* backBuffer;
            s_instance->m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
            s_instance->m_device->CreateRenderTargetView(backBuffer, NULL, &s_instance->m_renderTarget);
            backBuffer->Release();
            s_instance->m_context->OMSetRenderTargets(1, &s_instance->m_renderTarget, NULL);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Overlay::Init(int width, int height) {
    s_instance = this;
    m_width = width;
    m_height = height;

    WNDCLASSEXA wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXA);
    wcex.style = CS_CLASSDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hIcon = LoadIcon(0, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(0, IDC_ARROW);
    wcex.lpszClassName = "OverlayWndClass";
    wcex.hIconSm = LoadIcon(0, IDI_APPLICATION);
    RegisterClassExA(&wcex);

    m_window = CreateWindowExA(
        0, "OverlayWndClass", "",
        WS_POPUP | WS_VISIBLE,
        0, 0, m_width, m_height,
        nullptr, nullptr, wcex.hInstance, nullptr
    );

    if (!m_window) {
        LOG_ERROR("Failed to create overlay window");
        return false;
    }

    SetWindowLong(m_window, GWL_EXSTYLE, GetWindowLong(m_window, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(m_window, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margin = { -1 };
    DwmExtendFrameIntoClientArea(m_window, &margin);
    ShowWindow(m_window, SW_SHOW);
    UpdateWindow(m_window);

    // Query actual refresh rate
    DEVMODEA devMode = {};
    devMode.dmSize = sizeof(DEVMODEA);
    UINT refreshRate = 60;
    if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &devMode))
        refreshRate = devMode.dmDisplayFrequency;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = m_width;
    sd.BufferDesc.Height = m_height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = refreshRate;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_window;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (FAILED(D3D11CreateDeviceAndSwapChain(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
            &featureLevel, 1, D3D11_SDK_VERSION,
            &sd, &m_swapChain, &m_device, NULL, &m_context))) {
        LOG_ERROR("Failed to create D3D11 device and swap chain");
        return false;
    }

    ID3D11Texture2D* backBuffer;
    m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
    m_device->CreateRenderTargetView(backBuffer, NULL, &m_renderTarget);
    backBuffer->Release();
    m_context->OMSetRenderTargets(1, &m_renderTarget, NULL);

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)m_width;
    vp.Height = (float)m_height;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(m_window);
    ImGui_ImplDX11_Init(m_device, m_context);
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(font, sizeof(font), 14.0f);
    ImGui::GetIO().Fonts->Build();

    m_initialized = true;
    LOG_INFO("Overlay initialized ({}x{} @ {}Hz)", m_width, m_height, refreshRate);
    return true;
}

void Overlay::BeginFrame() {
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)cursorPos.x, (float)cursorPos.y);
    io.MouseDown[0] = GetAsyncKeyState(VK_LBUTTON) != 0;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void Overlay::EndFrame(bool transparent) {
    ImGui::EndFrame();

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, transparent ? 0.0f : 1.0f };
    m_context->ClearRenderTargetView(m_renderTarget, clearColor);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT result = m_swapChain->Present(1, 0);  // VSync enabled
    if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET) {
        LOG_WARN("D3D device lost, recreating resources");
        ImGui_ImplDX11_InvalidateDeviceObjects();
        // Recreate render target
        ID3D11Texture2D* backBuffer;
        m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
        m_device->CreateRenderTargetView(backBuffer, NULL, &m_renderTarget);
        backBuffer->Release();
        ImGui_ImplDX11_CreateDeviceObjects();
    }
}

bool Overlay::ProcessMessages() {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

Overlay::~Overlay() {
    Shutdown();
}

void Overlay::Shutdown() {
    if (!m_initialized) return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (m_renderTarget) { m_renderTarget->Release(); m_renderTarget = nullptr; }
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
    if (m_window) { DestroyWindow(m_window); m_window = nullptr; }

    s_instance = nullptr;
    m_initialized = false;
}
