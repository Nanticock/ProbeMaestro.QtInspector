#include "StringPropertyEditor.h"

bool StringPropertyEditor::canHandleType(const QMetaType &type) const
{
    return type == QMetaType::fromType<QString>();
}

bool StringPropertyEditor::canHandleValue(const QVariant &value) const
{
    return value.canConvert<QString>();
}
