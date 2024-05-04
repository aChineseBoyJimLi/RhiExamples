#include "MeshPipelineDx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        std::unique_ptr<MeshPipelineDx> app = std::make_unique<MeshPipelineDx>(1280, 720, hInstance, TEXT("D3D12 Mesh Pipeline"));
        app->Run();
        return 0;
    }
    catch (std::runtime_error& err)
    {
        Log::Error(err.what());
        return -1;
    }
}