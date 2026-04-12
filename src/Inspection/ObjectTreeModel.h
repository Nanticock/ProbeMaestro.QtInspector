#ifndef OBJECTTREEMODEL_H
#define OBJECTTREEMODEL_H

#include <QAbstractItemModel>
#include <QPointer>

#include <memory>

//
// NOTES: This Model is designed to use lazy loading to improve performance with large scale applications
//
class ObjectTreeModel : public QAbstractItemModel
{
    Q_OBJECT

private:
    struct ObjectNode
    {
        QPointer<QObject> object;
        ObjectNode *parent = nullptr;
        std::vector<std::unique_ptr<ObjectNode>> children;

        // Required for lazy-loading
        bool fetched = false;
    };

public:
    enum Roles
    {
        ObjectRole = Qt::UserRole + 1,
    };

public:
    explicit ObjectTreeModel(QObject *parent = nullptr);

    void refresh();
    QObject *objectFromIndex(const QModelIndex &index) const;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    // Lazy loading functionality
    void fetchMore(const QModelIndex &parent) override;
    bool canFetchMore(const QModelIndex &parent) const override;

private:
    void buildTree(ObjectNode *parentNode, QObject *obj);
    QString formatObject(QObject *obj) const;

private:
    std::unique_ptr<ObjectNode> m_root;
};

#endif // OBJECTTREEMODEL_H
