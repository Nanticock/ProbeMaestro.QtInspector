#include "ObjectLocator.h"

#include "CacheRepo.h"

#include <Compat/MemoryMaps/ObjectsMemoryMap/QtObjectsMemoryMap.h>
#include <QObjectViewer/PointerChecker/QMetaObjectPointerChecker.h>
#include <QObjectViewer/PointerChecker/QObjectPointerChecker.h>
#include <compat_Qt.h>

#include <QApplication>
#include <QtQml>

static CacheRepo s_pathsCacheRepo;
static CacheRepo s_windowsCacheRepo;

static bool s_enableCaching = true;

static QHash<const QMetaObject *, QHash<QString, QMetaMethod>> s_metaObjectsMetaMethodNamesMap;
static QHash<const QMetaObject *, QHash<QString, QMetaProperty>> s_metaObjectsMetaPropertyNamesMap;

void ObjectLocator::clearCache()
{
    s_pathsCacheRepo.clear();
    s_windowsCacheRepo.clear();
}

bool ObjectLocator::cachingIsEnabled()
{
    return s_enableCaching;
}

void ObjectLocator::enableCaching(bool value)
{
    if (s_enableCaching == value)
        return;

    s_enableCaching = value;

    // in case the caching was disabled then renabled, clear the old cache
    if (s_enableCaching)
        clearCache();
}

bool ObjectLocator::isValidQmlId(const QString &id)
{
    // Refer to:
    // https://sl.bing.net/c3QqPysjXeS

    // Define a regular expression that matches valid QML ids
    QRegularExpression re("^[_a-z][_a-zA-Z0-9]*$");
    // Check if the input matches the regular expression
    return re.match(id).hasMatch();
}

QString ObjectLocator::getParentPath(const QString &path)
{
    return QFileInfo(path).dir().path();
}

QString ObjectLocator::getAbsolutePath(const QString &path)
{
    // if the path is already absolute, then we have nothing to do
    if (path.startsWith('/'))
        return path;

    return '/' + path;
}

QString ObjectLocator::getRelativePath(const QString &baseDir, const QString &path)
{
    return QDir(getAbsolutePath(baseDir)).relativeFilePath(getAbsolutePath(path));
}

QString ObjectLocator::getPathFirstUnit(const QString &path, QString *remainingPath)
{
    QString cleanPath = getRelativePath("/", path);

    QStringList pathComponents = cleanPath.split('/');
    QString result = pathComponents.first();

    if (remainingPath != nullptr)
        // if the path could be split to more than one component,
        // return the remainder of the path, otherwise return nothing
        *remainingPath = pathComponents.count() > 1 ? cleanPath.right(cleanPath.length() - (result.length() + 1)) : "";

    return result;
}

bool ObjectLocator::indexAllAvailablePointers()
{
    QSet<QObject *> visitedItems;

    QList<QQuickWindow *> qmlWindows = getQmlWindows();

    for (QQuickWindow *window : qAsConst(qmlWindows))
        indexAllAvailablePointers(window);

    return true;
}

bool ObjectLocator::indexAllAvailablePointers(QObject *rootObject)
{
    static const QSet<QString> forbiddenMetaObjects = {};

    static QString searchString = "";
    static const QMetaObject *searchMetaObject = getQMetaObjectFromTypeName(searchString);

    if (!QObjectPointerChecker::isValid(rootObject))
        return false;

    static QSet<QObject *> visitedObjects;

    if (visitedObjects.contains(rootObject))
        return true;

    const QMetaObject *metaObject = rootObject->metaObject();
    Compat::QtObjectsMemoryMap::addMetaObject(metaObject);

    if (searchMetaObject == nullptr)
    {
        if (metaObject->className() == searchString)
            searchMetaObject = metaObject;
    }

    if (metaObject == searchMetaObject)
        qInfo() << rootObject;

    visitedObjects << rootObject;

    if (visitedObjects.count() % 10000 == 0)
        qInfo() << visitedObjects.count();

    qInfo() << " MetaObject:" << metaObject->className();

    if (!forbiddenMetaObjects.contains(metaObject->className()))
    {
        // check the object properties as well to get any pointer to any useful values
        for (int i = 0; i < metaObject->propertyCount(); i++)
        {
            QMetaProperty property = metaObject->property(i);

            if (!property.isValid())
                continue;

            if (property.name() == nullptr)
                continue;

            QVariant propertyVariantValue = rootObject->property(property.name());
            QObject *propertyValue = propertyVariantValue.value<QObject *>();

            // in the case of values that cannot be get directly as QObjects (such as values of type QVariant)
            // we will have to do it the long way
            if (propertyValue == nullptr && propertyVariantValue.convert(qMetaTypeId<QObject *>()))
                propertyValue = propertyVariantValue.value<QObject *>();

            // only values that can be converted to a QObject are of any interest to us
            if (propertyValue == nullptr)
                continue;

            // if current object inherits the same meta object given by the user, then we add it to the result
            if (propertyValue->metaObject()->inherits(metaObject))
                indexAllAvailablePointers(propertyValue);
        }
    }

    QList<QObject *> childrenList = rootObject->children();

    if (childrenList.isEmpty())
        return false;

    for (QObject *child : childrenList)
        indexAllAvailablePointers(child);

    return true;
}

