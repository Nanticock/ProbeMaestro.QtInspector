#ifndef COMPAT_MEMORYMAP_H
#define COMPAT_MEMORYMAP_H

#include <QHash>
#include <QSharedPointer>

namespace Compat
{
/// @deprecated
class MemoryMap
{
public:
    typedef void *voidPtr;

public:
    static void init();
    static void deinit();

    static bool registerMemoryMap(QSharedPointer<MemoryMap> map, bool initialize = true);
    static bool unregisterMemoryMap(QSharedPointer<MemoryMap> map, bool deinitialize = true);

    static size_t imageOffset();
    static void setImageOffset(size_t value);

    static size_t getSectionBase(const QString &sectionName);
    static void setSectionBase(const QString &sectionName, size_t baseValue);

    /**
     * @param entryPointMemoryAddress the address of the entry point of the executable after getting loaded in memory
     * @param entryPointFileOffset the address of the entry point of the executable before getting loaded in memory
     */
    static void setEntryPoint(size_t entryPointMemoryAddress, size_t entryPointFileOffset);

    static void *offsetToPointer(const QString &sectionName, size_t offset);

    template <typename T>
    static T *offsetToPointer(const QString &sectionName, size_t offset);

protected:
    // these two functions should return if the class is already initialized
    // they can get called multiple times without causing any problems
    virtual bool initialize();
    virtual bool deinitialize();

    virtual bool isInitialized() = 0;

    static std::wstring getModuleNameFromAddress(const void *address);
    static std::wstring getModuleFileNameFromAddress(const void *address);

private:
    static QHash<QString, size_t> &executableFileSectionsBases();

private:
    static size_t s_imageOffset;
    static QHash<QString, size_t> s_executableFileSectionsBases;
    static QSet<QSharedPointer<MemoryMap>> s_registeredMaps;
};

template <typename T>
T *MemoryMap::offsetToPointer(const QString &sectionName, size_t offset)
{
    return reinterpret_cast<T *>(offsetToPointer(sectionName, offset));
}
} // namespace Compat

#endif // COMPAT_MEMORYMAP_H
