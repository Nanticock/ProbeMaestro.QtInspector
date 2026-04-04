#ifndef IPROPERTYEDITOR_P_H
#define IPROPERTYEDITOR_P_H

#include "IPropertyEditor.h"

class IPropertyEditorPrivate
{
public:
    static void initDefaultEditors();

    static QSize calculateMaximumWindowSize(const QWidget &widget);
    static void applyPropertiesTreeWidgetDefaultStyle(QTreeWidget &treeWidget);
    static void setOptimumPropertiesDialogSize(QDialog &dialog, QTreeWidget &treeWidget);
    static void applyPropertiesDialogDefaultStyle(QDialog &dialog, const QString &name = "");
    static void setPropertiesTreeWidgetDefaultContextMenus(QObjectViewer *parentObjectViewer, QTreeWidget &treeWidget);

    static QSize calculateOptimumTreeWidgetSize(const QTreeWidget &treeWidget);
    static QSize calculateOptimumPropertiesDialogSize(const QTreeWidget &treeWidget);

    static IPropertyEditorPtr getPropertyEditorForType(int typeId, const QList<IPropertyEditorPtr> &editorsList);
    static IPropertyEditorPtr getPropertyEditorForValue(const QVariant &value, const QList<IPropertyEditorPtr> &editorsList);
    static IPropertyEditorPtr getPropertyEditorForProperty(const QMetaProperty &property, const QList<IPropertyEditorPtr> &editorsList);

public:
    static QList<IPropertyEditorPtr> s_defaultEditors;
    static IPropertyEditorPtr s_defaultPropertyEditor;
};

#endif // IPROPERTYEDITOR_P_H
