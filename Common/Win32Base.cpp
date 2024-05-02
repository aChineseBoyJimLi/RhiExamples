#include "Win32Base.h"
#include <WindowsX.h>
#include <stdexcept>
#include <chrono>

#include "Log.h"

Win32Base::Win32Base(uint32_t inWidth, uint32_t inHeight, HINSTANCE inHInstance, const char* inTitle)
        : m_Width(inWidth)
        , m_Height(inHeight)
        , m_hInstance(inHInstance)
{
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MsgProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("WinApp");

    if(!RegisterClass(&wc))
    {
        throw std::runtime_error("Failed to register window class");
    }

    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = { 0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height) };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    const int width  = R.right - R.left;
    const int height = R.bottom - R.top;

    m_hWnd = CreateWindow(
        TEXT("WinApp"), inTitle,
        WS_OVERLAPPEDWINDOW^WS_THICKFRAME^WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, 0, m_hInstance, 0);

    if(m_hWnd == nullptr)
    {
        throw std::runtime_error("Failed to create window");
    }
    
    s_Listeners.push_back(this);
}

Win32Base::~Win32Base()
{
    s_Listeners.remove(this);
}

void Win32Base::Run()
{
    if(!Init())
    {
        Shutdown();
        Log::Fatal("Failed to initialize application");
        return;
    }

    ShowWindow(m_hWnd, SW_NORMAL);
    UpdateWindow(m_hWnd);

    // Main Loop
    auto lastTime = std::chrono::high_resolution_clock::now();
    auto startTime = std::chrono::high_resolution_clock::now();
    m_IsRunning = true;
    MSG Msg = { 0 };
    while (m_IsRunning)
    {
        if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            if (Msg.message == WM_QUIT)
            {
                m_IsRunning = false;
            }
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_Timer.DeltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        m_Timer.TotalTimeSinceStart = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        lastTime = currentTime;
        Tick();
    }
    
    Shutdown();
}

std::list<Win32Base*> Win32Base::s_Listeners;

LRESULT CALLBACK Win32Base::MsgProc(HWND HWnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    switch (Msg)
    {
    case WM_CLOSE: 
        DestroyWindow(HWnd);
        break;

    case WM_DESTROY: 
        PostQuitMessage(0);
        break;

    case WM_ENTERSIZEMOVE:
        break;

    case WM_EXITSIZEMOVE:
        break;

    case WM_SIZE:
        for (auto i : s_Listeners)
            i->OnResize(LOWORD(LParam), HIWORD(LParam));
        break;

    case WM_LBUTTONDOWN:
        for (auto i : s_Listeners)
            i->OnMouseLeftBtnDown(GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));
        break;
        
    case WM_MBUTTONDOWN:
        for (auto i : s_Listeners)
            i->OnMouseMidBtnDown(GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));
        break;
        
    case WM_RBUTTONDOWN:
        for (auto i : s_Listeners)
            i->OnMouseRightBtnDown(GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));
        break;

    case WM_LBUTTONUP:
        for (auto i : s_Listeners)
            i->OnMouseLeftBtnUp(GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));
        break;
        
    case WM_MBUTTONUP:
        for (auto i : s_Listeners)
            i->OnMouseMidBtnUp(GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));
        break;
        
    case WM_RBUTTONUP:
        for (auto i : s_Listeners)
            i->OnMouseRightBtnUp(GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));
        break;

    case WM_MOUSEMOVE:
        for (auto i : s_Listeners)
            i->OnMouseMove(GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam));
        break;
        
    case WM_MOUSEWHEEL :
        for (auto i : s_Listeners)
            i->OnMouseWheeling(GET_X_LPARAM(LParam), GET_Y_LPARAM(LParam), GET_WHEEL_DELTA_WPARAM(WParam));
        break;

    default:
        return DefWindowProc(HWnd, Msg, WParam, LParam);
    }
    return 0;
}