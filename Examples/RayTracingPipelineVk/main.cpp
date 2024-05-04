#include "RayTracingPipelineVk.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        RayTracingPipelineVk app(1280, 720, hInstance, TEXT("Vulkan Ray Tracing Pipeline"));
        app.Run();
        return 0;
    }
    catch (std::runtime_error& err)
    {
        Log::Error(err.what());
        return -1;
    }
}