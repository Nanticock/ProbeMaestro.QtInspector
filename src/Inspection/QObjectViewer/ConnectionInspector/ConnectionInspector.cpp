#include "ConnectionInspector.h"

#include "QtCorePrivate/5/qobject_p.h"

#include <QDebug>
#include <QMetaMethod>

static QString pointerToString(const void *ptr, const QString &pointerType)
{
    QString result = pointerType + "(0x%1)";

    result = result.arg(size_t(ptr), 0, 16);

    return result;
}

static QString pointerToString(QObject *object)
{
    return pointerToString(object, object->metaObject()->className());
}

static QString connectionTypeToString(int connectionType)
{
    static QHash<int, QString> s_connectionTypeNameMap{
        {Qt::ConnectionType::AutoConnection, "Auto"},
        {Qt::ConnectionType::DirectConnection, "Direct"},
        {Qt::ConnectionType::QueuedConnection, "Queued"},
        {Qt::ConnectionType::BlockingQueuedConnection, "BlockingQueued"},
        {4, "Blocking"},
        {Qt::ConnectionType::UniqueConnection, "Unique"},
    };

    return pointerToString((void *)size_t(connectionType), s_connectionTypeNameMap[connectionType]);
}

static QString methodToString(int methodIndex, const QString &methodName)
{
    QString result = QString::number(methodIndex) + " \"%1\"";

    result = result.arg(methodName);

    return result;
}

static QString methodToString(QObject *object, int methodIndex)
{
    return methodToString(methodIndex, object->metaObject()->method(methodIndex).name());
}

static QString connectionToQString(QObjectPrivate::Connection *connection)
{
    QString result = "Connection:\n"
                     "{\n"
                     "  ID: %1\n"
                     "  Signal: %2\n"
                     "  Sender: %3\n"
                     "  Slot: %4\n"
                     "  Receiver: %5\n"
                     "  Connection type: %6\n"
                     "}\n";

    result = result.arg(connection->id);
    result = result.arg(methodToString(connection->sender, connection->signal_index));
    result = result.arg(pointerToString(connection->sender));

    if (connection->isSlotObject)
        result = result.arg(methodToString(connection->receiver.loadRelaxed(), connection->slotObj->ref()));
    else
        result = result.arg(pointerToString((void *)connection->callFunction, "_slot_"));

    result = result.arg(pointerToString(connection->receiver.loadRelaxed()));
    result = result.arg(connectionTypeToString(connection->connectionType));

    return result;
}

ConnectionInspector::ConnectionInspector()
{
}

void ConnectionInspector::test(QObject *object, const QString &title)
{
    if (object == nullptr)
        return;

    // Refer to: https://sl.bing.net/ezdg2uMpqGy
    QObjectPrivate *object_p = QObjectPrivate::get(object);

    // qInfo().noquote() << "sender list:" << object_p->senderList();

    // for(int i = 0; i < object->metaObject()->methodCount(); i++)
    // {
    //     const char *signalName = object->metaObject()->method(i).name();
    //     object_p->signalIndex(signalName);
    //     qInfo().noquote() << "receiver list:" << object_p->receiverList(object->metaObject()->method(i).tag()).count();
    // }

    // Refer to: https://sl.bing.net/cneGWHlaCuO
    QObjectPrivate::ConnectionData *connectionData = object_p->connections.loadRelaxed();

    if (!connectionData)
        return;

    auto signalVector = connectionData->signalVector.loadAcquire() ? connectionData->signalVector.loadRelaxed() : nullptr;

    if (signalVector == nullptr)
        return;

    qInfo() << " **" << title << object << "Signal Vector Count:" << signalVector->count();

    for (int i = 0; i < signalVector->count(); i++)
    {
        auto connectionList = signalVector->at(i);

        QObjectPrivate::Connection *connection = connectionList.first;
        while (connection != nullptr)
        {
            qInfo().noquote() << "[" << i << "] = " << connectionToQString(connection);

            QObjectPrivate::Connection *nextConnection = connection->ConnectionOrSignalVector::next;
            if (connection != nextConnection)
                connection = nextConnection;
            else
                connection = nullptr;
        }
    }
}
