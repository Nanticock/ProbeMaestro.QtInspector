#ifndef IPROPERTYEDITOR_H
#define IPROPERTYEDITOR_H

#include <QTreeWidgetItem>

struct PropertyData
{
    PropertyData(const QMetaProperty &metaProperty);

    const char *name;
    QString typeName;
    QVariant::Type type;

    bool hasNotifySignal;
    bool hasStdCppSet;
    bool isConstant;
    bool isDesignable;
    bool isEnumType;
    bool isFinal;
    bool isFlagType;
    bool isReadable;
    bool isRequired;
    bool isResettable;
    bool isScriptable;
    bool isStored;
    bool isUser;
    bool isValid;
    bool isWritable;
    bool isEditable;
};

class QObjectViewer;

class IPropertyEditor;

typedef QSharedPointer<IPropertyEditor> IPropertyEditorPtr;

class IPropertyEditor
{
    friend class QObjectViewer;

public:
    static QList<IPropertyEditorPtr> defaultEditors();
    static bool registerDefaultEditor(IPropertyEditorPtr editor);
    static bool unregisterDefaultEditor(IPropertyEditorPtr editor);

    static IPropertyEditorPtr getPropertyEditorForType(int typeId);
    static IPropertyEditorPtr getPropertyEditorForValue(const QVariant &value);
    static IPropertyEditorPtr getPropertyEditorForProperty(const QMetaProperty &property);

public:
    virtual size_t uid() const;
    virtual QString name() const;

    // types
    bool canHandleType(int typeId) const;
    virtual bool canHandleType(const QMetaType &type) const;
    // values
    virtual bool canHandleValue(const QVariant &value) const;
    // properties
    bool canHandleProperty(const QMetaProperty &property) const;
    virtual bool canHandleProperty(const PropertyData &property) const;

    virtual QTreeWidgetItem *createPropertyTreeItem(const QMetaProperty &property, QObjectViewer *parentObjectViewer,
                                                    QTreeWidgetItem *parentItem = nullptr) const;

    virtual QTreeWidgetItem *createPropertyTreeItem(const PropertyData &propertyData, QObjectViewer *parentObjectViewer,
                                                    QTreeWidgetItem *parentItem = nullptr) const;

    virtual QTreeWidgetItem *createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                    QTreeWidgetItem *parentItem = nullptr, const QVariant &propertyValue = QVariant(),
                                                    QObjectViewer *parentObjectViewer = nullptr) const;

protected:
    static QTreeWidgetItem *createNewAttribute(const QString &name, bool value, QTreeWidgetItem *parent);
};

#endif // IPROPERTYEDITOR_H
