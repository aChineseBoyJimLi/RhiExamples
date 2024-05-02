#include "IndirectDrawDx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        IndirectDrawDx app(1280, 720, hInstance, TEXT("D3D12 Indirect Draw"));
        app.Run();
        return 0;
    }
    catch (std::runtime_error& err)
    {
        Log::Error(err.what());
        return -1;
    }
}