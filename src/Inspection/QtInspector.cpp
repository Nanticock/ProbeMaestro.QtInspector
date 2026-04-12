#pragma comment(                                                                                                                                     \
    linker,                                                                                                                                          \
    "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "QtInspector.h"
#include "MainThreadHijacker.h"
#include "MainWindow.h"

#include <QMutex>

static QSharedPointer<MainThreadHijacker> s_mainThreadHijacker;
static QtMessageHandler s_nativeMessageHandler = nullptr;

void initMainThreadHijacker()
{
    s_mainThreadHijacker = QSharedPointer<MainThreadHijacker>::create();
    s_mainThreadHijacker->initialize();
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

    initMainThreadHijacker();

    static MainWindow mainWindow;
    mainWindow.show();
}