QUrl ObjectLocator::getQmlObjectSourceFilePath(QObject *object)
{
    QQmlContext *context = qmlContext(object);

    if (context == nullptr)
        return QUrl();

    return context->baseUrl();
}

QObjectList ObjectLocator::searchForQmlObjectsOfType(const QMetaType &metaType)
{
    return searchForQmlObjectsOfType(metaType.metaObject());
}

QObjectList ObjectLocator::searchForQmlObjectsOfType(const QMetaObject *metaObject)
{
    return searchForQmlObjectsOfType(metaObject->className(), true);
}

QObjectList ObjectLocator::searchForQmlObjectsOfType(const QString &typeName, bool pointerOfTypeIsType)
{
    // some metaObjects cause the application to crash when we try to access some of their properties, the name of such metaObjects can be added to
    // this set and the search function would automatically ignore looking up for objects referenced in the propertirs of any of these objects
    static const QSet<QString> forbiddenMetaObjects = {""};

    QObjectList result;

    // TODO: move the implementation of this method outside the body of this function
    auto inheritsType = [pointerOfTypeIsType](const QMetaObject *metaObject, const QString &typeName)
    {
        QString pointerTypeName = typeName + " *";

        if (typeName == metaObject->className())
            return true;

        if (pointerOfTypeIsType && pointerTypeName == metaObject->className())
            return true;

        const QMetaObject *superClassType = metaObject->superClass();

        while (superClassType)
        {
            if (typeName == superClassType->className())
                return true;

            if (pointerOfTypeIsType && pointerTypeName == superClassType->className())
                return true;

            superClassType = superClassType->superClass();
        }

        return false;
    };

    QList<QQuickWindow *> qmlWindows = getQmlWindows();

    QSet<QObject *> visitedItems;
    for (QQuickWindow *window : qAsConst(qmlWindows))
    {
        if (window == nullptr)
            continue;

        QObjectList childrenList = window->children();

        // we are using a QVector indtead of a QList because it has constant access time
        QVector<QObject *> windowChildren = PM::internal::createQVector<QObject *>(childrenList.begin(), childrenList.end());

        for (int i = 0; i < windowChildren.count(); i++)
        {
            // when running in the main thread we make sure not to block the application UI
            if (QThread::currentThread() == QApplication::instance()->thread())
                QApplication::processEvents();

            if (QThread::currentThread()->isInterruptionRequested())
                return result;

            QObject *child = windowChildren[i];

            if (visitedItems.contains(child))
                continue;

            // we do that because the objects that we get through scanning the values of properties may have parents that we don't know about
            if (child->parent() != nullptr && !visitedItems.contains(child->parent()))
                windowChildren << child->parent();

            QObjectList childChildren = child->children();
            // append the children of the current object to the window children vector, and the visited items set
            windowChildren << PM::internal::createQVector<QObject *>(childChildren.begin(), childChildren.end());

            // if current object inherits the same meta object given by the user, then we add it to the result
            if (inheritsType(child->metaObject(), typeName))
                result << child;

            const QMetaObject *metaObject = child->metaObject();

            // search only the properties of metaObjects that are not in the forbidden list
            if (!forbiddenMetaObjects.contains(metaObject->className()))
            {
                // iterate over the object properties and search for values that match what we are looking for
                for (int j = 0; j < child->metaObject()->propertyCount(); j++)
                {
                    QMetaProperty property = child->metaObject()->property(j);

                    QVariant propertyVariantValue = child->property(property.name());
                    QObject *propertyValue = propertyVariantValue.value<QObject *>();

                    // in the case of values that cannot be get directly as QObjects (such as values of type QVariant)
                    // we will have to do it the long way
                    if (propertyValue == nullptr && propertyVariantValue.convert(qMetaTypeId<QObject *>()))
                        propertyValue = propertyVariantValue.value<QObject *>();

                    // only values that can be converted to a QObject are of any interest to us
                    if (propertyValue == nullptr)
                        continue;

                    // if we visited this object before then no need to process it again
                    if (visitedItems.contains(propertyValue))
                        continue;

                    windowChildren << propertyValue;
                }
            }

            visitedItems << child;
        }
    }

    qInfo() << "ObjectLocator::searchForQmlObjectsOfType"
            << "visitedObjects =" << visitedItems.count();
    return result;
}

