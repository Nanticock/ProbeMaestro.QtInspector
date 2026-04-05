#include "QMetaObjectPointerChecker.h"

template <>
bool QMetaObjectPointerChecker::performAdvancedChecks() const
{
    // TODO: we need more checks on the data and string data structs

    if (d.stringdata && !isReadableAddress(d.stringdata))
        return false;

    if (d.data && !isReadableAddress(d.data))
        return false;

    if (d.static_metacall && !isReadableAddress(d.static_metacall))
        return false;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0) // FIXME: check the correct version of qt to do this on
    if (d.relatedMetaObjects && !isReadableAddress(d.relatedMetaObjects, sizeof(QMetaObject::SuperData)))
        return false;

    if (d.relatedMetaObjects && d.relatedMetaObjects->direct && !isValid(d.relatedMetaObjects->direct))
        return false;

    if (d.extradata && !isReadableAddress(d.extradata))
        return false;
#endif

    return true;
}
