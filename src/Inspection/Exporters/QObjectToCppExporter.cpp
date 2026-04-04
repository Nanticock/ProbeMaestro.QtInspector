#include "QObjectToCppExporter.h"
#include "QObjectToCppExporter_p.h"

#include <QDebug>
#include <QDir>
#include <QMetaEnum>

namespace
{
// enum template
const char ENUM_TEMPLATE_NAMESPACE_NAME_KEY[] = "{ENUM_NAME}";
const char ENUM_TEMPLATE_NAMESPACE_BODY_KEY[] = "{ENUM_BODY}";

// namespace template
const char NAMESPACE_SEPARATOR_KEY[] = "::";
const char NAMESPACE_TEMPLATE_NAMESPACE_NAME_KEY[] = "{NAMESPACE_NAME}";
const char NAMESPACE_TEMPLATE_NAMESPACE_BODY_KEY[] = "{NAMESPACE_BODY}";

// class template
const char CLASS_TEMPLATE_CLASS_NAME_KEY[] = "{CLASS_NAME}";
const char CLASS_TEMPLATE_PUBLIC_SECTION_KEY[] = "{PUBLIC_SECTION}";
const char CLASS_TEMPLATE_PRIVATE_SECTION_KEY[] = "{PRIVATE_SECTION}";
const char CLASS_TEMPLATE_INHERITANCE_INFO_KEY[] = "{INHERITANCE_INFO}";
const char CLASS_TEMPLATE_PROTECTED_SECTION_KEY[] = "{PROTECTED_SECTION}";
} // namespace

const QString QObjectToCppExporterPrivate::s_enumTemplate = "enum {ENUM_NAME}\n"
                                                            "{\n"
                                                            "{ENUM_BODY}\n"
                                                            "};\n";

const QString QObjectToCppExporterPrivate::s_namespaceTemplate = "namespace {NAMESPACE_NAME}\n"
                                                                 "{\n"
                                                                 "{NAMESPACE_BODY}\n"
                                                                 "} // {NAMESPACE_NAME}";

const QString QObjectToCppExporterPrivate::s_headerFileTemplate = "class {CLASS_NAME} "
                                                                  "{INHERITANCE_INFO}\n"
                                                                  "{\n"
                                                                  "\tQ_OBJECT\n"
                                                                  "public:\n"
                                                                  "{PUBLIC_SECTION}\n"
                                                                  "protected:\n"
                                                                  "{PROTECTED_SECTION}\n"
                                                                  "private:\n"
                                                                  "{PRIVATE_SECTION}\n"
                                                                  "};";

QString QObjectToCppExporterPrivate::classNameToFileName(const QString &className, bool useNamespacesAsSubDirectories)
{
    QString result = className;

    if (useNamespacesAsSubDirectories)
        return result.replace(NAMESPACE_SEPARATOR_KEY, QDir::separator());

    return result.replace(NAMESPACE_SEPARATOR_KEY, "_");
}

QString QObjectToCppExporterPrivate::incloseInNameSpace(const QString &namespaceName, const QString &body, bool preserveEmptyNameSpaces)
{
    if (namespaceName.isEmpty())
    {
        if (!preserveEmptyNameSpaces)
            return body;
        else
            return incloseInNameSpace_impl("", body);
    }

    QString parentNamesSpace;
    QString currentNameSpace;

    getNameSpaceNameFromLongClassName(namespaceName, &currentNameSpace, &parentNamesSpace);

    QString result = incloseInNameSpace_impl(currentNameSpace, body);
    return incloseInNameSpace(parentNamesSpace, result, preserveEmptyNameSpaces);
}

QString QObjectToCppExporterPrivate::incloseInNameSpace_impl(const QString &namespaceName, const QString &body)
{
    QString result = s_namespaceTemplate;

    result = result.replace(NAMESPACE_TEMPLATE_NAMESPACE_NAME_KEY, namespaceName);
    result = result.replace(NAMESPACE_TEMPLATE_NAMESPACE_BODY_KEY, body);

    return result;
}

void QObjectToCppExporterPrivate::getNameSpaceNameFromLongClassName(const QString &longClassName, QString *className, QString *namespaceName)
{
    int lastNameSpaceIndex = longClassName.lastIndexOf(NAMESPACE_SEPARATOR_KEY);
    if (lastNameSpaceIndex == -1)
    {
        if (className != nullptr)
            *className = longClassName;

        if (namespaceName != nullptr)
            *namespaceName = "";

        return;
    }

    QString parentNamesSpace = longClassName.left(lastNameSpaceIndex);
    QString _className = longClassName.right(longClassName.length() - (lastNameSpaceIndex + sizeof(NAMESPACE_SEPARATOR_KEY) - 1));

    if (className != nullptr)
        *className = _className.trimmed();

    if (namespaceName != nullptr)
        *namespaceName = parentNamesSpace.trimmed();

    return;
}

