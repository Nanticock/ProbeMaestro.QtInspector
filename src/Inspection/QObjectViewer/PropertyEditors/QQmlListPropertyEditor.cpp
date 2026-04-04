#include "QQmlListPropertyEditor.h"
#include "QQmlListPropertyEditor_p.h"

#include "QObjectViewer/QObjectViewer.h"

#include <QQmlListProperty>
#include <QQuickItem>

#define QQMLLISTPROPERTY_NAME_SIGNATURE "QQmlListProperty<"

bool QQmlListPropertyEditorPrivate::isQQmlListProperty(const QString &typeName)
{
    return typeName.trimmed().startsWith(QQMLLISTPROPERTY_NAME_SIGNATURE);
}

bool QQmlListPropertyEditorPrivate::isQQmlListProperty(const PropertyData &property)
{
    return isQQmlListProperty(property.typeName);
}

int QQmlListPropertyEditorPrivate::getListElementTypeId(const QString &listTypeName)
{
    QString listElementTypeName = getListElementTypeName(listTypeName);

    return QMetaType::type(listElementTypeName.toStdString().c_str());
}

QString QQmlListPropertyEditorPrivate::getListElementTypeName(const QString &listTypeName)
{
    QString result = listTypeName.trimmed().split('<').last();
    result = result.left(result.length() - 1) + "*";

    return result;
}

QQmlListProperty<QObject> QQmlListPropertyEditorPrivate::toQmlListProperty(const QVariant &value)
{
    // return an empty QQmlListProperty<QOBject> for invalid and null values
    // Fixes BUG: goToSuperClass() for QMetaObjects with properties of type QQmlListProperty<>
    if (!value.isValid() || value.isNull())
        return QQmlListProperty<QObject>();

    const QQmlListProperty<QObject> *listPtr = reinterpret_cast<const QQmlListProperty<QObject> *>(value.data());

    return *listPtr;
}

QQmlListReference QQmlListPropertyEditorPrivate::getQmlListReference(QObject *object, const QString &propertyName, QQmlEngine *engine)
{
    return QQmlListReference(object, propertyName.toStdString().c_str(), engine);
}

QVariantList QQmlListPropertyEditorPrivate::toVariantList(const QQmlListReference &value)
{
    QVariantList result;

    if (!value.canCount() || !value.canAt())
        return result;

    int listCount = value.count();
    for (int i = 0; i < listCount; i++)
        result << QVariant::fromValue(value.at(i));

    return result;
}

QVariantList QQmlListPropertyEditorPrivate::toVariantList(QQmlListProperty<QObject> &value)
{
    QVariantList result;

    if (value.count == nullptr || value.at == nullptr)
        return result;

    int listCount = value.count(&value);
    for (int i = 0; i < listCount; i++)
        result << QVariant::fromValue(value.at(&value, i));

    return result;
}

bool QQmlListPropertyEditor::canHandleProperty(const PropertyData &property) const
{
    // FIXME: modify the IPropertyEditor interface so that it can handle sitiuations such as these,
    // where we need to have access to the QObject instance of this property if we want to use the QQmlReference method

    return QQmlListPropertyEditorPrivate::isQQmlListProperty(property);
}

QTreeWidgetItem *QQmlListPropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                                QTreeWidgetItem *parentItem, const QVariant &propertyValue,
                                                                QObjectViewer *parentObjectViewer) const
{
    // we should usually use a QQmlListReference to access QQmlListProperties, but since we allow the user to view
    // non-QObject objects, we have a fall-back mechanism that can access QQmlListProperties in our own way

    QVariantList variantList;

    // if we have a parent object viewer and it has a valid current object, we use QQmlListReference
    if (parentObjectViewer != nullptr && parentObjectViewer->currentObjectIsValid())
    {
        QQmlListReference list = QQmlListPropertyEditorPrivate::getQmlListReference(parentObjectViewer->currentObject(), propertyData.name);
        variantList = QQmlListPropertyEditorPrivate::toVariantList(list);
    }
    else // otherwise we use the dirty fall-back mechanism
    {
        QQmlListProperty<QObject> list = QQmlListPropertyEditorPrivate::toQmlListProperty(propertyValue);
        variantList = QQmlListPropertyEditorPrivate::toVariantList(list);
    }

    return VariantListPropertyEditor::createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, QVariant::fromValue(variantList),
                                                             parentObjectViewer);
}
