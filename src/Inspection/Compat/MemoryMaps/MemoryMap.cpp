#include "MemoryMap.h"

#include <ExecutableLoader/ExecutableLoader_p.h>
#include <compat_Qt.h>

#include <QSet>

namespace Compat
{
size_t MemoryMap::s_imageOffset = 0x0;
QHash<QString, size_t> MemoryMap::s_executableFileSectionsBases;
QSet<QSharedPointer<MemoryMap>> MemoryMap::s_registeredMaps;
} // namespace Compat

using namespace Compat;

void MemoryMap::init()
{
    for (auto memoryMap : PM::internal::qAsConst(s_registeredMaps))
        memoryMap->initialize();
}

void MemoryMap::deinit()
{
    for (auto memoryMap : PM::internal::qAsConst(s_registeredMaps))
        memoryMap->initialize();
}

size_t MemoryMap::imageOffset()
{
    return s_imageOffset;
}

void MemoryMap::setImageOffset(size_t value)
{
    s_imageOffset = value;
}

size_t MemoryMap::getSectionBase(const QString &sectionName)
{
    return s_executableFileSectionsBases[sectionName];
}

void MemoryMap::setSectionBase(const QString &sectionName, size_t baseValue)
{
    s_executableFileSectionsBases[sectionName] = baseValue;
}

void MemoryMap::setEntryPoint(size_t entryPointMemoryAddress, size_t entryPointFileOffset)
{
    setImageOffset(entryPointMemoryAddress - entryPointFileOffset);
    setSectionBase("text", imageOffset());
}

void *MemoryMap::offsetToPointer(const QString &sectionName, size_t offset)
{
    return reinterpret_cast<void *>(offset + getSectionBase(sectionName));
}

std::wstring MemoryMap::getModuleNameFromAddress(const void *address)
{
    return internal::getFileName(getModuleFileNameFromAddress(address));
}

std::wstring MemoryMap::getModuleFileNameFromAddress(const void *address)
{
    void *moduleHandle = internal::getModuleHandleFromAddress(address);

    return internal::getModuleFileName(moduleHandle);
}

bool MemoryMap::registerMemoryMap(QSharedPointer<MemoryMap> map, bool initialize)
{
    if (map.isNull())
        return false;

    if (s_registeredMaps.contains(map))
        return false;

    if (initialize)
        map->initialize();

    s_registeredMaps << map;

    return true;
}

bool MemoryMap::unregisterMemoryMap(QSharedPointer<MemoryMap> map, bool deinitialize)
{
    if (map.isNull())
        return false;

    if (!s_registeredMaps.contains(map))
        return false;

    if (deinitialize)
        map->deinitialize();

    s_registeredMaps.remove(map);

    return true;
}

bool MemoryMap::initialize()
{
    if (isInitialized())
        return false;

    return true;
}

bool MemoryMap::deinitialize()
{
    if (!isInitialized())
        return false;

    return true;
}

QHash<QString, size_t> &MemoryMap::executableFileSectionsBases()
{
    return s_executableFileSectionsBases;
}
