#include "compat_Qt.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
class ActionCallbackStub : public QObject
{
    Q_OBJECT

public:
    explicit ActionCallbackStub(QObject *parent, const std::function<void()> &callback);

public slots:
    void invokeCallback();

private:
    std::function<void()> m_callback;
};

ActionCallbackStub::ActionCallbackStub(QObject *parent, const std::function<void()> &callback) : QObject(parent), m_callback(callback)
{
}

void ActionCallbackStub::invokeCallback()
{
    if (!m_callback)
        return;

    m_callback();
}

#include "compat_Qt.moc"

#endif

QAction *PM::internal::addAction(QToolBar *toolbar, const QIcon &icon, const QString &text, const QObject *receiver,
                                 const std::function<void()> &callback)
{
    if (toolbar == nullptr)
        return nullptr;

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    return toolbar->addAction(icon, text, receiver, callback);
#else
    QAction *result = toolbar->addAction(icon, text);

    ActionCallbackStub *callbackStub = new ActionCallbackStub(result, callback);
    QObject::connect(result, SIGNAL(triggered()), callbackStub, SLOT(invokeCallback()));

    return result;
#endif
}

QAction *PM::internal::addAction(QToolBar *toolbar, const QString &text, const QObject *receiver, const std::function<void()> &callback)
{
    if (toolbar == nullptr)
        return nullptr;

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    return toolbar->addAction(text, receiver, callback);
#else
    QAction *result = toolbar->addAction(text);

    ActionCallbackStub *callbackStub = new ActionCallbackStub(result, callback);
    QObject::connect(result, SIGNAL(triggered()), callbackStub, SLOT(invokeCallback()));

    return result;
#endif
}
