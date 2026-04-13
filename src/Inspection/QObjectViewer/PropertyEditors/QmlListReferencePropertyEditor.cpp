#include "QmlListReferencePropertyEditor.h"

#include "QObjectViewer/QObjectViewer.h"

#include <compat_Qt.h>

#include <QApplication>
#include <QDialog>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>

bool QmlListReferencePropertyEditor::canHandleType(const QMetaType &type) const
{
    return PM::internal::getMetaTypeId(type) == qMetaTypeId<QQmlListReference>();
}

bool QmlListReferencePropertyEditor::canHandleValue(const QVariant &value) const
{
    return value.canConvert<QQmlListReference>();
}

QTreeWidgetItem *QmlListReferencePropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                                        QTreeWidgetItem *parentItem, const QVariant &propertyValue,
                                                                        QObjectViewer *parentObjectViewer) const
{
    QTreeWidgetItem *propertyItem =
        IPropertyEditor::createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, propertyValue, parentObjectViewer);

    if (!propertyValue.isValid())
        return propertyItem;

    QQmlListReference qmlListReference = propertyValue.value<QQmlListReference>();

    // create the gotoButton
    QPushButton *gotoButton = new QPushButton();
    gotoButton->setText("...");
    gotoButton->setEnabled(qmlListReference.count() > 0);
    gotoButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    const int gotoTextWidth = PM::internal::fontMetricsHorizontalAdvance(QApplication::fontMetrics(), gotoButton->text());
    gotoButton->setMaximumWidth(std::max(gotoTextWidth, 20));

    // create the itemLabel
    QLabel *itemLabel = new QLabel();
    itemLabel->setFont(QFont("", -1, QFont::Thin, true));
    itemLabel->setText(
        QString("QQmlReferenceList<%1> [Length = %2]").arg(qmlListReference.listElementType()->className()).arg(qmlListReference.count()));
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

    QObject::connect(gotoButton, &QPushButton::clicked, gotoButton, [qmlListReference, this, propertyData, parentObjectViewer]()
                     { showListDialog(propertyData, parentObjectViewer, qmlListReference); });

    return propertyItem;
}

bool QmlListReferencePropertyEditor::showListDialog(const PropertyData &propertyData, QObjectViewer *parentObjectViewer,
                                                    const QQmlListReference &list) const
{
    if (!list.isValid() || !list.isReadable())
        return false;

    QVariantList variantList;

    for (int i = 0; i < list.count(); i++)
    {
        QVariant variant = QVariant::fromValue(list.at(i));
        variantList.append(variant);
    }

    return showListDialog(propertyData, parentObjectViewer, variantList);
}

bool QmlListReferencePropertyEditor::showListDialog(const PropertyData &propertyData, QObjectViewer *parentObjectViewer,
                                                    const QVariantList &list) const
{
    return VariantListPropertyEditor::showListDialog(propertyData, parentObjectViewer, list);
}
