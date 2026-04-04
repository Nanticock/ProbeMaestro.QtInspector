#include "WindowChildrenTreeModel.h"

#include "WindowChildrenTreeItem.h"
#include <QObjectViewer/ObjectLocator/ObjectLocator.h>
#include <Compat/MemoryMaps/ObjectsMemoryMap/QtObjectsMemoryMap.h>

#include <QDebug>
#include <QtQml>

WindowChildrenTreeModel::WindowChildrenTreeModel(QObject *parent) : QAbstractItemModel(parent), m_rootItem(nullptr)
{
}

WindowChildrenTreeModel::~WindowChildrenTreeModel()
{
}

WindowChildrenTreeItem *WindowChildrenTreeModel::itemFromIndex(const QModelIndex &index) const
{
    if (!rootObjectIsValid())
        return nullptr;

    if (!index.isValid())
        return nullptr;

    return static_cast<WindowChildrenTreeItem *>(index.internalPointer());
}

int WindowChildrenTreeModel::columnCount(const QModelIndex &parent) const
{
    if (!rootObjectIsValid())
        return 0;

    return m_rootItem->columnCount();
}

QObject *WindowChildrenTreeModel::rootObject() const
{
    if (m_rootItem.isNull())
        return nullptr;

    if (m_rootItem->data(0).isNull() || !m_rootItem->data(0).isValid())
        return nullptr;

    return m_rootItem->data(0).value<QObject *>();
}

void WindowChildrenTreeModel::setRootObject(QObject *rootItem)
{
    if (rootObject() == rootItem)
        return;

    m_rootItem = QSharedPointer<WindowChildrenTreeItem>::create(QVariantList{QVariant::fromValue(rootItem)});
    setupModelData(m_rootItem.data());
}

bool WindowChildrenTreeModel::rootObjectIsValid() const
{
    return rootObject() != nullptr;
}

QVariant WindowChildrenTreeModel::data(const QModelIndex &index, int role) const
{
    if (!rootObjectIsValid())
        return QVariant();

    // Refer to:
    // https://sl.bing.net/budSoIeTTzM
    // https://sl.bing.net/jI1PbxIwwIC

    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    WindowChildrenTreeItem *item = static_cast<WindowChildrenTreeItem *>(index.internalPointer());

    // Get the QObject pointer from the item's data
    QObject *obj = item->data(0).value<QObject *>();

    // Get the object name or an empty string if it is not set
    QString objectName = obj->objectName();
    QString objectId = ObjectLocator::getQmlObjectId(obj);

    // Get the meta-object of the QObject
    const QMetaObject *metaObject = obj->metaObject();

    // Get the class name of the QObject
    QString className = metaObject->className();

    // Return a text that combines the class name and the object name
    return QString("[%1]: \"%2\"").arg(className, !objectId.isEmpty() ? objectId : objectName);
}

Qt::ItemFlags WindowChildrenTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QVariant WindowChildrenTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (!rootObjectIsValid())
        return QVariant();

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_rootItem->data(section);

    return QVariant();
}

QModelIndex WindowChildrenTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!rootObjectIsValid())
        return QModelIndex();

    if (!hasIndex(row, column, parent))
        return QModelIndex();

    WindowChildrenTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem.data();
    else
        parentItem = static_cast<WindowChildrenTreeItem *>(parent.internalPointer());

    WindowChildrenTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex WindowChildrenTreeModel::parent(const QModelIndex &index) const
{
    if (!rootObjectIsValid())
        return QModelIndex();

    if (!index.isValid())
        return QModelIndex();

    WindowChildrenTreeItem *childItem = static_cast<WindowChildrenTreeItem *>(index.internalPointer());
    WindowChildrenTreeItem *parentItem = childItem->parentItem();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int WindowChildrenTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!rootObjectIsValid())
        return 0;

    WindowChildrenTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem.data();
    else
        parentItem = static_cast<WindowChildrenTreeItem *>(parent.internalPointer());

    return parentItem->childCount();
}

void WindowChildrenTreeModel::setupModelData(WindowChildrenTreeItem *parent)
{
    if (!rootObjectIsValid())
        return;

    if (!parent)
        return;

    // Refer to: https://sl.bing.net/HuCP8b5KVg
    // Get the QObject pointer from the parent item's data
    QObject *obj = parent->data(0).value<QObject *>();

    if (!obj)
        return;

    Compat::QtObjectsMemoryMap::addMetaObject(obj->metaObject());

    // Iterate over the children of the QObject
    for (QObject *child : obj->children())
    {
        // Create a list of QVariant to store the data for each column
        QList<QVariant> itemData;

        // Store the QObject pointer as the first column data
        itemData << QVariant::fromValue(child);

        // Store the object name as the second column data
        itemData << child->objectName();

        // Create a new TreeItem with the data and the parent item
        WindowChildrenTreeItem *childItem = new WindowChildrenTreeItem(itemData, parent);

        // Append the child item to the parent item
        parent->appendChild(childItem);

        // Recursively call setupModelData for the child item
        setupModelData(childItem);
    }
}
