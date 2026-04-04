#ifndef MAINTHREADHIJACKER_H
#define MAINTHREADHIJACKER_H

#include <QQmlEngine>
#include <QQuickWindow>
#include <QWindow>

class MainThreadHijacker : public QObject
{
    Q_OBJECT
public:
    MainThreadHijacker();

public:
    void startMonitoringApplicationWindows();

    static QWindow *mainWindow();
    static QWindow *splashScreenWindow();
    static QQmlEngine *mainWindowEngine();
    static QQmlEngine *splashScreenEngine();

    static QObject *createQmlObject(const QString &uri, int versionMajor, int versionMinor, const QString &qmlName, QQmlEngine *engine = nullptr);

signals:
    void mainWindowDisplayed();
    void splashScreenCreated();

    void windowCreated(QWindow *window);
    void windowDestroyed(QWindow *window);

public slots:
    void initialize();
    void initializeMods();
    void showComponentsTest();
    void showAuxiliaryWindow();

private slots:
    void onMainWindowDisplayed();
    void onSplashScreenCreated();
    void onMod2MenuItemClicked();

    void onWindowCreated(QWindow *window);
    void onWindowDestroyed(QWindow *window);
};

#endif // MAINTHREADHIJACKER_H
