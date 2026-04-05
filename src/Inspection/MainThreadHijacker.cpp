#include "MainThreadHijacker.h"

#include <Compat/MemoryMaps/ObjectsMemoryMap/QtObjectsMemoryMap.h>
#include <MainWindow.h>
#include <QObjectViewer/ObjectLocator/ObjectLocator.h>
#include <compat_Qt.h>

#include <QDebug>
#include <QGuiApplication>
#include <QMessageBox>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlListProperty>
#include <QThread>
#include <QWindow>

#include <Windows.h>

#define MAIN_WINDOW_ID "applicationWindow"
#define SPLASH_SCREEN_WINDOW_ID "_splashScreen"

static MainWindow *s_auxiliaryWindow = nullptr;

// important windows
static QWindow *s_mainWindow = nullptr;
static QWindow *s_splashScreenWindow = nullptr;

static QSet<QWindow *> s_applicationWindows;
static QThread *s_windowsMonitoringThread = nullptr;

MainThreadHijacker::MainThreadHijacker()
{
    connect(this, &MainThreadHijacker::mainWindowDisplayed, this, &MainThreadHijacker::onMainWindowDisplayed, Qt::QueuedConnection);
    connect(this, &MainThreadHijacker::splashScreenCreated, this, &MainThreadHijacker::onSplashScreenCreated, Qt::QueuedConnection);

    connect(this, &MainThreadHijacker::windowCreated, this, &MainThreadHijacker::onWindowCreated, Qt::QueuedConnection);
    connect(this, &MainThreadHijacker::windowDestroyed, this, &MainThreadHijacker::onWindowDestroyed, Qt::QueuedConnection);
}

void MainThreadHijacker::startMonitoringApplicationWindows()
{
    // if the application window monitoring thread is already up, then nothing to be done
    if (s_windowsMonitoringThread != nullptr)
        return;

    s_windowsMonitoringThread = QThread::create(
        [this]()
        {
            while (true)
            {
                for (QWindow *window : QGuiApplication::allWindows())
                {
                    if (s_applicationWindows.contains(window))
                        continue;

                    s_applicationWindows << window;
                    emit windowCreated(window);

                    // remove the window from the application windows list when it gets destroyed
                    QObject::connect(
                        window, &QWindow::destroyed, window,
                        [this, window](QObject *)
                        {
                            s_applicationWindows.remove(window);
                            emit windowDestroyed(window);
                        },
                        Qt::QueuedConnection);
                }

                QThread::usleep(1);
            }
        });

    s_windowsMonitoringThread->start();
}

QWindow *MainThreadHijacker::mainWindow()
{
    return s_mainWindow;
}

QWindow *MainThreadHijacker::splashScreenWindow()
{
    return s_splashScreenWindow;
}

QQmlEngine *MainThreadHijacker::mainWindowEngine()
{
    if (mainWindow() == nullptr)
        return nullptr;

    // Refer to this: https://sl.bing.net/iy3MTdzLOvc
    return qmlEngine(mainWindow());
}

QQmlEngine *MainThreadHijacker::splashScreenEngine()
{
    if (splashScreenWindow() == nullptr)
        return nullptr;

    return qmlEngine(splashScreenWindow());
}

QObject *MainThreadHijacker::createQmlObject(const QString &uri, int versionMajor, int versionMinor, const QString &qmlName, QQmlEngine *engine)
{
    QString qmlFileData = "import %1 %2.%3\n"
                          "%4{}\n";

    qmlFileData = qmlFileData.arg(uri);
    qmlFileData = qmlFileData.arg(versionMajor);
    qmlFileData = qmlFileData.arg(versionMinor);
    qmlFileData = qmlFileData.arg(qmlName);

    QQmlComponent component(engine);
    component.setData(qmlFileData.toUtf8(), QUrl());

    QObject *result = component.create();

    if (result == nullptr || component.status() != QQmlComponent::Status::Ready)
        qWarning() << component.errorString();

    return result;
}

