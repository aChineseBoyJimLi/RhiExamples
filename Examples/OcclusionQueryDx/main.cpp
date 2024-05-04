#include "OcclusionQueryDx.h"
#include "Log.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        OcclusionQueryDx app(1280, 720, hInstance, TEXT("D3D12 Occlusion Query"));
        app.Run();
        return 0;
    }
    catch (std::runtime_error& err)
    {
        Log::Error(err.what());
        return -1;
    }
}