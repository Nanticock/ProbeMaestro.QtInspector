#ifndef QQMLLISTPROPERTYEDITOR_P_H
#define QQMLLISTPROPERTYEDITOR_P_H

#include "IPropertyEditor.h"

#include <QQmlListProperty>
#include <QQuickItem>

class QQmlListPropertyEditorPrivate
{
public:
    static bool isQQmlListProperty(const QString &typeName);
    static bool isQQmlListProperty(const PropertyData &property);

    static int getListElementTypeId(const QString &listTypeName);
    static QString getListElementTypeName(const QString &listTypeName);

    static QQmlListProperty<QObject> toQmlListProperty(const QVariant &value);
    static QQmlListReference getQmlListReference(QObject *object, const QString &propertyName, QQmlEngine *engine = nullptr);

    static QVariantList toVariantList(const QQmlListReference &value);
    static QVariantList toVariantList(QQmlListProperty<QObject> &value);
};

#endif // QQMLLISTPROPERTYEDITOR_P_H
