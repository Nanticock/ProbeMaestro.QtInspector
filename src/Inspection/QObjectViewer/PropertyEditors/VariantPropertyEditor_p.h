#ifndef VARIANTPROPERTYEDITOR_P_H
#define VARIANTPROPERTYEDITOR_P_H

#include "VariantPropertyEditor.h"

class VariantPropertyEditorPrivate
{
public:
    static QObject *variantToQObject(const QVariant &value);
    static IPropertyEditorPtr getPropertyEditorForValue(const QVariant &value, QObjectViewer *parentObjectViewer = nullptr);
};

#endif // VARIANTPROPERTYEDITOR_P_H
