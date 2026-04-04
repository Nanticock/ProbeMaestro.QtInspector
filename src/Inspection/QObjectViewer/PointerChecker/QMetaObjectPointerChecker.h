#ifndef QMETAOBJECTPOINTERCHECKER_H
#define QMETAOBJECTPOINTERCHECKER_H

#include "PointerChecker.h"

#include <QMetaObject>

typedef PointerChecker<QMetaObject> QMetaObjectPointerChecker;

template <>
bool QMetaObjectPointerChecker::performAdvancedChecks() const;

#endif // QMETAOBJECTPOINTERCHECKER_H
