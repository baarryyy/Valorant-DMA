#include "Core/Application.h"

int main() {
    SetConsoleTitleA("Valorant DMA");

    Application app;
    if (!app.Init()) {
        LOG_ERROR("Application initialization failed");
        Sleep(3000);
        return 1;
    }

    app.Run();
    app.Shutdown();
    return 0;
}
