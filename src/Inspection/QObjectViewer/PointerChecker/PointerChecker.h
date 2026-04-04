#ifndef POINTERCHECKER_H
#define POINTERCHECKER_H

#include <QDebug>

struct BasicPointerChecker
{
    static bool isReadableAddress(size_t address, size_t size = sizeof(size_t));
    static bool isReadableAddress(const void *address, size_t size = sizeof(size_t));
};

template <typename T>
struct PointerChecker : public T
{
    static bool isValid(size_t ptr);
    static bool isValid(const void *ptr);

    static bool isReadableAddress(size_t address, size_t size = sizeof(size_t));
    static bool isReadableAddress(const void *address, size_t size = sizeof(size_t));

protected:
    bool validate() const;
    bool performAdvancedChecks() const;
};

template <typename T>
inline bool PointerChecker<T>::isValid(size_t ptr)
{
    return isValid(reinterpret_cast<const void *>(ptr));
}

template <typename T>
inline bool PointerChecker<T>::isValid(const void *ptr)
{
    if (!isReadableAddress(ptr))
        return false;

    return reinterpret_cast<const PointerChecker<T> *>(ptr)->validate();
}

template <typename T>
inline bool PointerChecker<T>::isReadableAddress(size_t address, size_t size)
{
    return BasicPointerChecker::isReadableAddress(address, size);
}

template <typename T>
inline bool PointerChecker<T>::isReadableAddress(const void *address, size_t size)
{
    return BasicPointerChecker::isReadableAddress(address, size);
}

template <typename T>
inline bool PointerChecker<T>::validate() const
{
    if (!isReadableAddress(this, sizeof(T)))
        return false;

    if (!performAdvancedChecks())
        return false;

    if (!dynamic_cast<const T *>(this))
        return false;

    return true;
}

template <typename T>
bool PointerChecker<T>::performAdvancedChecks() const
{
    return true;
}

#endif // POINTERCHECKER_H
