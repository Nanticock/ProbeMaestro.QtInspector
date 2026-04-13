#include "ExecutableLoader.h"
#include "ExecutableLoader_p.h"

#include <cwctype>
#include <iostream>
#include <thread>

#define WAIT_SLEEP_INTERVAL_MS 250 // 0.25 seconds

MainParams::MainParams() : MainParams(0, nullptr)
{
}

MainParams::MainParams(int argc, char **argv) : argc(argc), argv(argv)
{
}

ExecutableLoaderData::ExecutableLoaderData(ExecutableLoader *q) :
    q(q),
    exitCode(0),
    moduleIsRunning(false),
    moduleHandle(0),
    moduleImageBase(0),
    entryPoint(nullptr),
    moduleFreeingErrorHandler(ExecutableLoaderData::defaultErrorHandler),
    moduleLoadingErrorHandler(ExecutableLoaderData::defaultErrorHandler),
    moduleRunningErrorHandler(ExecutableLoaderData::defaultErrorHandler)
{
}

bool ExecutableLoaderData::freeModule()
{
    if (FreeLibrary(HMODULE(moduleHandle)))
    {
        reset();

        return true;
    }

    std::wstring message = L"couldn't free module: \"" + moduleName + L"\"";
    moduleFreeingErrorHandler(*q, message);

    return false;
}

bool ExecutableLoaderData::loadModule(const std::wstring &moduleFile)
{
    HMODULE newModuleHandle = LoadLibraryW(moduleFile.data());

    if (!newModuleHandle)
    {
        std::wstring message = L"couldn't load module: \"" + moduleFile + L"\"";
        moduleLoadingErrorHandler(*q, message);
        return false;
    }

    moduleIsRunning = false;
    this->moduleFile = moduleFile;
    moduleHandle = newModuleHandle;
    moduleName = internal::getFileName(moduleFile);
    entryPoint = internal::getModuleEntryPoint(moduleHandle);
    moduleImageBase = internal::getModuleImageBase(moduleHandle);

    return true;
}

DWORD ExecutableLoaderData::callEntryPointInThread_stub(LPVOID lpParam)
{
    WinThreadMemberFunctionsParamsWrapper<MainParams> *paramsWrapper = reinterpret_cast<WinThreadMemberFunctionsParamsWrapper<MainParams> *>(lpParam);

    ExecutableLoaderData *_this = reinterpret_cast<ExecutableLoaderData *>(paramsWrapper->_this);

    DWORD result = _this->callEntryPoint(paramsWrapper->params.argc, paramsWrapper->params.argv);

    // make sure we delete the
    delete paramsWrapper;

    return result;
}

void ExecutableLoaderData::reset()
{
    moduleImageBase = 0;
    entryPoint = nullptr;
    moduleHandle = nullptr;
    moduleIsRunning = false;
}

// Refer to: https://sl.bing.net/gFbIFpar94K
unsigned long ExecutableLoaderData::callEntryPoint(int argc, char **argv)
{
    if (entryPoint != nullptr)
    {
        // TODO: report functions that don't return true
        for (auto function : initializationFunctions)
            function(*q);

        moduleIsRunning = true;
        exitCode = entryPoint(argc, argv);
        moduleIsRunning = false;

        return exitCode;
    }
    else
    {
        std::wstring message = L"couldn't locate the entrypoint of module: \"" + moduleName + L"\"";
        moduleRunningErrorHandler(*q, message);
    }

    freeModule();

    exitCode = -1;
    reset();

    return exitCode;
}

unsigned long ExecutableLoaderData::loadAndRunModule(const std::wstring &modulePath, int argc, char **argv)
{
    if (!loadModule(modulePath))
        return -1;

    return callEntryPoint(argc, argv);
}

void ExecutableLoaderData::defaultErrorHandler(ExecutableLoader &loader, const std::wstring &message)
{
    std::wcerr << message;
}

