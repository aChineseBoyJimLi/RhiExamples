#include "GpuQueryDx.h"
#include "Log.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        GpuQueryDx app(1280, 720, hInstance, TEXT("D3D12 GPU Query"));
        app.Run();
        return 0;
    }
    catch (std::runtime_error& err)
    {
        Log::Error(err.what());
        return -1;
    }
}