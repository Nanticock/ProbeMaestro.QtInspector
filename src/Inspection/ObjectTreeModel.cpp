#include "ObjectTreeModel.h"

#include "QObjectViewer/ObjectLocator/ObjectLocator.h"

#include <QApplication>
#include <QGuiApplication>
#include <QWidget>
#include <QWindow>

#include <memory>

ObjectTreeModel::ObjectTreeModel(QObject *parent) : QAbstractItemModel(parent), m_root(std::unique_ptr<ObjectNode>(new ObjectNode()))
{
}

void ObjectTreeModel::refresh()
{
    // NOTE: refresh only updates the top most level, lazy loading does the rest

    beginResetModel();

    m_root->children.clear();

    // QWidget windows
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget *w : widgets)
    {
        auto node = std::unique_ptr<ObjectNode>(new ObjectNode());
        node->object = w;
        node->parent = m_root.get();
        node->fetched = false;

        m_root->children.push_back(std::move(node));
    }

    // QWindow windows (for QML, etc.)
    const auto windows = QGuiApplication::topLevelWindows();
    for (QWindow *w : windows)
    {
        // Avoid duplicates from QWidget-backed windows
        if (w->parent())
            continue;

        auto node = std::unique_ptr<ObjectNode>(new ObjectNode());
        node->object = w;
        node->parent = m_root.get();
        node->fetched = false;

        m_root->children.push_back(std::move(node));
    }

    endResetModel();
}

bool ObjectTreeModel::canFetchMore(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return false; // root already populated

    ObjectNode *node = static_cast<ObjectNode *>(parent.internalPointer());

    if (!node || node->fetched)
        return false;

    QObject *obj = node->object.data();
    return obj && !obj->children().isEmpty();
}

void ObjectTreeModel::fetchMore(const QModelIndex &parent)
{
    ObjectNode *node = parent.isValid() ? static_cast<ObjectNode *>(parent.internalPointer()) : m_root.get();

    if (!node || node->fetched)
        return;

    QObject *obj = node->object.data();
    if (!obj)
        return;

    const auto children = obj->children();

    if (children.isEmpty())
    {
        node->fetched = true;
        return;
    }

    beginInsertRows(parent, 0, children.size() - 1);

    for (QObject *child : children)
    {
        auto childNode = std::unique_ptr<ObjectNode>(new ObjectNode());
        childNode->object = child;
        childNode->parent = node;
        childNode->fetched = false;

        node->children.push_back(std::move(childNode));
    }

    node->fetched = true;

    endInsertRows();
}

QModelIndex ObjectTreeModel::index(int row, int column, const QModelIndex &parentIdx) const
{
    if (!hasIndex(row, column, parentIdx))
        return {};

    ObjectNode *parentNode = parentIdx.isValid() ? static_cast<ObjectNode *>(parentIdx.internalPointer()) : m_root.get();

    // DO NOT fake anything here
    if (row < 0 || row >= parentNode->children.size())
        return {};

    return createIndex(row, column, parentNode->children[row].get());
}

QModelIndex ObjectTreeModel::parent(const QModelIndex &childIdx) const
{
    if (!childIdx.isValid())
        return {};

    auto node = static_cast<ObjectNode *>(childIdx.internalPointer());
    auto parentNode = node->parent;

    if (!parentNode || parentNode == m_root.get())
        return {};

    auto grandParent = parentNode->parent;

    int row = 0;
    for (int i = 0; i < grandParent->children.size(); ++i)
    {
        if (grandParent->children[i].get() == parentNode)
        {
            row = i;
            break;
        }
    }

    return createIndex(row, 0, parentNode);
}

int ObjectTreeModel::rowCount(const QModelIndex &parentIdx) const
{
    ObjectNode *node = parentIdx.isValid() ? static_cast<ObjectNode *>(parentIdx.internalPointer()) : m_root.get();

    // ROOT: always return actual children
    if (node == m_root.get())
        return int(node->children.size());

    // Already fetched: return real children
    if (node->fetched)
        return int(node->children.size());

    // Not fetched yet: check if expandable
    QObject *obj = node->object.data();
    if (obj && !obj->children().isEmpty())
        return 1; // fake row to show expand arrow

    return 0;
}

int ObjectTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant ObjectTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    auto node = static_cast<ObjectNode *>(index.internalPointer());

    switch (role)
    {
    case Qt::DisplayRole:
        return formatObject(node->object.data());

    case Qt::ForegroundRole:
    {
        if (node->object.isNull())
            return QBrush(Qt::gray);

        QObject *obj = node->object.data();

        if (auto *w = qobject_cast<QWidget *>(obj))
        {
            if (!w->isVisible())
                return QBrush(Qt::gray);
        }

        if (auto *win = qobject_cast<QWindow *>(obj))
        {
            if (!win->isVisible())
                return QBrush(Qt::gray);
        }

        return {};
    }

    case ObjectRole:
        return QVariant::fromValue(node->object.data());
    }

    return {};
}

QObject *ObjectTreeModel::objectFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    auto *node = static_cast<ObjectNode *>(index.internalPointer());
    return node ? node->object.data() : nullptr;
}

QString ObjectTreeModel::formatObject(QObject *obj) const
{
    if (!obj)
        return "<deleted>";

    QString objectName = obj->objectName();
    QString objectId = ObjectLocator::getQmlObjectId(obj);
    QString className = obj->metaObject()->className();

    return QString("[%1]: \"%2\"").arg(className, !objectId.isEmpty() ? objectId : objectName);
}
