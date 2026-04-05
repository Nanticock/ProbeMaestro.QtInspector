#include "StringPropertyEditor.h"

#include <compat_Qt.h>

bool StringPropertyEditor::canHandleType(const QMetaType &type) const
{
    return PM::internal::getMetaTypeId(type) == qMetaTypeId<QString>();
}

bool StringPropertyEditor::canHandleValue(const QVariant &value) const
{
    return value.canConvert<QString>();
}
