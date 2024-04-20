#include "MeshPipelineDx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        MeshPipelineDx app(380, 200, hInstance, TEXT("D3D12 Mesh Pipeline"));
        app.Run();
        return 0;
    }
    catch (std::runtime_error& err)
    {
        Log::Error(err.what());
        return -1;
    }
}