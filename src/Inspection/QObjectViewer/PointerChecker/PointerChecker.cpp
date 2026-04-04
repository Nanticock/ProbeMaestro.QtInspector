#include "PointerChecker.h"

#include <QByteArray>

#include <Windows.h>

namespace Internal
{
bool isReadableAddress(HANDLE hProcess, const void *address, size_t nSize)
{
    // Refer to:
    // https://sl.bing.net/cC2d0HXy1ca

    QByteArray buffer(nSize, 0x0);

    // A variable to store the number of bytes read
    size_t bytesRead;

    // Try to read from the target process
    return ReadProcessMemory(hProcess, address, buffer.data(), nSize, &bytesRead) != 0;
}

bool isWritableAddress(const void *address, size_t nSize)
{
    // Refer to:
    // https://sl.bing.net/hivXRI0h91U

    void *_address = const_cast<void *>(address);

    DWORD oldProtection;
    // Try to change the protection to PAGE_READWRITE
    if (VirtualProtect(_address, nSize, PAGE_READWRITE, &oldProtection) == 0)
        // Failed to change protection
        return false;

    // Restore the original protection
    VirtualProtect(_address, nSize, oldProtection, nullptr);

    // Check if the memory region was not writable before
    return (oldProtection & (PAGE_READONLY | PAGE_NOACCESS | PAGE_GUARD)) == 0;
}

bool isExecutableAddress(const void *address)
{
    // Refer to:
    // https://sl.bing.net/W8rYvAgkuW

    MEMORY_BASIC_INFORMATION memoryBasicInformation;
    if (VirtualQuery(address, &memoryBasicInformation, sizeof(memoryBasicInformation)) == 0)
        // Failed to query memory information
        return false;

    // Check if the memory region is executable
    return (memoryBasicInformation.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}
} // namespace Internal

bool BasicPointerChecker::isReadableAddress(size_t address, size_t size)
{
    return isReadableAddress(reinterpret_cast<const void *>(address), size);
}

bool BasicPointerChecker::isReadableAddress(const void *address, size_t size)
{
    // Refer to: https://sl.bing.net/cC2d0HXy1ca
    return Internal::isReadableAddress(GetCurrentProcess(), address, size);
}
