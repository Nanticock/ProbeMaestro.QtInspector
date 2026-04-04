#include "IPropertyEditor.h"
#include "IPropertyEditor_p.h"

#include "BoolPropertyEditor.h"
#include "ColorPropertyEditor.h"
#include "JSValuePropertyEditor.h"
#include "PointerPropertyEditor.h"
#include "QObjectViewer/PointerChecker/PointerChecker.h"
#include "QObjectViewer/QObjectViewer.h"
#include "QQmlListPropertyEditor.h"
#include "QmlListReferencePropertyEditor.h"
#include "StringPropertyEditor.h"
#include "VariantListPropertyEditor.h"
#include "VariantMapPropertyEditor.h"
#include "VariantPropertyEditor.h"

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QHeaderView>
#include <QMenu>
#include <QMetaProperty>
#include <QScreen>

QList<IPropertyEditorPtr> IPropertyEditorPrivate::s_defaultEditors;
IPropertyEditorPtr IPropertyEditorPrivate::s_defaultPropertyEditor = IPropertyEditorPtr::create();

QSize IPropertyEditorPrivate::calculateMaximumWindowSize(const QWidget &widget)
{
    const QStyle *style = QApplication::style();

    const int leftMargin = style->pixelMetric(QStyle::PM_LayoutLeftMargin);
    const int rightMargin = style->pixelMetric(QStyle::PM_LayoutRightMargin);
    const int topMargin = style->pixelMetric(QStyle::PM_LayoutTopMargin);
    const int bottomMargin = style->pixelMetric(QStyle::PM_LayoutBottomMargin);
    const int titleBarHeight = style->pixelMetric(QStyle::PM_TitleBarHeight);

    return widget.screen()->availableSize() - QSize(leftMargin + rightMargin, topMargin + bottomMargin + titleBarHeight);
}

void IPropertyEditorPrivate::initDefaultEditors()
{
    IPropertyEditorPrivate::s_defaultEditors.clear();

    // [priority 3] editors for the most trivial types
    IPropertyEditor::registerDefaultEditor(QSharedPointer<BoolPropertyEditor>::create());
    IPropertyEditor::registerDefaultEditor(QSharedPointer<ColorPropertyEditor>::create());
    IPropertyEditor::registerDefaultEditor(QSharedPointer<StringPropertyEditor>::create());

    // [priority 2] editors for non-trivial/complex types
    IPropertyEditor::registerDefaultEditor(QSharedPointer<JSValuePropertyEditor>::create());
    IPropertyEditor::registerDefaultEditor(QSharedPointer<VariantMapPropertyEditor>::create());
    IPropertyEditor::registerDefaultEditor(QSharedPointer<VariantListPropertyEditor>::create());
    IPropertyEditor::registerDefaultEditor(QSharedPointer<QmlListReferencePropertyEditor>::create());

    // [priority 1] editors for exotic/uncommon types
    IPropertyEditor::registerDefaultEditor(QSharedPointer<QQmlListPropertyEditor>::create());

    // [priority 0] fall-back editors when no high-priority editors can be used
    IPropertyEditor::registerDefaultEditor(QSharedPointer<VariantPropertyEditor>::create());
    IPropertyEditor::registerDefaultEditor(QSharedPointer<PointerPropertyEditor>::create());
}

void IPropertyEditorPrivate::applyPropertiesDialogDefaultStyle(QDialog &dialog, const QString &name)
{
    dialog.setModal(false);
    dialog.setWindowTitle(name);
    dialog.setLayout(new QVBoxLayout());
}

