#ifndef JSVALUEPROPERTYEDITOR_H
#define JSVALUEPROPERTYEDITOR_H

#include "IPropertyEditor.h"

#include <QJSValue>

class JSValuePropertyEditor : public IPropertyEditor
{
    // IPropertyEditor interface
public:
    virtual bool canHandleType(const QMetaType &type) const override;
    virtual bool canHandleValue(const QVariant &value) const override;

    virtual QTreeWidgetItem *createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                    QTreeWidgetItem *parentItem = nullptr, const QVariant &propertyValue = QVariant(),
                                                    QObjectViewer *parentObjectViewer = nullptr) const override;

protected:
    virtual bool showDetailsDialog(QObjectViewer *parentObjectViewer, const QJSValue &value, const PropertyData &propertyData) const;
};

#endif // JSVALUEPROPERTYEDITOR_H