QList<QMetaProperty> ObjectLocator::getObjectPropertiesOfType(const QObject *object, const QMetaType &metaType)
{
    return getObjectPropertiesOfType(object, metaType.metaObject());
}

QList<QMetaProperty> ObjectLocator::getObjectPropertiesOfType(const QObject *object, const QMetaObject *metaObject)
{
    QList<QMetaProperty> result;

    if (object == nullptr)
        return result;

    for (int i = 0; i < object->metaObject()->propertyCount(); i++)
    {
        QMetaProperty property = object->metaObject()->property(i);

        QVariant propertyVariantValue = object->property(property.name());
        QObject *propertyValue = propertyVariantValue.value<QObject *>();

        // in the case of values that cannot be get directly as QObjects (such as values of type QVariant)
        // we will have to do it the long way
        if (propertyValue == nullptr && propertyVariantValue.convert(qMetaTypeId<QObject *>()))
            propertyValue = propertyVariantValue.value<QObject *>();

        // only values that can be converted to a QObject are of any interest to us
        if (propertyValue == nullptr)
            continue;

        // if current object inherits the same meta object given by the user, then we add it to the result
        if (propertyValue->metaObject()->inherits(metaObject))
            result << property;
    }

    return result;
}

QList<QMetaProperty> ObjectLocator::getObjectPropertiesOfType(const QObject *object, const QString &typeName, bool pointerOfTypeIsType)
{
    return getObjectPropertiesOfType(object, getQMetaObjectFromTypeName(typeName, pointerOfTypeIsType));
}

QString ObjectLocator::getQmlObjectPath(QObject *object, bool relativeToTopMostWindow)
{
    return getQmlObjectPath(object, qmlContext(object), relativeToTopMostWindow);
}

QString ObjectLocator::getQmlObjectPath(QObject *object, QQmlContext *context, bool relativeToTopMostWindow)
{
    if (object == nullptr)
        return "";

    if (cachingIsEnabled() && s_pathsCacheRepo.hasObject(object))
        return s_pathsCacheRepo.getObjectFirstString(object);

    auto objectToPathUnit = [](QQmlContext *context, QObject *object, bool *success = nullptr)
    {
        static const QString pathUnitTemplate("/%1");

        QString result = pathUnitTemplate.arg(getQmlObjectId(object, context));

        if (success != nullptr)
            *success = true;

        // in case of valid result path
        if (result.length() > 1)
            return result;

        // in the case of no valid result path
        if (success != nullptr)
            *success = false;

        return QString();
    };

    bool pathUnitIsValid = true;
    QString result = objectToPathUnit(context, object, &pathUnitIsValid);

    if (!pathUnitIsValid)
        return "";

    QObject *topMostWindow = object->isWindowType() ? object : nullptr;

    QObject *currentParent = object->parent();
    while (currentParent)
    {
        result = objectToPathUnit(qmlContext(currentParent), currentParent, &pathUnitIsValid) + result;

        if (!pathUnitIsValid)
            result = objectToPathUnit(context, currentParent, &pathUnitIsValid) + result;

        if (!pathUnitIsValid)
            break;

        if (currentParent->isWindowType())
            topMostWindow = currentParent;

        currentParent = currentParent->parent();
    }

    result = result.remove(0, 1);

    // cache absolute path
    if (cachingIsEnabled())
        s_pathsCacheRepo.add(object, result);

    if (!relativeToTopMostWindow)
        return result;

    if (topMostWindow == nullptr || topMostWindow == object)
        return result;

    // return the path relative to the topmost window
    QString topMostPath = getQmlObjectPath(topMostWindow, false);
    topMostPath = getParentPath(topMostPath);

    result = getRelativePath(topMostPath, result);

    // cache relative path
    if (cachingIsEnabled())
        s_pathsCacheRepo.add(object, result);

    return result;
}

