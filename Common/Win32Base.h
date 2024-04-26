#pragma once

#include <Windows.h>
#include <list>
#include <cstdint>



class Win32Base
{
public:
    Win32Base(uint32_t inWidth, uint32_t inHeight, HINSTANCE inHInstance, const char* inTitle);
    void Run();
    virtual ~Win32Base();

protected:
    virtual bool Init() = 0;
    virtual void Tick() = 0;
    virtual void Shutdown() = 0;
    
    virtual void OnResize(int width, int height){ }
    virtual void OnMouseLeftBtnDown(int x, int y){ }
    virtual void OnMouseLeftBtnUp(int x, int y)  { }
    virtual void OnMouseRightBtnDown(int x, int y){ }
    virtual void OnMouseRightBtnUp(int x, int y)  { }
    virtual void OnMouseMidBtnDown(int x, int y){ }
    virtual void OnMouseMidBtnUp(int x, int y)  { }
    virtual void OnMouseMove(int x, int y){ }
    virtual void OnMouseWheeling(int x, int y, int delta) { }

    uint32_t m_Width;
    uint32_t m_Height;
    HINSTANCE const m_hInstance;
    HWND m_hWnd;
    bool m_IsRunning = true;

    float GetDeltaTime() const { return m_Timer.DeltaTime; }
    float GetTotalTimeSinceStart() const { return m_Timer.TotalTimeSinceStart; }

private:
    static std::list<Win32Base*> s_Listeners;
    static LRESULT CALLBACK MsgProc(HWND HWnd, UINT Msg, WPARAM WParam, LPARAM LParam);

    struct Timer
    {
        float DeltaTime {0};
        float TotalTimeSinceStart {0};
    };

    Timer m_Timer;
};