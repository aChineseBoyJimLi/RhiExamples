#include "RayTracingPipelineDx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        RayTracingPipelineDx app(320, 200, hInstance, TEXT("D3D12 Ray Tracing Pipeline"));
        app.Run();
        return 0;
    }
    catch (std::runtime_error& err)
    {
        Log::Error(err.what());
        return -1;
    }
}