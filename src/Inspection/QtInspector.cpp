#pragma comment(                                                                                                                                     \
    linker,                                                                                                                                          \
    "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "QtInspector.h"

#include <Compat/MemoryMaps/FunctionsMemoryMaps/FunctionsMemoryMap.h>
#include <Compat/MemoryMaps/ObjectsMemoryMap/QtObjectsMemoryMap.h>
#include <ExecutableLoader/ExecutableLoader.h>
#include <ExecutableLoader/ExecutableLoader_p.h>
#include <MainThreadHijacker.h>

#include <QDebug>
#include <QGuiApplication>
#include <QMutex>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QThread>
#include <QTimer>

#include <Windows.h>
#include <shellapi.h>

#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#endif

void openURL(const std::string &url)
{
#ifdef _WIN32
    ShellExecute(0, 0, url.c_str(), 0, 0, SW_SHOW);
#else
    std::string command;
#ifdef __APPLE__
    command = "open " + url;
#else
    command = "xdg-open " + url;
#endif

    system(command.c_str());
#endif
}

std::wstring getModuleName(HMODULE hModule)
{
    wchar_t moduleName[MAX_PATH];
    if (GetModuleFileNameW(hModule, moduleName, MAX_PATH) == 0)
        // Handle error, if any
        return L"";

    return std::wstring(moduleName);
}

static QSharedPointer<MainThreadHijacker> s_mainThreadHijacker;
static QtMessageHandler s_nativeMessageHandler = nullptr;

bool initMemoryMaps(ExecutableLoader &loader)
{
    std::cout << "initializing memory maps..." << std::endl;

    Compat::MemoryMap::setEntryPoint(size_t(loader.entryPoint()), 0x140D06658);

    Compat::MemoryMap::registerMemoryMap(QSharedPointer<Compat::QtObjectsMemoryMap>::create());
    Compat::MemoryMap::registerMemoryMap(QSharedPointer<Compat::FunctionsMemoryMap>::create());

    Compat::MemoryMap::init();

    return true;
}

void initMainThreadHijacker()
{
    QTimer::singleShot(0, QGuiApplication::instance(),
                       []()
                       {
                           s_mainThreadHijacker = QSharedPointer<MainThreadHijacker>::create();
                           s_mainThreadHijacker->initialize();
                       });
}

void alternativeQtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (s_nativeMessageHandler)
        s_nativeMessageHandler(type, context, msg);

    static QMutex mutex;
    QMutexLocker locker(&mutex);

    // don't display internal Qt Debug Messages
    if (type != QtDebugMsg)
        fprintf(stdout, "%s\n", msg.toStdString().c_str());
}

void helperThreadBody()
{
    while (QGuiApplication::allWindows().count() == 0)
        ;

    QQmlEngine *activeEngine = qmlEngine(QGuiApplication::allWindows().first());

    while (true)
    {
        std::wstring input;
        qInfo() << "Enter a command:";
        std::getline(std::wcin, input);

        QString command = QString::fromStdWString(input);

        if (command.toLower() == "list windows")
        {
            for (QWindow *window : QGuiApplication::allWindows())
            {
                qInfo().nospace() << window->objectName() << " (" << window->winId() << "): {" << window->title() << ", " << window->visibility()
                                  << "}";
            }
        }
        else if (command.toLower() == "list children")
        {
            for (QWindow *window : QGuiApplication::allWindows())
            {
                window->dumpObjectTree();
            }
        }
        else if (command.toLower() == "window")
        {
            qInfo() << "window created";
            QTimer::singleShot(0, s_mainThreadHijacker.data(), &MainThreadHijacker::showAuxiliaryWindow);
        }
        else if (command.toLower() == "test")
        {
            QTimer::singleShot(0, s_mainThreadHijacker.data(), &MainThreadHijacker::showComponentsTest);
        }
        else if (command.startsWith("evaluate"))
        {
            if (activeEngine->evaluate(command.remove("evaluate").trimmed()).isError())
                qInfo() << "error:";
            else
                qInfo() << "success:";
        }
        else if (command.toLower() == "debug1")
        {
        }

        qInfo() << "user command is:" << command;
    }
}

void loaderErrorHandler(ExecutableLoader &loader, const std::wstring &message)
{
    int result = MessageBoxW(0, message.data(), L"Fatal error", MB_ICONERROR | MB_YESNO);

    if (result == IDYES)
        openURL(internal::toString(L"probemaestro://invalid_url"));

    // TODO: implement prompting the user to choose if he wants to create a dll out of the given input file
    // if he chose to do so, generate a dll from the file, run the application again, and exit
    exit(0);
}

void runningErrorHandler(ExecutableLoader &loader, const std::wstring &message)
{
    loaderErrorHandler(loader, message);
}

// TODO: make this function into a stand-alone tool
// with the sole purpose of figuring out what version of the crt to use
std::string getMSVCRuntimeVersion()
{
    char path[MAX_PATH];
    if (GetModuleFileNameA(GetModuleHandleA("msvcrt.dll"), path, MAX_PATH) == 0)
        return "Failed to get MSVCRT path";

    DWORD handle;
    DWORD size = GetFileVersionInfoSizeA(path, &handle);
    if (size == 0)
        return "Failed to get version info size";

    std::vector<char> buffer(size);
    if (!GetFileVersionInfoA(path, handle, size, buffer.data()))
        return "Failed to get version info";

    VS_FIXEDFILEINFO *fileInfo;
    UINT len;
    if (!VerQueryValueA(buffer.data(), "\\", reinterpret_cast<LPVOID *>(&fileInfo), &len))
        return "Failed to query version value";

    DWORD versionMS = fileInfo->dwFileVersionMS;
    DWORD versionLS = fileInfo->dwFileVersionLS;
    DWORD major = HIWORD(versionMS);
    DWORD minor = LOWORD(versionMS);
    DWORD build = HIWORD(versionLS);
    DWORD revision = LOWORD(versionLS);

    return std::string(path) + ": " + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(build) + "." +
           std::to_string(revision);
}

void PM::initializeQtInspector()
{
    s_nativeMessageHandler = qInstallMessageHandler(alternativeQtMessageHandler);

    // create the helper thread and run it.
    // We create a helper thread using QThread because The Qt QObjects cannot be used from threads
    // that were not created using any method other than a QThread
    QThread *helperThread = QThread::create(helperThreadBody);
    helperThread->start();

    // initialize the main thread hijacker
    QThread::create(initMainThreadHijacker)->start();

    std::cout << "main thread: " << QGuiApplication::instance()->thread() << ", helper thread: " << helperThread << std::endl;
}