void MainThreadHijacker::initialize()
{
    startMonitoringApplicationWindows();

    // QmlEngine internal objects registration mechanism, refer to:
    // https://sl.bing.net/jHxW1IB24T6
    // https://sl.bing.net/hE395JxnqfI
    // https://codebrowser.dev/qt5/qtdeclarative/src/qml/qml/qqmlmetatype_p.h.html#QQmlMetaType
    // https://sl.bing.net/kkZLAi2VfXM

    //    QQmlComponent component(activeEngine, "qrc:/AuxiliaryWindow.qml");
    //    component.create();
}

void MainThreadHijacker::initializeMods()
{
    QObject *mainMenuBarPtr = ObjectLocator::getQmlObjectByPath("applicationWindow/g_menuBar");

    if (!mainMenuBarPtr)
        return;

    // create the mods menu
    QObject *modsMenu = createQmlObject("QtQuick.Controls", 1, 2, "Menu", mainWindowEngine());
    modsMenu->setProperty("title", "Mods");

    // add the mods menu to the menu bar of the main window
    // by appending it to the "g_menuBar.menus" property
    QQmlListReference mainMenuBarMenusList(mainMenuBarPtr, "menus");

    if (!mainMenuBarMenusList.isValid() || !mainMenuBarMenusList.canAppend())
        return;

    mainMenuBarMenusList.append(modsMenu);

    // Method 2: create mod menus using the more complicated "createQmlObject" function

    // create the qml menu
    QObject *mod2MenuItem = createQmlObject("QtQuick.Controls", 1, 2, "MenuItem", mainWindowEngine());
    mod2MenuItem->setProperty("text", "Mod2");
    QObject::connect(mod2MenuItem, ObjectLocator::getMetaMethodByName(mod2MenuItem, "triggered"), this,
                     ObjectLocator::getMetaMethodByName(this, "onMod2MenuItemClicked"));

    // TODO: set the _action property for the new menu item to make a connection, this is more convenient and cleaner

    // append the created items to the mods menu
    QQmlListReference modsMenuItemsList(modsMenu, "items");
    modsMenuItemsList.append(mod2MenuItem);
}

void MainThreadHijacker::showAuxiliaryWindow()
{
    if (s_auxiliaryWindow == nullptr)
        s_auxiliaryWindow = new MainWindow();

    s_auxiliaryWindow->show();
}

void MainThreadHijacker::showComponentsTest()
{
    QQmlEngine *engine = mainWindowEngine();

    if (!engine)
        return;

    QQmlComponent component(engine, "qrc:/ComponentsTestWindow.qml");
    component.create();

    if (component.status() == QQmlComponent::Error)
        qWarning() << component.errorString();
}

void MainThreadHijacker::onMainWindowDisplayed()
{
    Compat::QtObjectsMemoryMap::initializeQmlSingletons();

    showAuxiliaryWindow();
    s_auxiliaryWindow->setRootObject(mainWindow());

    initializeMods();
}

QVariant createVariantFromPointer(void *ptr, int typeId)
{
    if (!ptr || typeId == QMetaType::UnknownType)
        return QVariant(); // Return an invalid QVariant if type is unknown

    return QVariant(typeId, ptr);
}

QStringList extractNamespaces(const QString &className)
{
    QStringList parts = className.split("::");
    if (parts.size() > 1)
        parts.removeLast(); // Removes the class name, leaving only the namespace segments
    return parts;
}

QVariant getMethodDefaultValue(QMetaMethod &method, QObject *context)
{
    if (!method.isValid())
        return {};

    if (method.parameterCount() != 0)
        return {};

    QMetaType methodReturnType(method.returnType());
    if (!methodReturnType.isValid())
        return {};

    QVector<char> returnBuffer(methodReturnType.sizeOf());
    methodReturnType.construct(returnBuffer.data());

    QVariant result;
    if (method.invoke(context, Qt::DirectConnection, QGenericReturnArgument(PM::internal::getMetaTypeName(methodReturnType), returnBuffer.data())))
        result = createVariantFromPointer(returnBuffer.data(), PM::internal::getMetaTypeId(methodReturnType));

    methodReturnType.destruct(returnBuffer.data());

    return result;
}

