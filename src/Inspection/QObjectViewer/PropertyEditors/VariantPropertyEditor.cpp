#include "VariantPropertyEditor.h"

#include "QObjectViewer/QObjectViewer.h"
#include "VariantPropertyEditor_p.h"
#include <compat_Qt.h>

#include <QMetaProperty>

bool VariantPropertyEditor::canHandleType(const QMetaType &type) const
{
    return PM::internal::getMetaTypeId(type) == qMetaTypeId<QVariant>();
}

QObject *VariantPropertyEditorPrivate::variantToQObject(const QVariant &value)
{
    QObject *result = value.value<QObject *>();

    // in the case of values that cannot be get directly as QObjects (such as values of type QVariant)
    // we will have to do it the long way
    QVariant temp = value;
    if (result == nullptr && temp.convert(qMetaTypeId<QObject *>()))
        result = temp.value<QObject *>();

    return result;
}

IPropertyEditorPtr VariantPropertyEditorPrivate::getPropertyEditorForValue(const QVariant &value, QObjectViewer *parentObjectViewer)
{
    IPropertyEditorPtr result;

    if (parentObjectViewer == nullptr)
        result = IPropertyEditor::getPropertyEditorForValue(value);
    else
        result = parentObjectViewer->getPropertyEditorForValue(value);

    return result;
}

QTreeWidgetItem *VariantPropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                               QTreeWidgetItem *parentItem, const QVariant &propertyValue,
                                                               QObjectViewer *parentObjectViewer) const
{
    // get the appropriate property editor
    IPropertyEditorPtr variantTypeEditor = VariantPropertyEditorPrivate::getPropertyEditorForValue(propertyValue, parentObjectViewer);

    // if we got a property editor then use it to create a property tree item
    // otherwise, just create a basic property tree item using the default implementation provided by our parent class
    QTreeWidgetItem *propertyItem = nullptr;
    if (variantTypeEditor.isNull())
        propertyItem = IPropertyEditor::createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, propertyValue, parentObjectViewer);
    else
        propertyItem = variantTypeEditor->createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, propertyValue, parentObjectViewer);

    propertyItem->setText(2, propertyValue.typeName());

    return propertyItem;
}
