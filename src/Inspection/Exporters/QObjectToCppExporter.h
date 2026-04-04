#ifndef QOBJECTTOCPPEXPORTER_H
#define QOBJECTTOCPPEXPORTER_H

#include "IExporter.h"

Q_DECLARE_METATYPE(const QMetaObject *)

class QObjectToCppExporter : public IExporter
{
public:
    class QObjectToCppExportOptions : public IExporter::Options
    {
    public:
        bool exportCpp = false;
        bool exportHeader = true;
    };

public:
    virtual bool exportToByteArray(const QVariant &value, QByteArray &output, const Options *options = nullptr) override;

private:
    virtual bool _exportToByteArray(const QObject *value, QByteArray &output, const QObjectToCppExportOptions &options);
    virtual bool _exportToByteArray(const QMetaObject *value, QByteArray &output, const QObjectToCppExportOptions &options);
};

#endif // QOBJECTTOCPPEXPORTER_H
