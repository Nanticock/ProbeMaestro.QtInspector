#ifndef POINTERPROPERTYEDITOR_H
#define POINTERPROPERTYEDITOR_H

#include "IPropertyEditor.h"

class PointerPropertyEditor : public IPropertyEditor
{
    // IPropertyEditor interface
public:
    virtual bool canHandleType(const QMetaType &type) const override;

    virtual QTreeWidgetItem *createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                    QTreeWidgetItem *parentItem = nullptr, const QVariant &propertyValue = QVariant(),
                                                    QObjectViewer *parentObjectViewer = nullptr) const override;
};

#endif // POINTERPROPERTYEDITOR_H
