#ifndef WINDOWCHILDRENTREEITEM_H
#define WINDOWCHILDRENTREEITEM_H

#include <QList>
#include <QVariant>
#include <QSharedPointer>

class WindowChildrenTreeItem : public QEnableSharedFromThis<WindowChildrenTreeItem>
{
public:
    WindowChildrenTreeItem() = default;
    WindowChildrenTreeItem(const WindowChildrenTreeItem &other) = default;

    explicit WindowChildrenTreeItem(const QList<QVariant> &data, WindowChildrenTreeItem *parentItem = nullptr);
    ~WindowChildrenTreeItem();

    void appendChild(WindowChildrenTreeItem *child);

    WindowChildrenTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    WindowChildrenTreeItem *parentItem();

private:
    QVariantList m_itemData;
    WindowChildrenTreeItem *m_parentItem;
    QList<WindowChildrenTreeItem *> m_childItems;
};
#endif // WINDOWCHILDRENTREEITEM_H
