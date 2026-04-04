#include "QtObjectsMemoryMap.h"

#include <ExecutableLoader/ExecutableLoader_p.h>
#include <MemoryMaps/MemoryMap_p.h>

#include <QObjectViewer/ObjectLocator/ObjectLocator.h>
#include <QObjectViewer/PointerChecker/QMetaObjectPointerChecker.h>

namespace
{
const char QOBJECT_MEMORY_MAP_TAG[] = "QObject";
const char QMETA_OBJECT_MEMORY_MAP_TAG[] = "QMetaObject";
const char QQML_SINGLETON_MEMORYMAP_TAG[] = "QQmlSingleton";
} // namespace

using namespace Compat;

bool QtObjectsMemoryMap::s_isInitialized = false;

QMetaObject *QtObjectsMemoryMap::getMetaObject(const QString &name)
{
    MemoryMapPrivate::MemoryMapEntry *entry = MemoryMapPrivate::getAddressByName(name.toStdString());

    if (entry == nullptr)
        return nullptr;

    if (!MemoryMapPrivate::satisfiesFilter(*entry, {"", {QMETA_OBJECT_MEMORY_MAP_TAG}}))
        return nullptr;

    return reinterpret_cast<QMetaObject *>(entry->address);
}

QMetaObject *QtObjectsMemoryMap::addMetaObject(const QMetaObject *metaObject)
{
    if (!metaObject)
        return nullptr;

    if (!QMetaObjectPointerChecker::isValid(metaObject))
        return nullptr;

    const QMetaObject *superClass = metaObject;

    // add the inheritance hierarchy of the given meta-object
    do
    {
        ::MemoryMap::addAddress(superClass->className(), superClass, MemoryMap::getModuleFileNameFromAddress(superClass),
                                {QMETA_OBJECT_MEMORY_MAP_TAG});

        superClass = superClass->superClass();
    } while (superClass != nullptr);

    return const_cast<QMetaObject *>(metaObject);
}

bool QtObjectsMemoryMap::initialize()
{
    if (!MemoryMap::initialize())
        return false;

    initializeMetaObjects();
    initializeNamedObjects();

    s_isInitialized = true;
    return true;
}

bool QtObjectsMemoryMap::deinitialize()
{
    if (!MemoryMap::deinitialize())
        return false;

    s_isInitialized = false;
    return true;
}

bool QtObjectsMemoryMap::isInitialized()
{
    return s_isInitialized;
}

void QtObjectsMemoryMap::initializeMetaObjects()
{
    // TODO:: Call addMetaObject() here
}

void QtObjectsMemoryMap::initializeNamedObjects()
{
    // TODO:: Call addObject() here
}

void QtObjectsMemoryMap::initializeQmlSingletons()
{
    // TODO:: Call addQmlSingleton() here
}

QMetaObject *QtObjectsMemoryMap::addMetaObject(const QString &sectionName, size_t offset)
{
    QMetaObject *result = offsetToPointer<QMetaObject>(sectionName, offset);

    return addMetaObject(result);
}

void *QtObjectsMemoryMap::getObject(const QString &name)
{
    MemoryMapPrivate::MemoryMapEntry *entry = MemoryMapPrivate::getAddressByName(name.toStdString());

    if (entry == nullptr)
        return nullptr;

    if (!MemoryMapPrivate::satisfiesFilter(*entry, {"", {QOBJECT_MEMORY_MAP_TAG}}))
        return nullptr;

    return reinterpret_cast<void *>(entry->address);
}

void *QtObjectsMemoryMap::addObject(const QString &objectName, size_t offset, const QString &sectionName)
{
    void *result = offsetToPointer<void>(sectionName, offset);

    if (!result)
        return nullptr;

    ::MemoryMap::addAddress(objectName.toStdString(), result, MemoryMap::getModuleFileNameFromAddress(result), {QOBJECT_MEMORY_MAP_TAG});

    return result;
}

QObject *QtObjectsMemoryMap::getQmlSingleton(const QString &singletonName)
{
    MemoryMapPrivate::MemoryMapEntry *entry = MemoryMapPrivate::getAddressByName(singletonName.toStdString());

    if (entry == nullptr)
        return nullptr;

    if (!MemoryMapPrivate::satisfiesFilter(*entry, {"", {QQML_SINGLETON_MEMORYMAP_TAG, QOBJECT_MEMORY_MAP_TAG}}))
        return nullptr;

    return reinterpret_cast<QObject *>(entry->address);
}

QObject *QtObjectsMemoryMap::addQmlSingleton(const QString &uri, int versionMajor, int versionMinor, const QString &qmlName, QQmlEngine *engine)
{
    return addQmlSingleton(uri + "." + qmlName, uri, versionMajor, versionMinor, qmlName, engine);
}

QObject *QtObjectsMemoryMap::addQmlSingleton(const QString &singletonName, const QString &uri, int versionMajor, int versionMinor,
                                             const QString &qmlName, QQmlEngine *engine)
{
    QObject *result = ObjectLocator::getQmlSingleton(uri, versionMajor, versionMinor, qmlName, engine);

    if (result == nullptr)
        return nullptr;

    ::MemoryMap::addAddress(singletonName.toStdString(), result, MemoryMap::getModuleFileNameFromAddress(result),
                            {QQML_SINGLETON_MEMORYMAP_TAG, QOBJECT_MEMORY_MAP_TAG});

    // add the meta-object of this singleton to the memory map
    addMetaObject(result->metaObject());

    return result;
}
