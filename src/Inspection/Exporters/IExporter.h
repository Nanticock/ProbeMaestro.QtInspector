#ifndef IEXPORTER_H
#define IEXPORTER_H

#include <QIODevice>
#include <QVariant>

class IExporter
{
public:
    class Options
    {
    public:
        virtual ~Options() = default;
    };

public:
    virtual bool exportToFile(const QVariant &value, const QString &filePath, const Options *options = nullptr);
    virtual bool exportToIoDevice(const QVariant &value, QIODevice &outputDevice, const Options *options = nullptr);

    virtual bool exportToByteArray(const QVariant &value, QByteArray &output, const Options *options = nullptr) = 0;
};

#endif // IEXPORTER_H
