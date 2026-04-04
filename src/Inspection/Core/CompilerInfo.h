#ifndef COMPILERINFO_H
#define COMPILERINFO_H

#include <Core/SemVer.h>

#include <Windows.h>

#include <DbgHelp.h>

/*
   The basic dependancy tree for MSVC runtime

   ntdll.dll
    |
    +-- kernel32.dll
    |    |
    +----+-- msvcrt.dll (C Runtime)
              |
              +-- MSVCR*.DLL (C++ Runtime)
              |       |
              |       +-- MSVCR100.DLL (Visual C++ 2010)
              |       |
              |       +-- MSVCR110.DLL (Visual C++ 2012)
              |       |
              |       +-- MSVCR120.DLL (Visual C++ 2013)
              |       |
              |       +-- MSVCR140.DLL (Visual C++ 2015)
              |
              +-- MSVCP*.DLL (C++ Standard Library)
                      |
                      +-- MSVCP100.DLL (Visual C++ 2010)
                      |
                      +-- MSVCP110.DLL (Visual C++ 2012)
                      |
                      +-- MSVCP120.DLL (Visual C++ 2013)
                      |
                      +-- MSVCP140.DLL (Visual C++ 2015)

 */

struct CompilerInfo
{
    std::string name;
    SemVer version;

    std::string architecture;
};

// Function to get the compiler information
inline CompilerInfo getCompilerInfo()
{
    CompilerInfo info;

#if defined(__GNUC__) || defined(__GNUG__)
    info.name = "GCC";
    info.version.major = __GNUC__;
    info.version.minor = __GNUC_MINOR__;
    info.version.patch = __GNUC_PATCHLEVEL__;
#elif defined(_MSC_VER)
    info.name = "MSVC";
    info.version.major = _MSC_VER / 100; // Major version is MSVC version divided by 100
    info.version.minor = _MSC_VER % 100; // Minor version is the remainder
    info.version.patch = 0;              // MSVC does not have a patch version
#elif defined(__clang__)
    info.name = "Clang";
    info.version.major = __clang_major__;
    info.version.minor = __clang_minor__;
    info.version.patch = __clang_patchlevel__;
#else
    info.name = "Unknown compiler";
#endif

#if defined(__x86_64__) || defined(_M_X64)
    info.architecture = "x86_64";
#elif defined(__i386) || defined(_M_IX86)
    info.architecture = "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    info.architecture = "ARM64";
#elif defined(__arm__) || defined(_M_ARM)
    info.architecture = "ARM";
#else
    info.architecture = "Unknown architecture";
#endif

    return info;
}

// Function to get the current C/C++ runtime name
inline std::string getRuntimeName()
{
    std::string runtimeName;

#if defined(__GLIBCXX__)
    runtimeName = "GNU Standard C++ Library (libstdc++)";
#elif defined(_LIBCPP_VERSION)
    runtimeName = "LLVM C++ Standard Library (libc++)";
#elif defined(__GNUC__)
    runtimeName = "GCC Low-Level Runtime Library (libgcc)";
#elif defined(_MSC_VER)
    runtimeName = "Microsoft Visual C++ Runtime Library";
#elif defined(__clang__)
    runtimeName = "LLVM Runtime Library (compiler-rt)";
#else
    runtimeName = "Unknown C/C++ runtime library";
#endif

    return runtimeName;
}

// Function to get the version of a specified DLL
inline std::string getDLLVersion(const wchar_t *dllName)
{
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(GetModuleHandleW(dllName), path, MAX_PATH) == 0)
        return "";

    DWORD handle;
    DWORD size = GetFileVersionInfoSizeW(path, &handle);
    if (size == 0)
        return "";

    std::vector<wchar_t> buffer(size);
    if (!GetFileVersionInfoW(path, handle, size, buffer.data()))
        return "";

    VS_FIXEDFILEINFO *fileInfo;
    UINT len;
    if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<LPVOID *>(&fileInfo), &len))
        return "";

    DWORD versionMS = fileInfo->dwFileVersionMS;
    DWORD versionLS = fileInfo->dwFileVersionLS;
    DWORD major = HIWORD(versionMS);
    DWORD minor = LOWORD(versionMS);
    DWORD build = HIWORD(versionLS);
    DWORD revision = LOWORD(versionLS);

    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(build) + "." + std::to_string(revision);
}

