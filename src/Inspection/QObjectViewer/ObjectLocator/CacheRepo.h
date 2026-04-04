#ifndef CACHEREPO_H
#define CACHEREPO_H

#include <QHash>

class CacheRepo
{
public:
    CacheRepo() = default;

    void clear();

    bool hasObject(const void *object);
    bool hasString(const QString &string);

    template <typename T>
    T getObject(const QString &string);
    void *getObject(const QString &string);
    QString getObjectFirstString(const void *object);
    QSet<QString> getObjectStrings(const void *object);

    void add(const void *object, const QString &string);

private:
    QHash<QString, void *> m_stringObjectMap;
    QHash<void *, QSet<QString>> m_objectStringMap;
};

template <typename T>
inline T CacheRepo::getObject(const QString &string)
{
    return reinterpret_cast<T>(getObject(string));
}

#endif // CACHEREPO_H
