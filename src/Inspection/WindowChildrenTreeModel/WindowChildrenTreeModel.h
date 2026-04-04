#ifndef WINDOWCHILDRENTREEMODEL_H
#define WINDOWCHILDRENTREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QSharedPointer>
#include <QVariant>

class WindowChildrenTreeItem;

class WindowChildrenTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit WindowChildrenTreeModel(QObject *parent = nullptr);
    ~WindowChildrenTreeModel();

    WindowChildrenTreeItem *itemFromIndex(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

public:
    QObject *rootObject() const;
    void setRootObject(QObject *rootItem);

private:
    bool rootObjectIsValid() const;
    void setupModelData(WindowChildrenTreeItem *parent);

private:
    mutable QSharedPointer<WindowChildrenTreeItem> m_rootItem;
};

#endif // WINDOWCHILDRENTREEMODEL_H
