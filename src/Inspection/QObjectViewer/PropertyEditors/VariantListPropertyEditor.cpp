#include "VariantListPropertyEditor.h"

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

bool VariantListPropertyEditor::canHandleType(const QMetaType &type) const
{
    return PM::internal::getMetaTypeId(type) == qMetaTypeId<QVariantList>() || PM::internal::getMetaTypeId(type) == qMetaTypeId<QStringList>();
}

bool VariantListPropertyEditor::canHandleValue(const QVariant &value) const
{
    return value.canConvert<QVariantList>() || value.canConvert<QStringList>();
}

QTreeWidgetItem *VariantListPropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                                   QTreeWidgetItem *parentItem, const QVariant &propertyValue,
                                                                   QObjectViewer *parentObjectViewer) const
{
    QTreeWidgetItem *propertyItem =
        IPropertyEditor::createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, propertyValue, parentObjectViewer);

    if (!propertyValue.isValid())
        return propertyItem;

    QVariantList variantList = propertyValue.value<QVariantList>();

    // create the gotoButton
    QPushButton *gotoButton = new QPushButton();
    gotoButton->setText("...");
    gotoButton->setEnabled(!variantList.isEmpty());
    gotoButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    const int gotoTextWidth = PM::internal::fontMetricsHorizontalAdvance(QApplication::fontMetrics(), gotoButton->text());
    gotoButton->setMaximumWidth(std::max(gotoTextWidth, 20));

    QObject::connect(gotoButton, &QPushButton::clicked, gotoButton,
                     [variantList, this, propertyData, parentObjectViewer]() { showListDialog(propertyData, parentObjectViewer, variantList); });

    // create the itemLabel
    QLabel *itemLabel = new QLabel();
    itemLabel->setFont(QFont("", -1, QFont::Normal, true));
    itemLabel->setText(QString("[Length = %1]").arg(variantList.count()));
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

bool VariantListPropertyEditor::showListDialog(const PropertyData &propertyData, QObjectViewer *parentObjectViewer, const QVariantList &list) const
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
    rootValueItem->setText(2, QString::number(list.count()));

    IPropertyEditorPtr variantTypeEditor;

    for (int i = 0; i < list.count(); i++)
    {
        const QVariant &currentElement = list[i];

        variantTypeEditor = VariantPropertyEditorPrivate::getPropertyEditorForValue(currentElement, parentObjectViewer);

        QTreeWidgetItem *valueItem =
            variantTypeEditor->createPropertyTreeItem(propertyData, treeWidget, rootValueItem, currentElement, parentObjectViewer);
        valueItem->takeChildren();
        valueItem->setText(0, QString::number(i));
        valueItem->setText(1, currentElement.typeName());
    }

    dialog.layout()->addWidget(&treeWidget);
    IPropertyEditorPrivate::setOptimumPropertiesDialogSize(dialog, treeWidget);

    if (!dialog.exec())
        return false;

    return true;
}