/// @brief takes the memory offset to an object inside the current module (from a tool like IDA)
/// and returns a usable pointer to this object in the current application memory space.
///
/// @warning returns nullptr if the current module wasn't loaded yet.
/// @note this function should be moved to a more appropriate public-facing interface.
void *ExecutableLoaderData::moduleMemoryOffsetToPointer(size_t offset) const
{
    if (moduleHandle == nullptr)
        return nullptr;

    size_t result = offset - moduleImageBase + size_t(moduleHandle);

    return reinterpret_cast<void *>(result);
}

ExecutableLoaderData &ExecutableLoaderData::getPrivate(ExecutableLoader &loader)
{
    return loader.d;
}

const ExecutableLoaderData &ExecutableLoaderData::getPrivate(const ExecutableLoader &loader)
{
    return getPrivate(const_cast<ExecutableLoader &>(loader));
}

bool ExecutableLoader::runExecutable(const std::wstring &exePath, int argc, char **argv)
{
    return d.loadAndRunModule(exePath, argc, argv);
}

bool ExecutableLoader::runExecutable(const std::string &exePath, int argc, char **argv)
{
    return runExecutable(internal::toWString(exePath.data()), argc, argv);
}

bool ExecutableLoader::runExecutableAsync(const std::string &exePath, int argc, char **argv)
{
    return runExecutableAsync(internal::toWString(exePath.data()), argc, argv);
}

bool ExecutableLoader::runExecutableAsync(const std::wstring &exePath, int argc, char **argv)
{
    if (isRunning())
    {
        std::wstring message = L"Module: \"" + exePath + L"\" is already loaded and running";
        d.moduleRunningErrorHandler(*this, message);
        return false;
    }

    if (!d.loadModule(exePath))
        return false;

    // TODO: make a more generic function that can run anything and use it here
    void *params = new WinThreadMemberFunctionsParamsWrapper<MainParams>(&d, {argc, argv});

    // Create a new thread
    HANDLE result = CreateThread(nullptr,                                           // Default security attributes
                                 0,                                                 // Default stack size
                                 ExecutableLoaderData::callEntryPointInThread_stub, // Thread function
                                 params,                                            // No parameter
                                 true,                                              // Run immediately
                                 nullptr                                            // No thread ID returned
    );

    return result;
}

size_t ExecutableLoader::addInitializationFunction(const InitializationFunction &func)
{
    d.initializationFunctions.push_back(func);

    return d.initializationFunctions.size();
}

const std::list<ExecutableLoader::InitializationFunction> &ExecutableLoader::initializationFunctions() const
{
    return d.initializationFunctions;
}

ExecutableLoader::ErrorHandlerFunction ExecutableLoader::setErrorHandler(ErrorHandlerType type, const ErrorHandlerFunction &function)
{
    ExecutableLoader::ErrorHandlerFunction result = ExecutableLoaderData::defaultErrorHandler;

    switch (type)
    {
    case LoadingErrorHandler:
        result = d.moduleLoadingErrorHandler;
        d.moduleLoadingErrorHandler = function;
        return result;

    case RunningErrorHandler:
        result = d.moduleRunningErrorHandler;
        d.moduleRunningErrorHandler = function;
        return result;

    case FreeingErrorHandler:
        result = d.moduleFreeingErrorHandler;
        d.moduleFreeingErrorHandler = function;
        return result;
    }

    return result;
}

ExecutableLoader::ExecutableLoader() : d(this)
{
}

void ExecutableLoader::waitUntilFinished() const
{
    if (!isRunning())
        return;

    while (isRunning())
        std::this_thread::sleep_for(std::chrono::duration<long, std::milli>(WAIT_SLEEP_INTERVAL_MS));

    return;
}

std::wstring ExecutableLoader::moduleName() const
{
    return d.moduleName;
}

std::wstring ExecutableLoader::moduleFile() const
{
    return d.moduleFile;
}

int ExecutableLoader::exitCode() const
{
    return d.exitCode;
}

