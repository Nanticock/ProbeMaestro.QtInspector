#ifndef CONNECTIONINSPECTOR_H
#define CONNECTIONINSPECTOR_H

#include <QObject>

// Refer to: https://sl.bing.net/ezdg2uMpqGy
class ConnectionInspector
{
public:
    ConnectionInspector();

    static void test(QObject *object, const QString &title = "");
};

#endif // CONNECTIONINSPECTOR_H
