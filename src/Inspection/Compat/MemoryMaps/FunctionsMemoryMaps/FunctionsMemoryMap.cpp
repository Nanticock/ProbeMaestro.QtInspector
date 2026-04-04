#include "FunctionsMemoryMap.h"

#include <ExecutableLoader/ExecutableLoader_p.h>
#include <MemoryMaps/MemoryMap_p.h>

using namespace Compat;

namespace
{
const char FUNCTION_MEMORY_MAP_TAG[] = "function";
}

bool FunctionsMemoryMap::s_isInitialized = false;

void *FunctionsMemoryMap::getFunction(const QString &name)
{
    MemoryMapPrivate::MemoryMapEntry *entry = MemoryMapPrivate::getAddressByName(name.toStdString());

    if (entry == nullptr)
        return nullptr;

    if (!MemoryMapPrivate::satisfiesFilter(*entry, {"", {FUNCTION_MEMORY_MAP_TAG}}))
        return nullptr;

    return entry->address;
}

void *FunctionsMemoryMap::addFunction(const QString &name, size_t offset, const QString &sectionName)
{
    void *result = offsetToPointer<void>(sectionName, offset);

    if (!result)
        return nullptr;

    ::MemoryMap::addAddress(name.toStdString(), result, getModuleFileNameFromAddress(result), {FUNCTION_MEMORY_MAP_TAG});

    return result;
}

bool FunctionsMemoryMap::initialize()
{
    if (!MemoryMap::initialize())
        return false;

    // TODO: Call addFunction here

    s_isInitialized = true;
    return true;
}

bool FunctionsMemoryMap::deinitialize()
{
    if (!MemoryMap::deinitialize())
        return false;

    ::MemoryMap::clear();

    s_isInitialized = false;
    return true;
}

bool FunctionsMemoryMap::isInitialized()
{
    return s_isInitialized;
}