bool ExecutableLoader::isLoaded() const
{
    return d.moduleHandle;
}

bool ExecutableLoader::isRunning() const
{
    return d.moduleIsRunning;
}

void *ExecutableLoader::moduleBaseAddress() const
{
    return d.moduleHandle;
}

ExecutableLoader::ExeEntryPointFunction ExecutableLoader::entryPoint() const
{
    return d.entryPoint;
}

/**
 * @note this function is guaranteed to provide lossless conversion,
 *
 * converting back and forth between Narrow and Wide versions
 * of the same string shall never cause any data loss
 */
std::string internal::toString(const std::wstring &input)
{
    // TODO: add support for other Operating Systems

    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (bufferSize <= 0)
        return "";

    std::string result(bufferSize - 1, 0); // bufferSize includes null-terminator
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, &result[0], bufferSize, nullptr, nullptr);

    return result;
}

/**
 * @note this function is guaranteed to provide lossless conversion,
 *
 * converting back and forth between Narrow and Wide versions
 * of the same string shall never cause any data loss
 */
std::wstring internal::toWString(const std::string &input)
{
    // TODO: add support for other Operating Systems

    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (bufferSize <= 0)
        return L"";

    std::wstring result(bufferSize - 1, 0); // bufferSize includes null-terminator
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &result[0], bufferSize);
    return result;
}

std::wstring internal::trimString(const std::wstring &input)
{
    std::wstring s = input;

    // Left trim (remove leading whitespace)
    size_t start = 0;
    while (start < s.size() && iswspace(s[start]))
        ++start;

    // Right trim (remove trailing whitespace)
    size_t end = s.size();
    while (end > start && iswspace(s[end - 1]))
        --end;

    return s.substr(start, end - start);
}

std::string internal::trimString(const std::string &input)
{
    std::wstring ws = toWString(input);
    std::wstring result = trimString(ws);

    return toString(result);
}

std::wstring internal::getFileName(const std::wstring &filePath)
{
    //
    // NOTE: cannot use `std::filesystem::path` as it's a c++17 feature
    //       which means the code won't be able to compile against old c++ runtimes that doesn't support c++17
    //

    wchar_t buffer[MAX_PATH];
    wchar_t *filePart = nullptr;

    GetFullPathNameW(filePath.c_str(), MAX_PATH, buffer, &filePart);

    if (filePart != nullptr)
        return std::wstring(filePart);

    return filePath;
}

size_t internal::getModuleImageBase(void *moduleHandle)
{
    if (moduleHandle == nullptr)
        return 0;

    const IMAGE_DOS_HEADER *dosHeader = static_cast<const IMAGE_DOS_HEADER *>(moduleHandle);
    const IMAGE_NT_HEADERS *ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS *>(size_t(moduleHandle) + dosHeader->e_lfanew);

    return ntHeaders->OptionalHeader.ImageBase;
}

ExecutableLoaderData::ExeEntryPointFunction internal::getModuleEntryPoint(void *moduleHandle)
{
    // TODO: replace this default method

    // Get the address of _exe_Main_ by ordinal
    return ExecutableLoader::ExeEntryPointFunction(GetProcAddress(HMODULE(moduleHandle), MAKEINTRESOURCE(1)));
}

std::wstring internal::getModuleName(void *moduleHandle)
{
    return getFileName(getModuleFileName(moduleHandle));
}

std::wstring internal::getModuleFileName(void *moduleHandle)
{
    wchar_t buffer[MAX_PATH];
    DWORD size = GetModuleFileNameW(HMODULE(moduleHandle), buffer, MAX_PATH);

    if (size == 0)
        return L"";

    return std::wstring(buffer);
}

void *internal::getCurrentModuleHandle()
{
    return reinterpret_cast<void *>(getModuleImageBase(nullptr));
}

void *internal::getModuleHandleFromAddress(const void *address)
{
    HMODULE hModule = nullptr;
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, LPCTSTR(address), &hModule))
        return hModule;

    return nullptr;
}
