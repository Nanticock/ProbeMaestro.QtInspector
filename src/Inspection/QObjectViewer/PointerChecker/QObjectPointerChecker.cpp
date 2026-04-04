#include "QObjectPointerChecker.h"

#include "QMetaObjectPointerChecker.h"

template <>
bool QObjectPointerChecker::performAdvancedChecks() const
{
    if (!isReadableAddress(d_ptr.data(), sizeof(QObjectData)))
        return false;

    if (d_ptr->parent && !isReadableAddress(d_ptr->parent))
        return false;

    if (d_ptr->q_ptr && !isReadableAddress(d_ptr->q_ptr, sizeof(QObject)))
        return false;

    if (d_ptr->metaObject && !QMetaObjectPointerChecker::isValid(d_ptr->metaObject))
        return false;

    return true;
}
