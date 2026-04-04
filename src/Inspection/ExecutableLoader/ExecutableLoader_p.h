#ifndef EXECUTABLELOADER_P_H
#define EXECUTABLELOADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the ProbeMaestro API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
//

#include <Windows.h>

#include <functional>
#include <list>
#include <string>

// TODO: move to a more appropriate location
#if __cplusplus >= 201703L
#define NO_DISCARD [[nodiscard]]
#else
#define NO_DISCARD
#endif

struct MainParams
{
    int argc;
    char **argv;

    MainParams();
    MainParams(int argc, char **argv);
};

template <typename T>
struct WinThreadMemberFunctionsParamsWrapper
{
    void *_this = nullptr;
    T params;

    explicit WinThreadMemberFunctionsParamsWrapper(void *_this, const T &value);

    template <typename... Args>
    static WinThreadMemberFunctionsParamsWrapper<T> *create(void *_this, Args &&...arguments)
    {
        T _params(std::forward<Args>(arguments)...);
        return new WinThreadMemberFunctionsParamsWrapper<T>(_this, _params);
    }
};

template <typename T>
inline WinThreadMemberFunctionsParamsWrapper<T>::WinThreadMemberFunctionsParamsWrapper(void *_this, const T &value) : _this(_this), params(value)
{
}

class ExecutableLoader;

class ExecutableLoaderData
{
public:
    typedef int(__cdecl *ExeEntryPointFunction)(int argc, char **argv);
    typedef std::function<bool(ExecutableLoader &loader)> InitializationFunction;
    typedef std::function<void(ExecutableLoader &loader, const std::wstring &message)> ErrorHandlerFunction;

public:
    ExecutableLoader *q;

    std::wstring moduleName;
    std::wstring moduleFile;

    int exitCode;
    bool moduleIsRunning;

    void *moduleHandle;
    size_t moduleImageBase; // TODO: maybe add this to the public interface in the future?!!
    ExeEntryPointFunction entryPoint;

    // error handlers
    ErrorHandlerFunction moduleFreeingErrorHandler;
    ErrorHandlerFunction moduleLoadingErrorHandler;
    ErrorHandlerFunction moduleRunningErrorHandler;

    // list of functions that will be executed after the application gets loaded into memory and before calling it's EntryPoint
    std::list<InitializationFunction> initializationFunctions;

public:
    ExecutableLoaderData(ExecutableLoader *q);

    bool freeModule();
    bool loadModule(const std::wstring &moduleFile);
    unsigned long callEntryPoint(int argc, char **argv);
    unsigned long loadAndRunModule(const std::wstring &modulePath, int argc, char **argv);

    static void defaultErrorHandler(ExecutableLoader &loader, const std::wstring &message);

    // TODO: move into the public interface
    void *moduleMemoryOffsetToPointer(size_t offset) const;

    // NOTE: these are for debugging purposes
    static ExecutableLoaderData &getPrivate(ExecutableLoader &loader);
    static const ExecutableLoaderData &getPrivate(const ExecutableLoader &loader);

public: // TODO: this should be private
    static DWORD WINAPI callEntryPointInThread_stub(LPVOID lpParam);

private:
    void reset();
};

// TODO: move to a more appropriate location
namespace internal
{
// string operations
std::string toString(const std::wstring &input);
std::wstring toWString(const std::string &input);

std::string trimString(const std::string &input);
std::wstring trimString(const std::wstring &input);

// path manipulation
std::wstring getFileName(const std::wstring &filePath);

// executable images operations
void *getCurrentModuleHandle();
void *getModuleHandleFromAddress(const void *address);
size_t getModuleImageBase(void *moduleHandle);
std::wstring getModuleName(void *moduleHandle);
std::wstring getModuleFileName(void *moduleHandle);
ExecutableLoaderData::ExeEntryPointFunction getModuleEntryPoint(void *moduleHandle);
} // namespace internal

#endif // EXECUTABLELOADER_P_H
