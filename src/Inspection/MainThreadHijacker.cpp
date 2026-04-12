#include "MainThreadHijacker.h"

#include <MainWindow.h>
#include <QObjectViewer/ObjectLocator/ObjectLocator.h>
#include <compat_Qt.h>

#include <QGuiApplication>
#include <QPointer>
#include <QQmlComponent>
#include <QTimer>

namespace
{
constexpr char MAIN_WINDOW_ID[] = "applicationWindow";
constexpr char SPLASH_SCREEN_WINDOW_ID[] = "_splashScreen";
} // namespace

// important windows
static QWindow *s_mainWindow = nullptr;
static QWindow *s_splashScreenWindow = nullptr;

static QSet<QWindow *> s_applicationWindows;
static QPointer<QTimer> s_windowsMonitoringTimer;

MainThreadHijacker::MainThreadHijacker()
{
    connect(this, &MainThreadHijacker::mainWindowDisplayed, this, &MainThreadHijacker::onMainWindowDisplayed);
    connect(this, &MainThreadHijacker::splashScreenCreated, this, &MainThreadHijacker::onSplashScreenCreated);

    connect(this, &MainThreadHijacker::windowCreated, this, &MainThreadHijacker::onWindowCreated);
    connect(this, &MainThreadHijacker::windowDestroyed, this, &MainThreadHijacker::onWindowDestroyed);
}

void MainThreadHijacker::startMonitoringApplicationWindows()
{
    if (s_windowsMonitoringTimer == nullptr)
    {
        s_windowsMonitoringTimer = new QTimer(this);
        s_windowsMonitoringTimer->setInterval(1);

        connect(s_windowsMonitoringTimer.data(), &QTimer::timeout, this,
                [this]()
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
                });
    }

    // if the application window monitoring Timer is already up, then nothing to be done
    if (s_windowsMonitoringTimer->isActive())
        return;

    s_windowsMonitoringTimer->start();
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
    mod2MenuItem->setProperty("text", "Show auxiliary window");
    // FIXME: find a way to get the name of the method if i have its function pointer
    QObject::connect(mod2MenuItem, ObjectLocator::getMetaMethodByName(mod2MenuItem, "triggered"), this,
                     ObjectLocator::getMetaMethodByName(this, "onMod2MenuItemClicked"));

    // TODO: set the _action property for the new menu item to make a connection, this is more convenient and cleaner

    // append the created items to the mods menu
    QQmlListReference modsMenuItemsList(modsMenu, "items");
    modsMenuItemsList.append(mod2MenuItem);
}

void MainThreadHijacker::onMainWindowDisplayed()
{
    initializeMods();
}

void MainThreadHijacker::onSplashScreenCreated()
{
}

void MainThreadHijacker::onMod2MenuItemClicked()
{
    // FIXME: add the option to show auxiliary window
}

void MainThreadHijacker::onWindowCreated(QWindow *window)
{
    const QString windowId = ObjectLocator::getQmlObjectId(window);

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
    // FIXME: replace this with QPointer usage
    const QString windowId = ObjectLocator::getQmlObjectId(window);

    if (windowId == MAIN_WINDOW_ID)
        s_mainWindow = nullptr;

    if (windowId == SPLASH_SCREEN_WINDOW_ID)
        s_splashScreenWindow = nullptr;
}
