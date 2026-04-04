#include "IExporter.h"

#include <QFile>

bool IExporter::exportToFile(const QVariant &value, const QString &filePath, const Options *options)
{
    QFile file(filePath);

    if (!file.open(QFile::WriteOnly))
        return false;

    return exportToIoDevice(value, file, options);
}

bool IExporter::exportToIoDevice(const QVariant &value, QIODevice &outputDevice, const Options *options)
{
    if (!outputDevice.isOpen() || !outputDevice.isWritable())
        return false;

    QByteArray data;

    if (!exportToByteArray(value, data, options))
        return false;

    return outputDevice.write(data) == data.length();
}
