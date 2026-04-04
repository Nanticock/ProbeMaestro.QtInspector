#ifndef VARIANTMAPPROPERTYEDITOR_H
#define VARIANTMAPPROPERTYEDITOR_H

#include "IPropertyEditor.h"

class VariantMapPropertyEditor : public IPropertyEditor
{
public:
    virtual bool canHandleType(const QMetaType &type) const override;

    virtual QTreeWidgetItem *createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                    QTreeWidgetItem *parentItem = nullptr, const QVariant &propertyValue = QVariant(),
                                                    QObjectViewer *parentObjectViewer = nullptr) const override;

protected:
    virtual bool showListDialog(const PropertyData &propertyData, QObjectViewer *parentObjectViewer, const QVariantMap &map) const;
};

#endif // VARIANTMAPPROPERTYEDITOR_H