QObject *ObjectLocator::getQmlObjectByPath(const QString &path)
{
    // TODO: find a way to not make this a separate implementation
    // different from the
    // ObjectLocator::getQmlObjectByPath(const QString &path, QQmlContext *context)

    if (path.isEmpty())
        return nullptr;

    if (cachingIsEnabled() && s_pathsCacheRepo.hasString(path))
        return s_pathsCacheRepo.getObject<QObject *>(path);

    // get all qml windows that has the first name in the path
    QString remainingPath;
    QString rootObjectName = getPathFirstUnit(path, &remainingPath);

    QList<QQuickWindow *> qmlWindows = getQmlWindows(rootObjectName);

    for (QQuickWindow *window : qAsConst(qmlWindows))
    {
        QObject *result = getQmlObjectByPath(remainingPath, window);

        if (result)
            return result;
    }

    return nullptr;
}

QObject *ObjectLocator::getQmlObjectByPath(const QString &path, QQmlContext *context)
{
    if (path.isEmpty())
        return nullptr;

    if (cachingIsEnabled() && s_pathsCacheRepo.hasString(path))
        return s_pathsCacheRepo.getObject<QObject *>(path);

    // get all qml windows that has the first name in the path
    QString remainingPath;
    QString rootObjectName = getPathFirstUnit(path, &remainingPath);

    QList<QQuickWindow *> qmlWindows = getQmlWindows(rootObjectName, context);

    if (remainingPath.isEmpty())
        return qmlWindows.first();

    for (QQuickWindow *window : qAsConst(qmlWindows))
    {
        QObject *result = getQmlObjectByPath(remainingPath, window, context);

        if (result)
            return result;
    }

    return nullptr;
}

QObject *ObjectLocator::getQmlObjectByPath(const QString &path, QObject *rootObject)
{
    return getQmlObjectByPath(path, rootObject, qmlContext(rootObject));
}

QObject *ObjectLocator::getQmlObjectByPath(const QString &path, QObject *rootObject, QQmlContext *context)
{
    // TODO: add a check for QML id validity

    if (path.isEmpty())
        return nullptr;

    if (rootObject == nullptr)
        return nullptr;

    if (cachingIsEnabled() && s_pathsCacheRepo.hasString(path))
        return s_pathsCacheRepo.getObject<QObject *>(path);

    QString remainingPath;
    QString rootObjectId = getPathFirstUnit(path, &remainingPath);

    QObject *result = getQmlChildById(rootObjectId, rootObject, context);

    // if couldn't get any child by this id in the current context,
    // then we try again by using each object's context
    if (result == nullptr)
        result = getQmlChildById(rootObjectId, rootObject, nullptr);

    if (result == nullptr)
        return nullptr;

    if (remainingPath.trimmed().isEmpty())
        return result;

    return getQmlObjectByPath(remainingPath, result, context);
}

QObject *ObjectLocator::getQmlChildById(const QString &id, QObject *rootObject)
{
    return getQmlChildById(id, rootObject, qmlContext(rootObject));
}

QObject *ObjectLocator::getQmlChildById(const QString &id, QObject *rootObject, QQmlContext *context)
{
    if (rootObject == nullptr)
        return nullptr;

    for (QObject *child : rootObject->children())
    {
        QString childId = context ? getQmlObjectId(child, context) : getQmlObjectId(child);

        //        if (cachingIsEnabled())
        //            s_namesCacheRepo.add(child, childId);

        if (childId == id)
            return child;
    }

    return nullptr;
}

QObject *ObjectLocator::getChildByPropertyValue(QObject *rootObject, const QString &propertyName, const QVariant &propertyValue)
{
    if (rootObject == nullptr)
        return nullptr;

    if (propertyName.isEmpty())
        return nullptr;

    const char *propertyNameData = propertyName.toUtf8().data();
    for (QObject *child : rootObject->children())
    {
        if (child->metaObject()->indexOfProperty(propertyNameData) == -1)
            continue;

        if (child->property(propertyNameData) == propertyValue)
            return child;

        if (!child->dynamicPropertyNames().contains(propertyName.toUtf8()))
            continue;

        if (child->property(propertyNameData) == propertyValue)
            return child;
    }

    return nullptr;
}

