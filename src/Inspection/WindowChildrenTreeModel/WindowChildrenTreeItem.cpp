#include "WindowChildrenTreeItem.h"

WindowChildrenTreeItem::WindowChildrenTreeItem(const QList<QVariant> &data, WindowChildrenTreeItem *parentItem) :
    m_itemData(data),
    m_parentItem(parentItem)
{
}

WindowChildrenTreeItem::~WindowChildrenTreeItem()
{
    qDeleteAll(m_childItems);
}

void WindowChildrenTreeItem::appendChild(WindowChildrenTreeItem *child)
{
    m_childItems.append(child);
}

WindowChildrenTreeItem *WindowChildrenTreeItem::child(int row)
{
    return m_childItems.value(row);
}

int WindowChildrenTreeItem::childCount() const
{
    return m_childItems.count();
}

int WindowChildrenTreeItem::columnCount() const
{
    return m_itemData.count();
}

QVariant WindowChildrenTreeItem::data(int column) const
{
    return m_itemData[column];
}

int WindowChildrenTreeItem::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<WindowChildrenTreeItem *>(this));

    return 0;
}

WindowChildrenTreeItem *WindowChildrenTreeItem::parentItem()
{
    return m_parentItem;
}