void IPropertyEditorPrivate::setPropertiesTreeWidgetDefaultContextMenus(QObjectViewer *parentObjectViewer, QTreeWidget &treeWidget)
{
    // Create a menu for the tree widget
    QMenu *treeMenu = new QMenu(&treeWidget);

    // Create actions for copying cell, copying row, expanding/collapsing row and expanding/collapsing all
    QAction *copyCellAction = new QAction("Copy", &treeWidget);
    QAction *copyRowAction = new QAction("Copy row", &treeWidget);
    QAction *followPointerAction = new QAction("Follow pointer", &treeWidget);

    // Add actions to the menu
    treeMenu->addAction(followPointerAction);
    treeMenu->addSeparator();
    treeMenu->addAction(copyCellAction);
    treeMenu->addAction(copyRowAction);

    treeWidget.setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect the custom context menu requested signal of the tree widget to a lambda function that shows the menu
    QObject::connect(&treeWidget, &QTreeWidget::customContextMenuRequested, &treeWidget,
                     [treeMenu, copyCellAction, followPointerAction, copyRowAction, &treeWidget](const QPoint &pos)
                     {
                         bool hasValidSelection = treeWidget.currentItem() != nullptr;

                         // copy actions
                         copyRowAction->setEnabled(hasValidSelection);
                         copyCellAction->setEnabled(hasValidSelection);

                         size_t value = 0;
                         bool isPointerValue = hasValidSelection && QObjectViewer::getAddressFromString(
                                                                        treeWidget.currentItem()->text(treeWidget.currentColumn()), &value);

                         bool isReadableAddress = isPointerValue && BasicPointerChecker::isReadableAddress(value);

                         followPointerAction->setEnabled(isReadableAddress);
                         followPointerAction->setToolTip(isReadableAddress ? "" : "Warning: This address points to an unreadable memory location");

                         treeMenu->exec(treeWidget.mapToGlobal(pos));
                     });

    // Connect the copy cell action to a lambda function that copies the text of the current item
    QObject::connect(copyCellAction, &QAction::triggered, &treeWidget,
                     [&treeWidget]()
                     {
                         QTreeWidgetItem *item = treeWidget.currentItem();
                         if (item)
                         {
                             // Get the text of the current item
                             QString text = item->text(treeWidget.currentColumn());
                             // Copy the text to the clipboard
                             QClipboard *clipboard = QApplication::clipboard();
                             clipboard->setText(text);
                         }
                     });

    // Connect the copy row action to a lambda function that copies the text of the current row
    QObject::connect(copyRowAction, &QAction::triggered, &treeWidget,
                     [&treeWidget]()
                     {
                         QTreeWidgetItem *item = treeWidget.currentItem();
                         if (item)
                         {
                             // Get the text of all columns of the current item
                             QStringList texts;
                             for (int i = 0; i < treeWidget.columnCount(); ++i)
                             {
                                 texts << item->text(i);
                             }
                             // Join the texts with tabs and copy to the clipboard
                             QString text = texts.join("\t");
                             QClipboard *clipboard = QApplication::clipboard();
                             clipboard->setText(text);
                         }
                     });

    // Connect the follow pointer action
    QObject::connect(followPointerAction, &QAction::triggered, &treeWidget,
                     [&treeWidget, parentObjectViewer]()
                     {
                         if (!parentObjectViewer)
                             return;

                         if (!treeWidget.currentItem())
                             return;

                         size_t value = 0;
                         if (!QObjectViewer::getAddressFromString(treeWidget.currentItem()->text(treeWidget.currentColumn()), &value))
                             return;

                         parentObjectViewer->gotoAddress(value);
                     });
}

void IPropertyEditorPrivate::applyPropertiesTreeWidgetDefaultStyle(QTreeWidget &treeWidget)
{
    treeWidget.setAlternatingRowColors(true);
    treeWidget.setHeaderLabels({"Index", "Type", "Value"});
    treeWidget.setSelectionBehavior(QAbstractItemView::SelectRows);
    treeWidget.setSelectionMode(QAbstractItemView::SingleSelection);
    treeWidget.header()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
}

void IPropertyEditorPrivate::setOptimumPropertiesDialogSize(QDialog &dialog, QTreeWidget &treeWidget)
{
    dialog.resize(IPropertyEditorPrivate::calculateOptimumPropertiesDialogSize(treeWidget));
}

QSize IPropertyEditorPrivate::calculateOptimumTreeWidgetSize(const QTreeWidget &treeWidget)
{
    QModelIndex root = treeWidget.rootIndex();
    QAbstractItemModel *model = treeWidget.model();
    const int headerHeight = treeWidget.header()->height();

    int contentWidth = 0;
    int contentHeight = headerHeight;

    const int columns = model->columnCount(root);
    for (int i = 0; i < columns; i++)
        contentWidth += treeWidget.columnWidth(i);

    for (int i = 0; i < treeWidget.topLevelItemCount(); i++)
    {
        contentHeight += headerHeight;

        QTreeWidgetItem *item = treeWidget.topLevelItem(i);
        if (item->isExpanded())
            contentHeight += item->childCount() * headerHeight;
    }

    const QSize contentSize(contentWidth, contentHeight);

    return contentSize.boundedTo(calculateMaximumWindowSize(treeWidget));
}