QObject *ObjectLocator::getQObjectFromAddress(void *address)
{
    if (!QObjectPointerChecker::isValid(address))
        return nullptr;

    return reinterpret_cast<QObject *>(address);
}

QObject *ObjectLocator::getQObjectFromAddress(size_t address)
{
    return getQObjectFromAddress(reinterpret_cast<void *>(address));
}

const QMetaObject *ObjectLocator::getQMetaObjectFromAddress(void *address)
{
    if (!QMetaObjectPointerChecker::isValid(address))
        return nullptr;

    return reinterpret_cast<const QMetaObject *>(address);
}

const QMetaObject *ObjectLocator::getQMetaObjectFromAddress(size_t address)
{
    return getQMetaObjectFromAddress(reinterpret_cast<void *>(address));
}

int ObjectLocator::getQMetaTypeIdFromTypeName(const QString &typeName, bool pointerOfTypeIsType)
{
    int typeId = QMetaType::type(typeName.toUtf8());

    // if couldn't find the type, try searching for its pointer type
    if (typeId == QMetaType::UnknownType && pointerOfTypeIsType)
        return getQMetaTypeIdFromTypeName((typeName + " *"), false);

    if (typeId == QMetaType::UnknownType)
        return typeId;

    return typeId;
}

const QMetaObject *ObjectLocator::getQMetaObjectFromTypeName(const QString &typeName, bool pointerOfTypeIsType)
{
    const QMetaObject *metaObject = QMetaType(getQMetaTypeIdFromTypeName(typeName, pointerOfTypeIsType)).metaObject();

    if (metaObject)
        return metaObject;

    // if we couldn't find the given MetaObject using qt itself, search for it in the ObjectsMemoryMap
    metaObject = Compat::QtObjectsMemoryMap::getMetaObject(typeName);

    if (metaObject)
        return metaObject;

    // if we couldn't locate the object in the ObjectsMemoryMap, try searching for it's pointer in the ObjectsMemoryMap
    if (pointerOfTypeIsType)
        return Compat::QtObjectsMemoryMap::getMetaObject(typeName + " *");

    return nullptr;
}

QString ObjectLocator::getQmlObjectId(QObject *object)
{
    return getQmlObjectId(object, qmlContext(object));
}

QString ObjectLocator::getQmlObjectId(QObject *object, QQmlContext *context)
{
    // TODO: find a way to utilize caching in this function

    // Refer to: https://sl.bing.net/b4Q0J8RqwzQ
    if (object == nullptr)
        return "";

    if (context == nullptr)
        context = qmlContext(object);

    if (context == nullptr)
        return "";

    return context->nameForObject(object);
}

QObject *ObjectLocator::getQmlObjectById(const QString &id)
{
    //    if (cachingIsEnabled() && s_namesCacheRepo.hasString(id))
    //        return s_namesCacheRepo.getObject<QObject *>(id);

    QWindowList allWindows = QApplication::allWindows();

    for (QWindow *window : qAsConst(allWindows))
    {
        QObject *object = getQmlObjectById(id, window);

        if (!object)
            continue;

        //        if (cachingIsEnabled())
        //            s_namesCacheRepo.add(object, id);

        return object;
    }

    return nullptr;
}

QObject *ObjectLocator::getQmlObjectById(const QString &id, QObject *rootObject)
{
    if (rootObject == nullptr)
        return nullptr;

    QQmlContext *context = qmlContext(rootObject);

    return getQmlObjectById(id, context);
}

QObject *ObjectLocator::getQmlObjectById(const QString &id, QQmlContext *context)
{
    // get a QML object by its id
    //
    // Refer to: https://sl.bing.net/b4Q0J8RqwzQ

    if (context == nullptr)
        return nullptr;

    QVariant result = context->contextProperty(id);

    if (!result.isValid() || !result.canConvert<QObject *>())
        return nullptr;

    return result.value<QObject *>();
}

