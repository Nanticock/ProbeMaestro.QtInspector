#include "PointerPropertyEditor.h"

#include "QObjectViewer/QObjectViewer.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaProperty>
#include <QPushButton>

bool PointerPropertyEditor::canHandleType(const QMetaType &type) const
{
    // TODO: redefine this definition

    // can handle any property with a type that is a pointer or inherits from QObject
    const QMetaObject *propertyTypeMetaObject = QMetaType::metaObjectForType(type.id());

    if (!propertyTypeMetaObject)
        return false;

    return propertyTypeMetaObject->inherits(&QObject::staticMetaObject);
}

QTreeWidgetItem *PointerPropertyEditor::createPropertyTreeItem(const PropertyData &propertyData, QTreeWidget &parentTreeWidget,
                                                               QTreeWidgetItem *parentItem, const QVariant &propertyValue,
                                                               QObjectViewer *parentObjectViewer) const
{
    QTreeWidgetItem *propertyItem =
        IPropertyEditor::createPropertyTreeItem(propertyData, parentTreeWidget, parentItem, propertyValue, parentObjectViewer);

    if (!propertyValue.isValid())
        return propertyItem;

    // create the gotoButton
    QPushButton *gotoButton = new QPushButton();
    gotoButton->setEnabled(false);
    gotoButton->setIcon(QIcon(":/resources/icons/goto-icon-red.svg"));
    gotoButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    QString pointerAddress = QString("0x%1");
    QString typeName = propertyValue.typeName();

    if (!propertyValue.isNull())
    {
        bool isQObject = propertyValue.canConvert<QObject *>();
        gotoButton->setEnabled(isQObject);

        // if this is a QObject, set its type name to the QMetaObject class name
        typeName = isQObject ? propertyValue.value<QObject *>()->metaObject()->className() : typeName;

        if (isQObject)
            pointerAddress = pointerAddress.arg(qulonglong(propertyValue.value<QObject *>()), 0, 16);
        else
            pointerAddress = pointerAddress.arg(qulonglong(propertyValue.data()), 0, 16);
    }
    else
    {
        pointerAddress = pointerAddress.arg(0);
    }

    // create the itemLabel
    QLabel *itemLabel = new QLabel();
    itemLabel->setText(QString("%1(%2)").arg(typeName).arg(pointerAddress));

    // create the itemWidget
    QWidget *itemWidget = new QWidget();
    itemWidget->setLayout(new QHBoxLayout());
    itemWidget->layout()->setMargin(0);
    itemWidget->layout()->addWidget(itemLabel);
    itemWidget->layout()->addWidget(gotoButton);

    propertyItem->setText(2, itemLabel->text());
    // hide the text from being displayed by default
    // Obtained from microsoft bing
    propertyItem->setForeground(2, QBrush(Qt::transparent));

    parentTreeWidget.setItemWidget(propertyItem, 2, itemWidget);

    if (parentObjectViewer == nullptr || !parentObjectViewer->currentObjectIsValid())
        return propertyItem;

    // connect the button action only in case a parentObjectViewer was provided
    QObject::connect(gotoButton, &QPushButton::clicked, gotoButton,
                     [parentObjectViewer, pointerAddress]()
                     {
                         QObject *value = reinterpret_cast<QObject *>(size_t(pointerAddress.toULongLong(nullptr, 16)));

                         parentObjectViewer->setCurrentObject(value);
                     });

    return propertyItem;
}