// Function to get the MSVC runtime DLL name
std::string getMSVCRTDllName()
{
#if _MSC_VER >= 1920
    return "vcruntime140_clr0400.dll";
#elif _MSC_VER >= 1910
    return "vcruntime140_1.dll";
#elif _MSC_VER >= 1900
    return "vcruntime140.dll";
#elif _MSC_VER >= 1600
    return "msvcrt.dll";
#endif

    return "Unknown MSVCRT version";
}

std::string getMSVCPDllName()
{
#if _MSC_VER >= 1920
    return "msvcp140_clr0400.dll";
#elif _MSC_VER >= 1910
    return "msvcp140_1.dll";
#elif _MSC_VER >= 1900
    return "msvcp140.dll";
#elif _MSC_VER >= 1600
    return "msvcp100.dll";
#endif

    return "Unknown MSVCP version";
}

std::string getMSVCRDllName()
{
#if _MSC_VER >= 1920
    return "msvcr140_clr0400.dll";
#elif _MSC_VER >= 1910
    return "msvcr140_1.dll";
#elif _MSC_VER >= 1900
    return "msvcr140.dll";
#elif _MSC_VER >= 1600
    return "msvcr100.dll";
#endif

    return "Unknown MSVCR version";
}

#pragma comment(lib, "Dbghelp.lib")

std::vector<std::string> getImportedDlls()
{
    std::vector<std::string> dllNames;
    HMODULE hModule = GetModuleHandle(nullptr);
    if (hModule == nullptr)
        return dllNames;

    // Get the base address of the module
    ULONG size;
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(hModule, true, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);

    if (importDesc == nullptr)
        return dllNames;

    // Iterate through the import descriptor table
    while (importDesc->Name)
    {
        // Get the name of the imported DLL
        const char *dllName = (const char *)((BYTE *)hModule + importDesc->Name);
        std::string dllNameStr(dllName);

        // // Check if the DLL name contains MSVCR, MSVCP, or MSVCRT
        // if (dllNameStr.find("msvcr") != std::string::npos || dllNameStr.find("msvcp") != std::string::npos ||
        //     dllNameStr.find("msvcrt") != std::string::npos)
            dllNames.push_back(dllNameStr);

        importDesc++;
    }

    return dllNames;
}

// FIXME: this function works only at runtime, fix this behavior
// Function to get the version of the first valid MSVC runtime DLL
inline std::string getMSVCRTVersion()
{
    // List of possible MSVC runtime DLLs
    std::vector<const wchar_t *> dllNames = {L"msvcrt.dll", L"ucrtbase.dll", L"vcruntime140.dll", L"vcruntime140_1.dll", L"vcruntime140_clr0400.dll"};

    // Get the version of the first valid MSVC runtime DLL
    for (const auto &dllName : dllNames)
    {
        std::string version = getDLLVersion(dllName);

        if (!version.empty())
            return version;
    }

    return "No valid MSVC runtime DLL found."
           "Version information not available for MSVC";
}

// Function to get the current C/C++ runtime version
inline std::string getRuntimeVersion()
{
    std::string runtimeVersion;

#if defined(__GLIBCXX__)
    runtimeVersion = std::to_string(__GLIBCXX__);
#elif defined(_LIBCPP_VERSION)
    runtimeVersion = std::to_string(_LIBCPP_VERSION);
#elif defined(__GNUC__)
    runtimeVersion = std::to_string(__GNUC__);
#elif defined(_MSC_VER)
    runtimeVersion = "Version information not available for MSVC";
#elif defined(__clang__)
    runtimeVersion = "Version information not available for compiler-rt";
#else
    runtimeVersion = "Version information not available";
#endif

    return runtimeVersion;
}

// #include <iostream>

// int main()
// {
//     CompilerInfo info = getCompilerInfo();
//     std::cout << "Compiler: " << info.name << std::endl;
//     std::cout << "Version: " << info.version.major << "." << info.version.minor << "." << info.version.patch << std::endl;
//     std::cout << "Architecture: " << info.architecture << std::endl;

//     std::cout << "Runtime Name: " << getRuntimeName() << std::endl;
//     std::cout << "Runtime Version: " << getRuntimeVersion() << std::endl;

//     return 0;
// }

class IExtension
{
    virtual CompilerInfo compilerInfo() const = 0;
    virtual CompilerInfo minimumCompilerRequirements() const = 0;
};

extern "C" void *getPluginEntryPoint()
{
    static CompilerInfo *compilerInfo = new CompilerInfo();
    *compilerInfo = getCompilerInfo();

    return compilerInfo;
}

#endif // COMPILERINFO_H