QString QObjectToCppExporterPrivate::generateEnum(const QMetaEnum &_enum)
{
    QString result = s_enumTemplate;

    result = result.replace(ENUM_TEMPLATE_NAMESPACE_NAME_KEY, _enum.name());

    QString body;
    for (int i = 0; i < _enum.keyCount(); i++)
        body += QString("\t%1 = 0x%2,\n").arg(_enum.key(i)).arg(_enum.value(i), 0, 16);

    result = result.replace(ENUM_TEMPLATE_NAMESPACE_BODY_KEY, "\t" + body.trimmed());

    if (!_enum.isScoped())
        result += QString("Q_ENUM(%1)\n").arg(_enum.name());
    else
        result += QString("Q_ENUM_NS(%1)\n").arg(_enum.name());

    if (_enum.isFlag())
        result += QString(_enum.isScoped() ? "Q_FLAG_NS(%1)\n" : "Q_FLAG(%1)\n").arg(_enum.name());

    return result;
}

QString QObjectToCppExporterPrivate::generateEnums(const QMetaObject *_class)
{
    if (_class == nullptr)
        return "";

    QString result;

    for (int i = 0; i < _class->enumeratorCount(); i++)
        result += generateEnum(_class->enumerator(i)) + "\n";

    return result;
}

QString QObjectToCppExporterPrivate::generatePublicSection(const QMetaObject *_class)
{
    if (_class == nullptr)
        return "";

    QString result;

    result += generateEnums(_class);

    return result;
}

QString QObjectToCppExporterPrivate::generateInheritanceInfo(const QMetaObject *superclass)
{
    return generateInheritanceInfo(superclass == nullptr ? "" : superclass->className());
}

QString QObjectToCppExporterPrivate::generateInheritanceInfo(const QString &superclassName)
{
    if (superclassName.isEmpty())
        return "\n";

    return ": public " + superclassName;
}

bool QObjectToCppExporter::exportToByteArray(const QVariant &value, QByteArray &output, const Options *options)
{
    QObjectToCppExportOptions defaultOptions;

    const QObjectToCppExportOptions *_optionsPtr = dynamic_cast<const QObjectToCppExportOptions *>(options);

    if (!_optionsPtr)
        _optionsPtr = &defaultOptions;

    if (value.canConvert<QObject *>())
        return _exportToByteArray(value.value<QObject *>(), output, *_optionsPtr);

    if (value.canConvert<const QMetaObject *>())
        return _exportToByteArray(value.value<const QMetaObject *>(), output, *_optionsPtr);

    return false;
}

bool QObjectToCppExporter::_exportToByteArray(const QObject *value, QByteArray &output, const QObjectToCppExportOptions &options)
{
    if (value == nullptr)
        return false;

    bool result = _exportToByteArray(value->metaObject(), output, options);

    // TODO: implement stuff like properties default values and such things

    return result;
}

bool QObjectToCppExporter::_exportToByteArray(const QMetaObject *value, QByteArray &output, const QObjectToCppExportOptions &options)
{
    if (value == nullptr)
        return false;

    QString className;
    QString namespaceName;
    QObjectToCppExporterPrivate::getNameSpaceNameFromLongClassName(value->className(), &className, &namespaceName);

    QString result = QObjectToCppExporterPrivate::s_headerFileTemplate;

    result = result.replace(CLASS_TEMPLATE_CLASS_NAME_KEY, className);
    result = result.replace(CLASS_TEMPLATE_INHERITANCE_INFO_KEY, QObjectToCppExporterPrivate::generateInheritanceInfo(value->superClass()));
    result = result.replace(CLASS_TEMPLATE_PUBLIC_SECTION_KEY, QObjectToCppExporterPrivate::generatePublicSection(value));

    // TODO: 1. save class info
    // TODO: 2. save public/private/protected methods
    // TODO: 3. save public/private/protected slots
    // TODO: 4. save signals
    // TODO: 5. save properties

    result = QObjectToCppExporterPrivate::incloseInNameSpace(namespaceName, result);

    output = result.toUtf8();

    return true;
}