void generateHeaderFile(QObject *obj, const QString &filename)
{
    if (!obj)
    {
        qWarning() << "Invalid QObject pointer";
        return;
    }

    const QMetaObject *metaObj = obj->metaObject();
    QString className(metaObj->className());
    QStringList namespaces = extractNamespaces(className);
    QString pureClassName = className.split("::").last(); // Extracts actual class name

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open file for writing";
        return;
    }

    QTextStream out(&file);
    out << "#pragma once\n\n";
    out << "#include <QObject>\n\n";

    for (const QString &ns : namespaces)
        out << "namespace " << ns << " {\n";

    out << "\nclass " << pureClassName << " : public QObject {\n";
    out << "    Q_OBJECT\n"
        << "public:\n";

    out << "    /* Properties\n\n";
    const int firstNonQObjectPropertyIndex = 1;
    for (int i = firstNonQObjectPropertyIndex; i < metaObj->propertyCount(); ++i)
    {
        QMetaProperty property = metaObj->property(i);
        if (!property.isValid() || !property.isReadable())
            continue;

        QVariant value = property.read(obj);
        out << "    /**\n";
        out << "     * @property " << property.name() << "\n";
        out << "     * @default " << value.toString() << "\n";
        out << "     */\n";
        out << "    Q_PROPERTY(" << property.typeName() << " " << property.name();

        if (property.isWritable())
            out << " READ " << property.name() << " WRITE " << property.name() << ")";
        else
            out << " READ " << property.name() << ")";

        out << "\n\n";
    }
    out << "    */\n";

    // **Method Extraction Logic**
    const int firstNonQObjectFunctionIndex = 5;
    for (int i = firstNonQObjectFunctionIndex; i < metaObj->methodCount(); ++i)
    {
        QMetaMethod method = metaObj->method(i);
        qInfo() << "Processing method:" << method.name();

        if (!method.isValid() || method.access() != QMetaMethod::Public)
            continue;

        out << "    /**\n";

        QVariant defaultValue = getMethodDefaultValue(method, obj);
        if (defaultValue.isValid())
            out << "     * @default " << defaultValue.toString() << "\n";
        else
            out << "     * @note Unable to retrieve default value due to invalid return type.\n";

        out << "     */\n";
        out << "    Q_INVOKABLE " << method.typeName() << " " << method.name() << "(";

        for (int j = 0; j < method.parameterCount(); ++j)
        {
            if (j > 0)
                out << ", ";
            out << PM::internal::getMetaTypeName(method.parameterType(j)) << " param" << j;
        }

        out << ") const\n    {\n";

        if (defaultValue.isValid())
        {
            QString returnValue = defaultValue.toString();
            if (defaultValue.type() == qMetaTypeId<QString>())
                returnValue = '\"' + returnValue + '\"';

            out << "        return " << returnValue << ";\n";
        }
        else
        {
            out << "        return {};\n";
        }

        out << "    }\n\n";
    }

    out << "};\n";

    for (const QString &ns : qAsConst(namespaces))
        out << "} // namespace " << ns << "\n";

    file.flush();
    file.close();
    qDebug() << "Header file generated successfully:" << filename;
}

void MainThreadHijacker::onSplashScreenCreated()
{
}

void MainThreadHijacker::onMod2MenuItemClicked()
{
    qInfo() << "mod2 triggered";
    QMessageBox("Mod2", "Hello from mod 2", QMessageBox::Icon::Information, 0, 0, 0).exec();
}

void MainThreadHijacker::onWindowCreated(QWindow *window)
{
    QString windowId = ObjectLocator::getQmlObjectId(window);

    // handle main window when it becomes visible
    if (windowId == MAIN_WINDOW_ID)
    {
        s_mainWindow = window;

        // emit the mainWindowDisplayed signal
        QObject::connect(
            window, &QWindow::visibleChanged, this,
            [this](bool value)
            {
                if (value)
                    emit mainWindowDisplayed();
            },
            Qt::QueuedConnection);
    }

    // handle splash screen when it gets created
    if (windowId == SPLASH_SCREEN_WINDOW_ID)
    {
        s_splashScreenWindow = window;
        emit splashScreenCreated();
    }
}

void MainThreadHijacker::onWindowDestroyed(QWindow *window)
{
    QString windowId = ObjectLocator::getQmlObjectId(window);

    if (windowId == MAIN_WINDOW_ID)
        s_mainWindow = nullptr;

    if (windowId == SPLASH_SCREEN_WINDOW_ID)
        s_splashScreenWindow = nullptr;
}
