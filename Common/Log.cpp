#include "Log.h"
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#if _WIN32
#include <Windows.h>
#endif

namespace Log
{
    
    static ELogLevel gMinLogLevel = ELogLevel::Info;
    static std::mutex gLogMutex;
    
    static constexpr size_t gMessageBufferSize = 4096;
    static constexpr size_t gOutputBufferSize = 4112;
    static void* gMessageBuffer = nullptr;
    static void* gOutputBuffer = nullptr;

    void SetLogLevel(ELogLevel inLevel)
    {
        gMinLogLevel = inLevel;
    }

    void DefaultCallback(ELogLevel inLevel, const char* message)
    {
        const char* severityText = "";
        switch (inLevel)
        {
        case ELogLevel::Debug: severityText = "[DEBUG]:";  break;
        case ELogLevel::Info: severityText = "[INFO]:";  break;
        case ELogLevel::Warning: severityText = "[WARNING]"; break;
        case ELogLevel::Error: severityText = "[ERROR]"; break;
        case ELogLevel::Fatal: severityText = "[FATAL]"; break;
        default:
            break;
        }

        if(gOutputBuffer == nullptr)
        {
            gOutputBuffer = malloc(gOutputBufferSize);
        }
            
        char* output = new (gOutputBuffer) char[gOutputBufferSize];
        snprintf(output, gOutputBufferSize, "%s %s", severityText, message);
#if _WIN32
        OutputDebugStringA(output);
        OutputDebugStringA("\n");

        if (inLevel == ELogLevel::Fatal)
        {
            MessageBoxA(0, output, "Error", MB_ICONERROR);
        }      
#elif
        fprintf(stderr, "%s\n", output);
#endif
        if (inLevel == ELogLevel::Fatal)
            abort();
    }

    static Callback gCallBack = &DefaultCallback;

    void SetCallback(Callback inFunc)
    {
        gCallBack = inFunc;
    }
    
    Callback GetCallback()
    {
        return gCallBack;    
    }
    
    void ResetCallback()
    {
        gCallBack = &DefaultCallback;
    }

    void Message(ELogLevel inLevel, const char* inFormat, ...)
    {
        if (static_cast<int>(gMinLogLevel) > static_cast<int>(inLevel))
            return;

        std::lock_guard lock(gLogMutex);
        

        if(gMessageBuffer == nullptr)
        {
            gMessageBuffer = malloc(gMessageBufferSize);
        }

        va_list args;
        va_start(args, inFormat);
        uint32_t strSize = _vscprintf(inFormat, args) + 1;
        if(strSize < gMessageBufferSize)
        {
            char* message = new (gMessageBuffer) char[strSize];
            vsnprintf(message, strSize, inFormat, args);
            if(gCallBack)
                gCallBack(inLevel, message);
        }
        va_end(args);
    }

    void Debug(const char* inFormat, ...)
    {
        if (static_cast<int>(gMinLogLevel) > static_cast<int>(ELogLevel::Debug))
            return;

        std::lock_guard lock(gLogMutex);

        if(gMessageBuffer == nullptr)
        {
            gMessageBuffer = malloc(gMessageBufferSize);
        }

        va_list args;
        va_start(args, inFormat);
        uint32_t strSize = _vscprintf(inFormat, args) + 1;
        if(strSize < gMessageBufferSize)
        {
            char* message = new (gMessageBuffer) char[strSize];
            vsnprintf(message, strSize, inFormat, args);
            if(gCallBack)
                gCallBack(ELogLevel::Debug, message);
        }
        va_end(args);
    }
    
    void Info(const char* inFormat, ...)
    {
        if (static_cast<int>(gMinLogLevel) > static_cast<int>(ELogLevel::Info))
            return;

        std::lock_guard lock(gLogMutex);

        if(gMessageBuffer == nullptr)
        {
            gMessageBuffer = malloc(gMessageBufferSize);
        }

        va_list args;
        va_start(args, inFormat);
        uint32_t strSize = _vscprintf(inFormat, args) + 1;
        if(strSize < gMessageBufferSize)
        {
            char* message = new (gMessageBuffer) char[strSize];
            vsnprintf(message, strSize, inFormat, args);
            if(gCallBack)
                gCallBack(ELogLevel::Info, message);
        }
        va_end(args);
    }
    
    void Warning(const char* inFormat, ...)
    {
        if (static_cast<int>(gMinLogLevel) > static_cast<int>(ELogLevel::Warning))
            return;

        std::lock_guard lock(gLogMutex);

        if(gMessageBuffer == nullptr)
        {
            gMessageBuffer = malloc(gMessageBufferSize);
        }

        va_list args;
        va_start(args, inFormat);
        uint32_t strSize = _vscprintf(inFormat, args) + 1;
        if(strSize < gMessageBufferSize)
        {
            char* message = new (gMessageBuffer) char[strSize];
            vsnprintf(message, strSize, inFormat, args);
            if(gCallBack)
                gCallBack(ELogLevel::Warning, message);
        }
        va_end(args);
    }
    
    void Error(const char* inFormat, ...)
    {
        if (static_cast<int>(gMinLogLevel) > static_cast<int>(ELogLevel::Error))
            return;

        std::lock_guard lock(gLogMutex);

        if(gMessageBuffer == nullptr)
        {
            gMessageBuffer = malloc(gMessageBufferSize);
        }

        va_list args;
        va_start(args, inFormat);
        uint32_t strSize = _vscprintf(inFormat, args) + 1;
        if(strSize < gMessageBufferSize)
        {
            char* message = new (gMessageBuffer) char[strSize];
            vsnprintf(message, strSize, inFormat, args);
            if(gCallBack)
                gCallBack(ELogLevel::Error, message);
        }
        va_end(args);
    }
    
    void Fatal(const char* inFormat, ...)
    {
        if (static_cast<int>(gMinLogLevel) > static_cast<int>(ELogLevel::Fatal))
            return;

        std::lock_guard lock(gLogMutex);

        if(gMessageBuffer == nullptr)
        {
            gMessageBuffer = malloc(gMessageBufferSize);
        }

        va_list args;
        va_start(args, inFormat);
        uint32_t strSize = _vscprintf(inFormat, args) + 1;
        if(strSize < gMessageBufferSize)
        {
            char* message = new (gMessageBuffer) char[strSize];
            vsnprintf(message, strSize, inFormat, args);
            if(gCallBack)
                gCallBack(ELogLevel::Fatal, message);
        }
        va_end(args);
    }
}



