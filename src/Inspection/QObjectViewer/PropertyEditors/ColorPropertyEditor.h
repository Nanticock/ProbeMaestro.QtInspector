#ifndef COLORPROPERTYEDITOR_H
#define COLORPROPERTYEDITOR_H

#include "IPropertyEditor.h"

class ColorPropertyEditor : public IPropertyEditor
{
    // IPropertyEditor interface
public:
    virtual bool canHandleType(const QMetaType &type) const override;

    virtual QTreeWidgetItem *createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                    QTreeWidgetItem *parentItem = nullptr, const QVariant &propertyValue = QVariant(),
                                                    QObjectViewer *parentObjectViewer = nullptr) const override;

private:
    static QString colorToString(const QColor &color);
    static void setWidgetBackgroundStyleSheet(QWidget *widget, const QColor &backgroundColor);
};

#endif // COLORPROPERTYEDITOR_H
