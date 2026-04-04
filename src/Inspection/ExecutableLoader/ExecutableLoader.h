#ifndef EXECUTABLELOADER_H
#define EXECUTABLELOADER_H

#include "ExecutableLoader_p.h"

#include <string>

class ExecutableLoader
{
    friend ExecutableLoaderData;

public:
    typedef ExecutableLoaderData::ErrorHandlerFunction ErrorHandlerFunction;
    typedef ExecutableLoaderData::ExeEntryPointFunction ExeEntryPointFunction;
    typedef ExecutableLoaderData::InitializationFunction InitializationFunction;

    enum ErrorHandlerType
    {
        LoadingErrorHandler,
        RunningErrorHandler,
        FreeingErrorHandler
    };

public:
    ExecutableLoader();

    virtual void waitUntilFinished() const;

    bool runExecutable(const std::string &exePath, int argc, char **argv);
    bool runExecutableAsync(const std::string &exePath, int argc, char **argv);

    virtual bool runExecutable(const std::wstring &exePath, int argc, char **argv);
    virtual bool runExecutableAsync(const std::wstring &exePath, int argc, char **argv);

    virtual size_t addInitializationFunction(const InitializationFunction &func);
    virtual const std::list<InitializationFunction> &initializationFunctions() const;

    virtual ErrorHandlerFunction setErrorHandler(ErrorHandlerType type, const ErrorHandlerFunction &function);

public:
    NO_DISCARD std::wstring moduleName() const;
    NO_DISCARD std::wstring moduleFile() const;

    NO_DISCARD bool isLoaded() const;
    NO_DISCARD bool isRunning() const;

    NO_DISCARD int exitCode() const;
    NO_DISCARD void *moduleBaseAddress() const;
    NO_DISCARD ExeEntryPointFunction entryPoint() const;

private:
    ExecutableLoaderData d;
};

#endif // EXECUTABLELOADER_H
