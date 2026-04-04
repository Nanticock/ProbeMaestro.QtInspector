#ifndef STRINGPROPERTYEDITOR_H
#define STRINGPROPERTYEDITOR_H

#include "IPropertyEditor.h"

class StringPropertyEditor : public IPropertyEditor
{
public:
    virtual bool canHandleType(const QMetaType &type) const override;
    virtual bool canHandleValue(const QVariant &value) const override;
};

#endif // STRINGPROPERTYEDITOR_H
