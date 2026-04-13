#include "JSValuePropertyEditor.h"
#include "IPropertyEditor_p.h"

#include "QObjectViewer/QObjectViewer.h"
#include "VariantPropertyEditor_p.h"

#include <compat_Qt.h>

#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>

bool JSValuePropertyEditor::canHandleType(const QMetaType &type) const
{
    return PM::internal::getMetaTypeId(type) == qMetaTypeId<QJSValue>();
}

bool JSValuePropertyEditor::canHandleValue(const QVariant &value) const
{
    return value.canConvert<QJSValue>();
}

QTreeWidgetItem *JSValuePropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                               QTreeWidgetItem *parentItem, const QVariant &propertyValue,
                                                               QObjectViewer *parentObjectViewer) const
{
    QTreeWidgetItem *propertyItem =
        IPropertyEditor::createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, propertyValue, parentObjectViewer);

    if (!propertyValue.isValid() || !propertyValue.canConvert<QJSValue>())
        return propertyItem;

    QJSValue jsValue = propertyValue.value<QJSValue>();

    // create the gotoButton
    QPushButton *gotoButton = new QPushButton();
    gotoButton->setText("...");
    gotoButton->setEnabled(!(jsValue.isNull() || jsValue.isError()));
    gotoButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    const int gotoButtonTextWidth = PM::internal::fontMetricsHorizontalAdvance(QApplication::fontMetrics(), gotoButton->text());
    gotoButton->setMaximumWidth(std::max(gotoButtonTextWidth, 20));

    QObject::connect(gotoButton, &QPushButton::clicked, gotoButton,
                     [jsValue, this, propertyData, parentObjectViewer]() { showDetailsDialog(parentObjectViewer, jsValue, propertyData); });

    // create the itemLabel
    QLabel *itemLabel = new QLabel();
    itemLabel->setFont(QFont("", -1, PM::internal::QFont_Thin, true));
    itemLabel->setText(QString("QJSValue(%1)").arg(jsValue.toVariant().typeName()));
    itemLabel->setSelection(0, 10);
    itemLabel->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));

    // create the itemWidget
    QWidget *itemWidget = new QWidget();
    itemWidget->setLayout(new QHBoxLayout());
    itemWidget->layout()->setMargin(0);
    itemWidget->layout()->addWidget(itemLabel);
    itemWidget->layout()->addWidget(gotoButton);

    propertyItem->setText(2, itemLabel->text());
    propertyItem->setForeground(2, QBrush(Qt::transparent));

    parentTreeWidget.setItemWidget(propertyItem, 2, itemWidget);

    return propertyItem;
}

bool JSValuePropertyEditor::showDetailsDialog(QObjectViewer *parentObjectViewer, const QJSValue &value, const PropertyData &propertyData) const
{
    QDialog dialog;
    IPropertyEditorPrivate::applyPropertiesDialogDefaultStyle(dialog, propertyData.name);

    QTreeWidget treeWidget;
    IPropertyEditorPrivate::applyPropertiesTreeWidgetDefaultStyle(treeWidget);
    IPropertyEditorPrivate::setPropertiesTreeWidgetDefaultContextMenus(parentObjectViewer, treeWidget);

    QVariant valueAsVariant = value.toVariant();

    // Value
    QTreeWidgetItem *rootValueItem = new QTreeWidgetItem(&treeWidget);
    rootValueItem->setExpanded(true);
    rootValueItem->setText(0, PM::internal::getMetaTypeName<QJSValue>());
    rootValueItem->setText(2, valueAsVariant.typeName());

    IPropertyEditorPtr variantTypeEditor = VariantPropertyEditorPrivate::getPropertyEditorForValue(valueAsVariant, parentObjectViewer);
    QTreeWidgetItem *valueItem =
        variantTypeEditor->createPropertyTreeItem(propertyData, treeWidget, rootValueItem, valueAsVariant, parentObjectViewer);
    valueItem->takeChildren();
    valueItem->setText(0, "Value");
    valueItem->setText(1, "");

    QTreeWidgetItem *propertiesItem = new QTreeWidgetItem(rootValueItem);
    propertiesItem->setText(0, "Properties");

    createNewAttribute("isArray", value.isArray(), propertiesItem);
    createNewAttribute("isBool", value.isBool(), propertiesItem);
    createNewAttribute("isCallable", value.isCallable(), propertiesItem);
    createNewAttribute("isDate", value.isDate(), propertiesItem);
    createNewAttribute("isError", value.isError(), propertiesItem);
    createNewAttribute("isNull", value.isNull(), propertiesItem);
    createNewAttribute("isNumber", value.isNumber(), propertiesItem);
    createNewAttribute("isObject", value.isObject(), propertiesItem);
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    createNewAttribute("isQMetaObject", value.isQMetaObject(), propertiesItem);
#endif
    createNewAttribute("isQObject", value.isQObject(), propertiesItem);
    createNewAttribute("isRegExp", value.isRegExp(), propertiesItem);
    createNewAttribute("isString", value.isString(), propertiesItem);
    createNewAttribute("isUndefined", value.isUndefined(), propertiesItem);
    createNewAttribute("isVariant", value.isVariant(), propertiesItem);

    dialog.layout()->addWidget(&treeWidget);
    IPropertyEditorPrivate::setOptimumPropertiesDialogSize(dialog, treeWidget);

    if (!dialog.exec())
        return false;

    return true;
}
