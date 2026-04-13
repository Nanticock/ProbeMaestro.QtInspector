#include "VariantMapPropertyEditor.h"
#include "IPropertyEditor_p.h"
#include "VariantPropertyEditor_p.h"

#include <QObjectViewer/QObjectViewer.h>
#include <compat_Qt.h>

#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>

bool VariantMapPropertyEditor::canHandleType(const QMetaType &type) const
{
    return PM::internal::getMetaTypeId(type) == qMetaTypeId<QVariantMap>();
}

QTreeWidgetItem *VariantMapPropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                                  QTreeWidgetItem *parentItem, const QVariant &propertyValue,
                                                                  QObjectViewer *parentObjectViewer) const
{
    QTreeWidgetItem *propertyItem =
        IPropertyEditor::createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, propertyValue, parentObjectViewer);

    if (!propertyValue.isValid())
        return propertyItem;

    QVariantMap variantMap = propertyValue.value<QVariantMap>();

    // create the gotoButton
    QPushButton *gotoButton = new QPushButton();
    gotoButton->setText("...");
    gotoButton->setEnabled(!variantMap.isEmpty());
    gotoButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    const int gotoTextWidth = PM::internal::fontMetricsHorizontalAdvance(QApplication::fontMetrics(), gotoButton->text());
    gotoButton->setMaximumWidth(std::max(gotoTextWidth, 20));

    QObject::connect(gotoButton, &QPushButton::clicked, gotoButton,
                     [variantMap, this, propertyData, parentObjectViewer]() { showListDialog(propertyData, parentObjectViewer, variantMap); });

    // create the itemLabel
    QLabel *itemLabel = new QLabel();
    itemLabel->setFont(QFont("", -1, PM::internal::QFont_Thin, true));
    itemLabel->setText(QString("[Length = %1]").arg(variantMap.count()));
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

bool VariantMapPropertyEditor::showListDialog(const PropertyData &propertyData, QObjectViewer *parentObjectViewer, const QVariantMap &map) const
{
    QDialog dialog;
    IPropertyEditorPrivate::applyPropertiesDialogDefaultStyle(dialog, propertyData.name);

    QTreeWidget treeWidget;
    IPropertyEditorPrivate::applyPropertiesTreeWidgetDefaultStyle(treeWidget);
    IPropertyEditorPrivate::setPropertiesTreeWidgetDefaultContextMenus(parentObjectViewer, treeWidget);

    // Value
    QTreeWidgetItem *rootValueItem = new QTreeWidgetItem(&treeWidget);
    rootValueItem->setExpanded(true);
    rootValueItem->setText(0, PM::internal::getMetaTypeName<QVariantList>());
    rootValueItem->setText(2, QString::number(map.size()));

    IPropertyEditorPtr variantTypeEditor;

    for (const QString &key : map.keys())
    {
        const QVariant &currentElement = map[key];

        variantTypeEditor = VariantPropertyEditorPrivate::getPropertyEditorForValue(currentElement, parentObjectViewer);

        QTreeWidgetItem *valueItem =
            variantTypeEditor->createPropertyTreeItem(propertyData, treeWidget, rootValueItem, currentElement, parentObjectViewer);
        valueItem->takeChildren();
        valueItem->setText(0, key);
        valueItem->setText(1, currentElement.typeName());
    }

    dialog.layout()->addWidget(&treeWidget);
    IPropertyEditorPrivate::setOptimumPropertiesDialogSize(dialog, treeWidget);

    if (!dialog.exec())
        return false;

    return true;
}
