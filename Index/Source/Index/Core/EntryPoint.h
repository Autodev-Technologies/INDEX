#if defined(INDEX_PLATFORM_WINDOWS)

#include "Core/CoreSystem.h"
#include "Platform/Windows/WindowsOS.h"

#pragma comment(linker, "/subsystem:windows")

#ifndef NOMINMAX
#define NOMINMAX // For windows.h
#endif

#include <windows.h>

extern Index::Application* Index::CreateApplication();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifndef INDEX_PRODUCTION
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif

    if(!Index::Internal::CoreSystem::Init(0, nullptr))
        return 0;

    auto windowsOS = new Index::WindowsOS();
    Index::OS::SetInstance(windowsOS);

    windowsOS->Init();

    Index::CreateApplication();

    windowsOS->Run();
    delete windowsOS;

    Index::Internal::CoreSystem::Shutdown();
    return 0;
}

#elif defined(INDEX_PLATFORM_LINUX)

#include "Core/CoreSystem.h"
#include "Platform/Unix/UnixOS.h"

extern Index::Application* Index::CreateApplication();

int main(int argc, char** argv)
{
    if(!Index::Internal::CoreSystem::Init(argc, argv))
        return 0;

    auto unixOS = new Index::UnixOS();
    Index::OS::SetInstance(unixOS);
    unixOS->Init();

    Index::CreateApplication();

    unixOS->Run();
    delete unixOS;

    Index::Internal::CoreSystem::Shutdown();
}

#elif defined(INDEX_PLATFORM_MACOS)

#include "Platform/MacOS/MacOSOS.h"

int main(int argc, char** argv)
{
    if(!Index::Internal::CoreSystem::Init(argc, argv))
        return 0;

    auto macOSOS = new Index::MacOSOS();
    Index::OS::SetInstance(macOSOS);
    macOSOS->Init();

    Index::CreateApplication();

    macOSOS->Run();
    delete macOSOS;

    Index::Internal::CoreSystem::Shutdown();
}

#elif defined(INDEX_PLATFORM_IOS)

#endif
