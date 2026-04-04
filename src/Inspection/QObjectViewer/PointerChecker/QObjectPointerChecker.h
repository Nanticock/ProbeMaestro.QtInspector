#ifndef QOBJECTPOINTERCHECKER_H
#define QOBJECTPOINTERCHECKER_H

#include "PointerChecker.h"

#include <QObject>

typedef PointerChecker<QObject> QObjectPointerChecker;

template <>
bool QObjectPointerChecker::performAdvancedChecks() const;

#endif // QOBJECTPOINTERCHECKER_H
