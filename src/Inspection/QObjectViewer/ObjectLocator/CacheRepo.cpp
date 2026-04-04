#include "CacheRepo.h"

#include <QSet>

void CacheRepo::clear()
{
    m_stringObjectMap.clear();
    m_objectStringMap.clear();
}

bool CacheRepo::hasString(const QString &string)
{
    return m_stringObjectMap.contains(string);
}

bool CacheRepo::hasObject(const void *object)
{
    void *nonConstObject = const_cast<void *>(object);

    return m_objectStringMap.contains(nonConstObject);
}

void *CacheRepo::getObject(const QString &string)
{
    return m_stringObjectMap[string];
}

QString CacheRepo::getObjectFirstString(const void *object)
{
    QSet<QString> strings = getObjectStrings(object);

    if (strings.isEmpty())
        return "";

    return strings.values().first();
}

QSet<QString> CacheRepo::getObjectStrings(const void *object)
{
    void *nonConstObject = const_cast<void *>(object);

    return m_objectStringMap[nonConstObject];
}

void CacheRepo::add(const void *object, const QString &string)
{
    void *nonConstObject = const_cast<void *>(object);

    m_stringObjectMap[string] = nonConstObject;
    m_objectStringMap[nonConstObject] << string;
}
