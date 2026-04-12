#include "QtInspector.h"
#include "MainWindow.h"
#include "ShortcutManager.h"

#include <QCoreApplication>
#include <QMutex>

static QtMessageHandler s_nativeMessageHandler = nullptr;

void alternativeQtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (s_nativeMessageHandler)
        s_nativeMessageHandler(type, context, msg);

    static QMutex mutex;
    QMutexLocker locker(&mutex);

    // don't display internal Qt Debug Messages
    if (type != QtDebugMsg)
        fprintf(stdout, "%s\n", msg.toStdString().c_str());
}

void PM::initializeQtInspector()
{
    s_nativeMessageHandler = qInstallMessageHandler(alternativeQtMessageHandler);

    static MainWindow mainWindow;
    mainWindow.show();

    static QtInspector::ShortcutManager shortcutManager;
    shortcutManager.registerSequence(mainWindow.globalDisplayKeySequence(),
                                     []()
                                     {
                                         // Display auxiliary window from anywhere
                                         mainWindow.show();
                                     });

    qApp->installEventFilter(&shortcutManager);
}
