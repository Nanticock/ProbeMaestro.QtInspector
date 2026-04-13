#ifndef COMPAT_QT_H
#define COMPAT_QT_H

#include <QFontMetrics>
#include <QKeySequence>
#include <QMetaEnum>
#include <QMetaType>
#include <QVector>

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
#define qInfo qDebug
#endif

namespace PM
{
namespace internal
{
    namespace QtPrivate
    {
        template <typename T>
        struct QAddConst
        {
            typedef const T Type;
        };
    } // namespace QtPrivate

    // this adds const to non-const objects (like std::as_const)
    template <typename T>
    inline typename QtPrivate::QAddConst<T>::Type &qAsConst(T &t)
    {
        return t;
    }
    // prevent rvalue arguments:
    template <typename T>
    void qAsConst(const T &&) = delete;

    inline int getMetaTypeId(const QMetaType &type)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        return type.id();
#else
        // NOTE: An ugly hack for old versions of qt that didn't have the id() member function
        struct QMetaTypeData
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            using TypedConstructor = QMetaType::TypedConstructor;
            using TypedDestructor = QMetaType::TypedDestructor;
#else
            using TypedConstructor = QMetaType::Creator;
            using TypedDestructor = QMetaType::Deleter;
#endif

            TypedConstructor m_typedConstructor;
            TypedDestructor m_typedDestructor;
            QMetaType::SaveOperator m_saveOp;
            QMetaType::LoadOperator m_loadOp;
            QMetaType::Constructor m_constructor;
            QMetaType::Destructor m_destructor;
            void *m_extension; // space reserved for future use
            uint m_size;
            uint m_typeFlags;
            uint m_extensionFlags;
            int m_typeId;
            const QMetaObject *m_metaObject;
        };

        const QMetaTypeData &typeData = reinterpret_cast<const QMetaTypeData &>(type);

        return typeData.m_typeId;
#endif
    }

    inline QByteArray getMetaTypeName(int typeId)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        return QMetaType(typeId).name();
#else
        return QMetaType::typeName(typeId);
#endif
    }

    inline QByteArray getMetaTypeName(const QMetaType &metaType)
    {
        const int typeId = getMetaTypeId(metaType);

        return getMetaTypeName(typeId);
    }

    template <typename T>
    inline QByteArray getMetaTypeName()
    {
        const int typeId = qMetaTypeId<T>();

        return PM::internal::getMetaTypeName(typeId);
    }

    namespace detail
    {
        template <typename T, typename InputIterator>
        inline void reserveIfPossible(QVector<T> &vec, InputIterator first, InputIterator last, std::input_iterator_tag)
        {
            Q_UNUSED(vec);
            Q_UNUSED(first);
            Q_UNUSED(last);
        }

        template <typename T, typename InputIterator>
        inline void reserveIfPossible(QVector<T> &vec, InputIterator first, InputIterator last, std::forward_iterator_tag)
        {
            vec.reserve(std::distance(first, last));
        }
    } // namespace detail

    template <typename T, typename InputIterator>
    inline QVector<T> createQVector(InputIterator first, InputIterator last)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        return QVector<T>(first, last);
#else
        QVector<T> result;

        detail::reserveIfPossible(result, first, last, typename std::iterator_traits<InputIterator>::iterator_category());

        for (; first != last; ++first)
            result.append(*first);

        return result;
#endif
    }

    inline bool isScopedEnum(const QMetaEnum &metaEnum)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
        return metaEnum.isScoped();
#else
        return false;
#endif
    }

    inline int fontMetricsHorizontalAdvance(const QFontMetrics &fontMetrics, const QString &text)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        return fontMetrics.horizontalAdvance(text);
#else
        return fontMetrics.width(text);
#endif
    }

    inline bool qMetaObjectInherits(const QMetaObject *metaObject, const QMetaObject *baseMetaObject)
    {
        if (metaObject == nullptr || baseMetaObject == nullptr)
            return false;

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        return metaObject->inherits(baseMetaObject);
#else
        // Manual inheritance check for older Qt versions
        const QMetaObject *mo = metaObject;

        while (mo != nullptr)
        {
            if (mo == baseMetaObject)
                return true;

            mo = mo->superClass();
        }

        return false;
#endif
    }
} // namespace internal
} // namespace PM

#if QT_VERSION <= QT_VERSION_CHECK(5, 5, 0)
inline uint qHash(const QKeySequence &key, uint seed) noexcept
{
    return qHash(key.toString(), seed);
}
#endif

#endif // COMPAT_QT_H
