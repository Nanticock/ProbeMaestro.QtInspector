#include "ColorPropertyEditor.h"

#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

#include <QObjectViewer/QObjectViewer.h>

bool ColorPropertyEditor::canHandleType(const QMetaType &type) const
{
    return type.id() == qMetaTypeId<QColor>();
}

QTreeWidgetItem *ColorPropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                             QTreeWidgetItem *parentItem, const QVariant &propertyValue,
                                                             QObjectViewer *parentObjectViewer) const
{
    QTreeWidgetItem *propertyItem =
        IPropertyEditor::createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, propertyValue, parentObjectViewer);

    if (!propertyValue.isValid())
        return propertyItem;

    // create the gotoButton
    QPushButton *editColorButton = new QPushButton();
    editColorButton->setText("...");
    editColorButton->setEnabled(propertyData.isWritable && parentObjectViewer != nullptr);
    editColorButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    editColorButton->setMaximumWidth(std::max(QApplication::fontMetrics().horizontalAdvance(editColorButton->text()), 20));

    QColor value = propertyValue.value<QColor>();

    // create the itemLabel
    QLabel *itemLabel = new QLabel();
    itemLabel->setText(colorToString(value));

    QLabel *colorLabel = new QLabel();
    colorLabel->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
    // FIXME: Find a cleaner way to set the color
    setWidgetBackgroundStyleSheet(colorLabel, value);

    // create the itemWidget
    QWidget *itemWidget = new QWidget();
    itemWidget->setLayout(new QHBoxLayout());
    itemWidget->layout()->setMargin(0);
    itemWidget->layout()->addWidget(colorLabel);
    itemWidget->layout()->addWidget(itemLabel);
    itemWidget->layout()->addWidget(editColorButton);

    colorLabel->setMaximumWidth(std::min(editColorButton->width(), editColorButton->height()));
    colorLabel->setMaximumHeight(colorLabel->maximumWidth());

    propertyItem->setText(2, propertyValue.toString());
    propertyItem->setForeground(2, QBrush(Qt::transparent));

    parentTreeWidget.setItemWidget(propertyItem, 2, itemWidget);

    if (parentObjectViewer == nullptr)
        return propertyItem;

    // we only need to do this connection if we have a parent ObjectViewer
    // because this is the only sitiuation where we can change the a QObject value by using its setProperty() function
    QObject::connect(editColorButton, &QPushButton::clicked, editColorButton,
                     [parentObjectViewer, propertyData, itemLabel, colorLabel]()
                     {
                         QObject *currentObject = parentObjectViewer->currentObject();
                         if(currentObject == nullptr)
                             return;

                         QColor value = currentObject->property(propertyData.name).value<QColor>();

                         QColorDialog dialog;
                         dialog.setCurrentColor(value);

                         if (!dialog.exec())
                             return;

                         value = dialog.currentColor();
                         currentObject->setProperty(propertyData.name, value);

                         // update property editor view
                         // FIXME: find a way to wrap this update procedure into a separate function

                         itemLabel->setText(colorToString(value));
                         setWidgetBackgroundStyleSheet(colorLabel, value);
                     });

    return propertyItem;
}

QString ColorPropertyEditor::colorToString(const QColor &color)
{
    return QString("[%1, %2, %3, %4]").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
}

void ColorPropertyEditor::setWidgetBackgroundStyleSheet(QWidget *widget, const QColor &backgroundColor)
{
    QString styleSheet = "background-color: rgba(%1, %2, %3, %4)";
    styleSheet = styleSheet.arg(backgroundColor.red());
    styleSheet = styleSheet.arg(backgroundColor.green());
    styleSheet = styleSheet.arg(backgroundColor.blue());
    styleSheet = styleSheet.arg(backgroundColor.alpha());

    widget->setStyleSheet(styleSheet);
    widget->update();
}
