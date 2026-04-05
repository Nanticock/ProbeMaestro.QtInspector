#include "BoolPropertyEditor.h"

#include "QObjectViewer/QObjectViewer.h"

#include <compat_Qt.h>

#include <QCheckBox>

bool BoolPropertyEditor::canHandleType(const QMetaType &type) const
{
    return PM::internal::getMetaTypeId(type) == qMetaTypeId<bool>();
}

QTreeWidgetItem *BoolPropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                            QTreeWidgetItem *parentItem, const QVariant &propertyValue,
                                                            QObjectViewer *parentObjectViewer) const
{
    QTreeWidgetItem *propertyItem =
        IPropertyEditor::createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, propertyValue, parentObjectViewer);

    QCheckBox *itemCheckbox = new QCheckBox();
    itemCheckbox->setChecked(propertyValue.value<bool>());
    itemCheckbox->setEnabled(propertyData.isWritable && parentObjectViewer != nullptr);
    itemCheckbox->setText(propertyValue.toString() + (propertyData.isWritable ? "" : " (Readonly)"));

    propertyItem->setText(2, propertyValue.toString());
    propertyItem->setForeground(2, QBrush(Qt::transparent));

    parentTreeWidget.setItemWidget(propertyItem, 2, itemCheckbox);

    if (parentObjectViewer == nullptr)
        return propertyItem;

    // we only need to do this connection if we have a parent ObjectViewer
    // because this is the only sitiuation where we can change the a QObject value by using its setProperty() function
    QObject::connect(itemCheckbox, &QCheckBox::toggled, itemCheckbox,
                     [parentObjectViewer, propertyData](bool value)
                     {
                         //
                         parentObjectViewer->currentObject()->setProperty(propertyData.name, value);
                     });

    return propertyItem;
}