QQuickWindow *ObjectLocator::getQmlWindow(const QString &nameOrId)
{
    if (cachingIsEnabled() && s_windowsCacheRepo.hasString(nameOrId))
        return s_windowsCacheRepo.getObject<QQuickWindow *>(nameOrId);

    QList<QQuickWindow *> qmlWindows = getQmlWindows(nameOrId);

    if (qmlWindows.isEmpty())
        return nullptr;

    return qmlWindows.first();
}

QList<QQuickWindow *> ObjectLocator::getQmlWindows(QQmlContext *contextFilter)
{
    return getQmlWindows("", contextFilter);
}

QList<QQuickWindow *> ObjectLocator::getQmlWindows(const QString &nameFilter, const QQmlContext *contextFilter)
{
    QList<QQuickWindow *> result;

    QWindowList allWindows = QApplication::allWindows();
    for (QWindow *window : qAsConst(allWindows))
    {
        QQuickWindow *qmlWindow = dynamic_cast<QQuickWindow *>(window);

        if (!qmlWindow)
            continue;

        // if the context filter is set, then discard any window that doesn't belong to the context filter
        if (contextFilter && qmlContext(qmlWindow) != contextFilter)
            continue;

        // if nameFilter isn't empty, then only include windows with the given nameFilter
        if (!nameFilter.isEmpty())
        {
            QString qmlId = getQmlObjectId(qmlWindow);
            QString windowName = qmlWindow->objectName();

            if (cachingIsEnabled())
            {
                s_windowsCacheRepo.add(window, qmlId);
                s_windowsCacheRepo.add(window, windowName);
            }

            if (qmlId != nameFilter && windowName != nameFilter)
                continue;
        }

        result << qmlWindow;
    }

    return result;
}

QObject *ObjectLocator::getQmlSingleton(const QString &singletonName, QQmlEngine *engine)
{
    QObject *result = Compat::QtObjectsMemoryMap::getQmlSingleton(singletonName);

    if (engine && qmlEngine(result) != engine)
        return nullptr;

    return result;
}

QObject *ObjectLocator::getQmlSingleton(const QString &uri, int versionMajor, int versionMinor, const QString &qmlName, QQmlEngine *engine)
{
    // FIXME: search for all available qml engines and use them in the search

    // if (engine == nullptr && MainThreadHijacker::mainWindowEngine())
    //     engine = MainThreadHijacker::mainWindowEngine();

    if (engine == nullptr)
        return nullptr;

    int typeId = qmlTypeId(uri.toStdString().c_str(), 1, 0, qmlName.toStdString().c_str());

    if (typeId == -1)
        return nullptr;

    return engine->singletonInstance<QObject *>(typeId);
}

QMetaMethod ObjectLocator::getMetaMethodByName(const QObject *object, const QString &methodName)
{
    if (object == nullptr)
        return QMetaMethod();

    return getMetaMethodByName(object->metaObject(), methodName);
}

QMetaMethod ObjectLocator::getMetaMethodByName(const QMetaObject *metaObject, const QString &methodName)
{
    if (metaObject == nullptr)
        return QMetaMethod();

    if (s_metaObjectsMetaMethodNamesMap.contains(metaObject))
        return s_metaObjectsMetaMethodNamesMap[metaObject][methodName];

    QMetaMethod result;

    for (int i = 0; i < metaObject->methodCount(); i++)
    {
        QMetaMethod method = metaObject->method(i);

        s_metaObjectsMetaMethodNamesMap[metaObject][method.name()] = method;

        if (method.name() != methodName)
            continue;

        result = method;
    }

    return result;
}

QMetaProperty ObjectLocator::getMetaPropertyByName(const QObject *object, const QString &propertyName)
{
    if (object == nullptr)
        return QMetaProperty();

    return getMetaPropertyByName(object->metaObject(), propertyName);
}

QMetaProperty ObjectLocator::getMetaPropertyByName(const QMetaObject *metaObject, const QString &propertyName)
{
    if (metaObject == nullptr)
        return QMetaProperty();

    if (s_metaObjectsMetaPropertyNamesMap.contains(metaObject))
        return s_metaObjectsMetaPropertyNamesMap[metaObject][propertyName];

    QMetaProperty result;

    for (int i = 0; i < metaObject->propertyCount(); i++)
    {
        QMetaProperty property = metaObject->property(i);

        s_metaObjectsMetaPropertyNamesMap[metaObject][property.name()] = property;

        if (property.name() != propertyName)
            continue;

        result = property;
    }

    return result;
}
