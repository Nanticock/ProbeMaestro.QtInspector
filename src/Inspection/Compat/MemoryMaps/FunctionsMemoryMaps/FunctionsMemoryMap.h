#ifndef COMPAT_FUNCTIONSMEMORYMAP_H
#define COMPAT_FUNCTIONSMEMORYMAP_H

#include <Compat/MemoryMaps/MemoryMap.h>

#include <QFile>

namespace Compat
{
/// @deprecated
struct FunctionsMemoryMap : public MemoryMap
{
    template <typename T>
    static T getFunction(const QString &name)
    {
        return reinterpret_cast<T>(getFunction(name));
    }
    static void *getFunction(const QString &name);
    static void *addFunction(const QString &name, size_t offset, const QString &sectionName = "text");

    // MemoryMap interface
protected:
    bool initialize() override;
    bool deinitialize() override;

    bool isInitialized() override;

private:
    static bool s_isInitialized;
};
} // namespace Compat

#endif // COMPAT_FUNCTIONSMEMORYMAP_H
