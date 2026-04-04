#ifndef OBJECTLOCATOR_H
#define OBJECTLOCATOR_H

#include <QQmlContext>
#include <QQuickWindow>

struct ObjectLocator
{
    static bool indexAllAvailablePointers();
    static bool indexAllAvailablePointers(QObject *rootObject);

    static QUrl getQmlObjectSourceFilePath(QObject *object);

    static QObjectList searchForQmlObjectsOfType(const QMetaType &metaType);
    static QObjectList searchForQmlObjectsOfType(const QMetaObject *metaObject);
    static QObjectList searchForQmlObjectsOfType(const QString &typeName, bool pointerOfTypeIsType = true);

    static QList<QMetaProperty> getObjectPropertiesOfType(const QObject *object, const QMetaType &metaType);
    static QList<QMetaProperty> getObjectPropertiesOfType(const QObject *object, const QMetaObject *metaObject);
    static QList<QMetaProperty> getObjectPropertiesOfType(const QObject *object, const QString &typeName, bool pointerOfTypeIsType = true);

    static QString getQmlObjectPath(QObject *object, bool relativeToTopMostWindow = true);
    static QString getQmlObjectPath(QObject *object, QQmlContext *context, bool relativeToTopMostWindow = true);

    static QObject *getQmlObjectByPath(const QString &path);
    static QObject *getQmlObjectByPath(const QString &path, QObject *rootObject);
    static QObject *getQmlObjectByPath(const QString &path, QQmlContext *context);
    static QObject *getQmlObjectByPath(const QString &path, QObject *rootObject, QQmlContext *context);

    static QObject *getQmlChildById(const QString &id, QObject *rootObject);
    static QObject *getQmlChildById(const QString &id, QObject *rootObject, QQmlContext *context);
    static QObject *getChildByPropertyValue(QObject *rootObject, const QString &propertyName, const QVariant &propertyValue);

    static QObject *getQObjectFromAddress(void *address);
    static QObject *getQObjectFromAddress(size_t address);

    static const QMetaObject *getQMetaObjectFromAddress(void *address);
    static const QMetaObject *getQMetaObjectFromAddress(size_t address);

    static int getQMetaTypeIdFromTypeName(const QString &typeName, bool pointerOfTypeIsType = true);
    static const QMetaObject *getQMetaObjectFromTypeName(const QString &typeName, bool pointerOfTypeIsType = true);

    static QString getQmlObjectId(QObject *object);
    static QString getQmlObjectId(QObject *object, QQmlContext *context);

    static QObject *getQmlObjectById(const QString &id);
    static QObject *getQmlObjectById(const QString &id, QObject *rootObject);
    static QObject *getQmlObjectById(const QString &id, QQmlContext *context);

    static QQuickWindow *getQmlWindow(const QString &nameOrId);
    static QList<QQuickWindow *> getQmlWindows(QQmlContext *contextFilter);
    static QList<QQuickWindow *> getQmlWindows(const QString &nameFilter = "", const QQmlContext *contextFilter = nullptr);

    static QObject *getQmlSingleton(const QString &singletonName, QQmlEngine *engine = nullptr);
    static QObject *getQmlSingleton(const QString &uri, int versionMajor, int versionMinor, const QString &qmlName, QQmlEngine *engine = nullptr);

    static QMetaMethod getMetaMethodByName(const QObject *object, const QString &methodName);
    static QMetaMethod getMetaMethodByName(const QMetaObject *metaObject, const QString &methodName);

    static QMetaProperty getMetaPropertyByName(const QObject *object, const QString &propertyName);
    static QMetaProperty getMetaPropertyByName(const QMetaObject *metaObject, const QString &propertyName);

    // cache functions
    static void clearCache();

    static bool cachingIsEnabled();
    static void enableCaching(bool value);

    // misc functions
    static bool isValidQmlId(const QString &id);

    static QString getParentPath(const QString &path);
    static QString getAbsolutePath(const QString &path);
    static QString getRelativePath(const QString &baseDir, const QString &path);
    static QString getPathFirstUnit(const QString &path, QString *remainingPath = nullptr);
};

#endif // OBJECTLOCATOR_H