QSize IPropertyEditorPrivate::calculateOptimumPropertiesDialogSize(const QTreeWidget &treeWidget)
{
    const QStyle *style = QApplication::style();

    const int leftMargin = style->pixelMetric(QStyle::PM_LayoutLeftMargin);
    const int rightMargin = style->pixelMetric(QStyle::PM_LayoutRightMargin);
    const int topMargin = style->pixelMetric(QStyle::PM_LayoutTopMargin);
    const int bottomMargin = style->pixelMetric(QStyle::PM_LayoutBottomMargin);

    const QSize marginsSize(leftMargin + rightMargin, topMargin + bottomMargin);

    return (calculateOptimumTreeWidgetSize(treeWidget) + marginsSize).boundedTo(calculateMaximumWindowSize(treeWidget));
}

IPropertyEditorPtr IPropertyEditorPrivate::getPropertyEditorForType(int typeId, const QList<IPropertyEditorPtr> &editorsList)
{
    IPropertyEditorPtr result = IPropertyEditorPrivate::s_defaultPropertyEditor;

    for (IPropertyEditorPtr propertyEditor : editorsList)
    {
        if (propertyEditor->canHandleType(typeId))
            return propertyEditor;
    }

    return result;
}

IPropertyEditorPtr IPropertyEditorPrivate::getPropertyEditorForValue(const QVariant &value, const QList<IPropertyEditorPtr> &editorsList)
{
    IPropertyEditorPtr result = IPropertyEditorPrivate::s_defaultPropertyEditor;

    for (const IPropertyEditorPtr &propertyEditor : editorsList)
    {
        if (propertyEditor->canHandleValue(value))
            return propertyEditor;
    }

    return result;
}

IPropertyEditorPtr IPropertyEditorPrivate::getPropertyEditorForProperty(const QMetaProperty &property, const QList<IPropertyEditorPtr> &editorsList)
{
    IPropertyEditorPtr result = IPropertyEditorPrivate::s_defaultPropertyEditor;

    for (const IPropertyEditorPtr &propertyEditor : editorsList)
    {
        if (propertyEditor->canHandleProperty(property))
            return propertyEditor;
    }

    return result;
}

QList<IPropertyEditorPtr> IPropertyEditor::defaultEditors()
{
    // add default set of editors in case the default editors were not initialized yet
    if (IPropertyEditorPrivate::s_defaultEditors.isEmpty())
        IPropertyEditorPrivate::initDefaultEditors();

    return IPropertyEditorPrivate::s_defaultEditors;
}

bool IPropertyEditor::registerDefaultEditor(IPropertyEditorPtr editor)
{
    if (editor.isNull())
        return false;

    for (IPropertyEditorPtr defaultEditor : qAsConst(IPropertyEditorPrivate::s_defaultEditors))
    {
        if (defaultEditor->uid() == editor->uid())
            return false;
    }

    IPropertyEditorPrivate::s_defaultEditors << editor;
    return true;
}

bool IPropertyEditor::unregisterDefaultEditor(IPropertyEditorPtr editor)
{
    if (editor.isNull())
        return false;

    for (IPropertyEditorPtr defaultEditor : qAsConst(IPropertyEditorPrivate::s_defaultEditors))
    {
        if (defaultEditor->uid() != editor->uid())
            continue;

        IPropertyEditorPrivate::s_defaultEditors.removeAll(defaultEditor);
        return true;
    }

    return false;
}

IPropertyEditorPtr IPropertyEditor::getPropertyEditorForType(int typeId)
{
    return IPropertyEditorPrivate::getPropertyEditorForType(typeId, IPropertyEditorPrivate::s_defaultEditors);
}

IPropertyEditorPtr IPropertyEditor::getPropertyEditorForValue(const QVariant &value)
{
    return IPropertyEditorPrivate::getPropertyEditorForValue(value, IPropertyEditorPrivate::s_defaultEditors);
}

IPropertyEditorPtr IPropertyEditor::getPropertyEditorForProperty(const QMetaProperty &property)
{
    return IPropertyEditorPrivate::getPropertyEditorForProperty(property, IPropertyEditorPrivate::s_defaultEditors);
}

size_t IPropertyEditor::uid() const
{
    return typeid(*this).hash_code();
}

QString IPropertyEditor::name() const
{
    return QString(typeid(*this).name()).remove(0, 6);
}

bool IPropertyEditor::canHandleType(int typeId) const
{
    return canHandleType(QMetaType(typeId));
}

bool IPropertyEditor::canHandleType(const QMetaType &type) const
{
    if (!type.isValid())
        return false;

    if (!type.isRegistered())
        return false;

    return true;
}

bool IPropertyEditor::canHandleValue(const QVariant &value) const
{
    return canHandleType(value.type());
}

