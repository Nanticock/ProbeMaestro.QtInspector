#ifndef VARIANTLISTPROPERTYEDITOR_H
#define VARIANTLISTPROPERTYEDITOR_H

#include "IPropertyEditor.h"

class VariantListPropertyEditor : public IPropertyEditor
{
    // IPropertyEditor interface
public:
    virtual bool canHandleType(const QMetaType &type) const override;
    virtual bool canHandleValue(const QVariant &value) const override;

    virtual QTreeWidgetItem *createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                    QTreeWidgetItem *parentItem = nullptr, const QVariant &propertyValue = QVariant(),
                                                    QObjectViewer *parentObjectViewer = nullptr) const override;

protected:
    virtual bool showListDialog(const PropertyData &propertyData, QObjectViewer *parentObjectViewer, const QVariantList &list) const;
};

#endif // VARIANTLISTPROPERTYEDITOR_H
