#include "QtInspector.h"

#include <QCoreApplication>

#include <thread>

#include <Windows.h>

void processAttached();
void processDetached();

class ModuleLifetimeInterceptor
{
    ModuleLifetimeInterceptor()
    {
        //
        // WARNING: Since `DllMain` runs under the loader lock,
        //          we must avoid doing anything complex here (like creating threads or loading other DLLs).
        //          We can queue a function to the thread pool from `DllMain`, which avoids creating a thread directly
        //          This is generally safe because the thread pool handles the threading behind the scenes.
        //
        QueueUserWorkItem(
            [](LPVOID)
            {
                processAttached();

                return DWORD(0);
            },
            nullptr, WT_EXECUTEDEFAULT);
    }

    ~ModuleLifetimeInterceptor()
    {
        processDetached();
    }

private:
    static ModuleLifetimeInterceptor s_instance;
};

ModuleLifetimeInterceptor ModuleLifetimeInterceptor::s_instance;

void waitForQtWorker()
{
    while (QCoreApplication::instance() == nullptr)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    QMetaObject::invokeMethod(QCoreApplication::instance(), PM::initializeQtInspector);
}

void processAttached()
{
    //
    // NOTE: We can always use std::thread.
    //       but since this thread lives as long as the application
    //       if QCoreApplication was never initialized, this means that the static thread that we created
    //       will be still running when the crt is trying to destroy it, which will cause an error
    //
    //       static std::thread t(waitForQtWorker);
    //       t.join(); // not calling join here is fatal
    //
    //       So we let the OS take care of such long running thread instead
    //

    ::CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(waitForQtWorker), nullptr, 0, nullptr);
}

void processDetached()
{
    // WARNING: showing a message box when the module is about to be unloaded usually causes the host process to crash
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return true;
}