bool IPropertyEditor::canHandleProperty(const PropertyData &property) const
{
    return canHandleType(QMetaType(property.type));
}

bool IPropertyEditor::canHandleProperty(const QMetaProperty &property) const
{
    return canHandleProperty(PropertyData(property));
}

QTreeWidgetItem *IPropertyEditor::createPropertyTreeItem(const QMetaProperty &property, QObjectViewer *parentObjectViewer,
                                                         QTreeWidgetItem *parentItem) const
{
    return createPropertyTreeItem(PropertyData(property), parentObjectViewer, parentItem);
}

QTreeWidgetItem *IPropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QObjectViewer *parentObjectViewer,
                                                         QTreeWidgetItem *parentItem) const
{
    // if we have a valid parentObjectViewer and it has a valid current object
    // then we need to set the property value as well
    QVariant propertyValue;
    if (parentObjectViewer != nullptr && parentObjectViewer->currentObjectIsValid())
        propertyValue = parentObjectViewer->currentObject()->property(propertyData.name);

    return createPropertyTreeItem(propertyData, parentObjectViewer->treeWidget(), parentItem, propertyValue, parentObjectViewer);
}

QTreeWidgetItem *IPropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget, QTreeWidgetItem *parentItem,
                                                         const QVariant &propertyValue, QObjectViewer *parentObjectViewer) const
{
    // Create a child item for the property
    QTreeWidgetItem *propertyItem = new QTreeWidgetItem(parentItem);
    propertyItem->setText(0, propertyData.name);
    propertyItem->setText(1, propertyData.typeName);

    // add property attributes
    createNewAttribute("hasNotifySignal", propertyData.hasNotifySignal, propertyItem);
    createNewAttribute("hasStdCppSet", propertyData.hasStdCppSet, propertyItem);
    createNewAttribute("isConstant", propertyData.isConstant, propertyItem);
    createNewAttribute("isDesignable", propertyData.isDesignable, propertyItem);
    createNewAttribute("isEnumType", propertyData.isEnumType, propertyItem);
    createNewAttribute("isFinal", propertyData.isFinal, propertyItem);
    createNewAttribute("isFlagType", propertyData.isFlagType, propertyItem);
    createNewAttribute("isReadable", propertyData.isReadable, propertyItem);
    createNewAttribute("isRequired", propertyData.isRequired, propertyItem);
    createNewAttribute("isResettable", propertyData.isResettable, propertyItem);
    createNewAttribute("isScriptable", propertyData.isScriptable, propertyItem);
    createNewAttribute("isStored", propertyData.isStored, propertyItem);
    createNewAttribute("isUser", propertyData.isUser, propertyItem);
    createNewAttribute("isValid", propertyData.isValid, propertyItem);
    createNewAttribute("isWritable", propertyData.isWritable, propertyItem);
    createNewAttribute("isEditable", propertyData.isEditable, propertyItem);

    if (propertyValue.isValid())
        propertyItem->setText(2, propertyValue.toString());

    return propertyItem;
}

QTreeWidgetItem *IPropertyEditor::createNewAttribute(const QString &name, bool value, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *result = new QTreeWidgetItem(parent);

    QFont font = QApplication::font();

    if (!value)
        font.setItalic(true);

    QBrush foregroundBrush = value ? Qt::black : Qt::darkGray;

    result->setText(0, name);
    result->setFont(0, font);
    result->setForeground(0, foregroundBrush);

    result->setText(2, value ? "true" : "false");
    result->setFont(2, font);
    result->setForeground(2, foregroundBrush);

    return result;
}

PropertyData::PropertyData(const QMetaProperty &metaProperty)
{
    name = metaProperty.name();
    type = metaProperty.type();
    typeName = metaProperty.typeName();

    hasNotifySignal = metaProperty.hasNotifySignal();
    hasStdCppSet = metaProperty.hasStdCppSet();
    isConstant = metaProperty.isConstant();
    isDesignable = metaProperty.isDesignable();
    isEnumType = metaProperty.isEnumType();
    isFinal = metaProperty.isFinal();
    isFlagType = metaProperty.isFlagType();
    isReadable = metaProperty.isReadable();
    isResettable = metaProperty.isResettable();
    isScriptable = metaProperty.isScriptable();
    isStored = metaProperty.isStored();
    isUser = metaProperty.isUser();
    isValid = metaProperty.isValid();
    isWritable = metaProperty.isWritable();

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    isEditable = true;
#else
    isEditable = metaProperty.isEditable();
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    isRequired = metaProperty.isRequired();
#else
    isRequired = false;
#endif
}
