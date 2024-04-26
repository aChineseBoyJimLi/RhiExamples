#pragma once
#include <functional>
namespace Log
{
    enum class ELogLevel
    {
        Debug = 0,
        Info,
        Warning,
        Error,
        Fatal,
        None,
    };

    typedef std::function<void(ELogLevel, char const*)> Callback;

    void        SetLogLevel(ELogLevel inLevel);
    void        SetCallback(Callback inFunc);
    Callback    GetCallback();
    void        ResetCallback();
    void        Message(ELogLevel inLevel, const char* inFormat, ...);
    void        Debug(const char* inFormat, ...);
    void        Info(const char* inFormat, ...);
    void        Warning(const char* inFormat, ...);
    void        Error(const char* inFormat, ...);
    void        Fatal(const char* inFormat, ...);
}
