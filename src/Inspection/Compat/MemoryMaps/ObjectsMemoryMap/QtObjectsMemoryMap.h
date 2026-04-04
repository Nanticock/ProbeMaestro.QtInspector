#ifndef COMPAT_QTOBJECTSMEMORYMAP_H
#define COMPAT_QTOBJECTSMEMORYMAP_H

#include <Compat/MemoryMaps/MemoryMap.h>

#include <QMetaObject>
#include <QQmlEngine>

namespace Compat
{
/// @deprecated
struct QtObjectsMemoryMap : public MemoryMap
{
public:
    static void initializeQmlSingletons();

    static QMetaObject *getMetaObject(const QString &name);
    static QMetaObject *addMetaObject(const QMetaObject *metaObject);
    static QMetaObject *addMetaObject(const QString &sectionName, size_t offset);

    template <typename T>
    static T getObject(const QString &name)
    {
        return reinterpret_cast<T>(getObject(name));
    }
    static void *getObject(const QString &name);
    static void *addObject(const QString &objectName, size_t offset, const QString &sectionName = "text");

    static QObject *getQmlSingleton(const QString &singletonName);
    static QObject *addQmlSingleton(const QString &uri, int versionMajor, int versionMinor, const QString &qmlName, QQmlEngine *engine = nullptr);
    static QObject *addQmlSingleton(const QString &singletonName, const QString &uri, int versionMajor, int versionMinor, const QString &qmlName,
                                    QQmlEngine *engine = nullptr);

    // MemoryMap interface
protected:
    bool initialize() override;
    bool deinitialize() override;

    bool isInitialized() override;

protected:
    void initializeMetaObjects();
    void initializeNamedObjects();

private:
    static bool s_isInitialized;
};
} // namespace Compat

#endif // COMPAT_QTOBJECTSMEMORYMAP_H
