#ifndef QOBJECTTOCPPEXPORTER_P_H
#define QOBJECTTOCPPEXPORTER_P_H

#include "QObjectToCppExporter.h"

class QObjectToCppExporterPrivate
{
public:
    static QString generateEnum(const QMetaEnum &_enum);
    static QString generateEnums(const QMetaObject *_class);

    static QString generatePublicSection(const QMetaObject *_class);

    static QString generateInheritanceInfo(const QMetaObject *superclass);
    static QString generateInheritanceInfo(const QString &superclassName);

    static QString classNameToFileName(const QString &className, bool useNamespacesAsSubDirectories = false);

    static QString incloseInNameSpace(const QString &namespaceName, const QString &body, bool preserveEmptyNameSpaces = false);
    static void getNameSpaceNameFromLongClassName(const QString &longClassName, QString *className = nullptr, QString *namespaceName = nullptr);

private:
    static QString incloseInNameSpace_impl(const QString &namespaceName, const QString &body);

public:
    static const QString s_enumTemplate;
    static const QString s_namespaceTemplate;
    static const QString s_headerFileTemplate;
};

#endif // QOBJECTTOCPPEXPORTER_P_H
