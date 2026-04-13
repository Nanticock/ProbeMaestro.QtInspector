#include "QMetaObjectPointerChecker.h"

/*
 *  Qt 5.9 structure
 *

    struct { // private data
        const QMetaObject *superdata;
        const QByteArrayData *stringdata;
        const uint *data;
        typedef void (*StaticMetacallFunction)(QObject *, QMetaObject::Call, int, void **);
        StaticMetacallFunction static_metacall;
        const QMetaObject * const *relatedMetaObjects;
        void *extradata; //reserved for future use
    } d;
*/

/*
 *  Qt 5.12 structure
 *

    struct { // private data
        const QMetaObject *superdata;
        const QByteArrayData *stringdata;
        const uint *data;
        typedef void (*StaticMetacallFunction)(QObject *, QMetaObject::Call, int, void **);
        StaticMetacallFunction static_metacall;
        const QMetaObject * const *relatedMetaObjects;
        void *extradata; //reserved for future use
    } d;
*/

template <>
bool QMetaObjectPointerChecker::performAdvancedChecks() const
{
    // =========================
    // 1. BASIC POINTER SAFETY
    // =========================

    if (d.stringdata && !isReadableAddress(d.stringdata))
        return false;

    if (d.data && !isReadableAddress(d.data))
        return false;

    if (d.static_metacall && !isReadableAddress(reinterpret_cast<const void *>(d.static_metacall)))
        return false;

    // =========================
    // 2. RELATED METAOBJECTS (pre Qt 5.15 handling)
    // =========================

    if (d.relatedMetaObjects)
    {
        if (!isReadableAddress(d.relatedMetaObjects))
            return false;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)

        // =========================
        // Qt 5.15+ safe assumption
        // =========================
        const QMetaObject * const *array = d.relatedMetaObjects;

        if (array[0] && !isReadableAddress(array[0]))
            return false;

#else
        // ======================================================
        // Qt <= 5.14: COMPLETELY UNSAFE MEMORY (best-effort scan)
        // ======================================================

        const QMetaObject * const *array = reinterpret_cast<const QMetaObject * const *>(d.relatedMetaObjects);

        // We CANNOT trust termination rules → use bounded scan
        constexpr int MAX_SCAN = 16;

        for (int i = 0; i < MAX_SCAN; ++i)
        {
            const void *entryPtr = nullptr;

            // avoid direct dereference until validated
            if (!isReadableAddress(&array[i]))
                break;

            entryPtr = array[i];

            if (!entryPtr)
                break;

            if (!isReadableAddress(entryPtr))
                return false;
        }
#endif
    }

    // =========================
    // 3. EXTRADATA SAFETY
    // =========================

    if (d.extradata && !isReadableAddress(d.extradata))
        return false;

    return true;
}
