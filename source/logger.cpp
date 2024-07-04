#include "atelier/atelier_base.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <varargs.h>
#include <string>

static std::string fmt_va(const char* const msg, va_list va_args)
{
    // How many chars we need
    size_t required_size = vsnprintf(nullptr, 0, msg, va_args);

    // vsn printf doesn't count null terminator!
    std::string out;
    out.resize(required_size + 1, '\0');

    // Perform the format in the buffer
    vsnprintf(out.data(), out.size(), msg, va_args);
    return out;
}

void Atelier::Log::init()
{
#ifndef NDEBUG
    // Do we have a console attached in win32?
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    }

#endif
}

void Atelier::Log::shutdown()
{
#ifndef NDEBUG
    system("pause");
#endif
}

void Atelier::Log::unformatted(const char* const msg) { printf("%s", msg); }

void Atelier::Log::info(const char* const msg, ...)
{
    va_list args;
    va_start(args, msg);
    auto fmt = fmt_va(msg, args);
    va_end(args);

    fmt = "INFO: " + fmt;
    unformatted(fmt.c_str());
    unformatted("\n");
}

void Atelier::Log::warn(const char* const msg, ...)
{
    va_list args;
    va_start(args, msg);
    auto fmt = fmt_va(msg, args);
    va_end(args);

    fmt = "WARN: " + fmt + "\n";
    unformatted(fmt.c_str());
    unformatted("\n");
}

void Atelier::Log::error(const char* const msg, ...)
{
    va_list args;
    va_start(args, msg);
    auto fmt = fmt_va(msg, args);
    va_end(args);

    fmt = "ERROR: " + fmt + "\n";
    unformatted(fmt.c_str());
    unformatted("\n");
}
