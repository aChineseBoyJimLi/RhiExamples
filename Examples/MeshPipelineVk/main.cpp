#include "MeshPipelineVk.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        std::unique_ptr<MeshPipelineVk> app = std::make_unique<MeshPipelineVk>(1280, 720, hInstance, TEXT("Vulkan Indirect Draw"));
        app->Run();
        return 0;
    }
    catch (std::runtime_error& err)
    {
        Log::Error(err.what());
        return -1;
    }
}