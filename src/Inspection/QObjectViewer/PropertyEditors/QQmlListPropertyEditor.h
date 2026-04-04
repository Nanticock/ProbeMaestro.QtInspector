#ifndef QQMLLISTPROPERTYEDITOR_H
#define QQMLLISTPROPERTYEDITOR_H

#include "VariantListPropertyEditor.h"

class QQmlListPropertyEditor : public VariantListPropertyEditor
{
    // IPropertyEditor interface
public:
    virtual bool canHandleProperty(const PropertyData &property) const override;

    virtual QTreeWidgetItem *createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                    QTreeWidgetItem *parentItem = nullptr, const QVariant &propertyValue = QVariant(),
                                                    QObjectViewer *parentObjectViewer = nullptr) const override;
};

#endif // QQMLLISTPROPERTYEDITOR_H
