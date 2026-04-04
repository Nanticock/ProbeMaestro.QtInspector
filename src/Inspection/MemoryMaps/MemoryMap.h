#ifndef MEMORYMAP_H
#define MEMORYMAP_H

#include <string>
#include <unordered_set>

//
// FIXME:   find a better design and move these to a better place
//
// WARNING: these are not part of the public interface.
//          they are just here as a placeholder and will be replaced
//          in the final API design.
//
namespace placeholder
{
std::wstring currentModuleName();
} // namespace placeholder

class MemoryMap
{
public:
    typedef std::unordered_set<std::string> tags_t;

    struct FilterOptions
    {
        FilterOptions();
        FilterOptions(FilterOptions &&other) = default;
        FilterOptions(const FilterOptions &other) = default;
        FilterOptions(const std::string &moduleName, const MemoryMap::tags_t &tags);
        FilterOptions(const std::wstring &moduleName, const MemoryMap::tags_t &tags);

        std::wstring moduleName;
        tags_t tags;
    };

public:
    static bool addAddress(const std::string &name, size_t address, const std::wstring &moduleName = placeholder::currentModuleName(),
                           const tags_t &tags = {});
    template <typename T>
    static bool addAddress(const std::string &name, const T *address, const std::wstring &moduleName = placeholder::currentModuleName(),
                           const tags_t &tags = {});

    template <typename T>
    static T getAddress(const std::string &name);

    static std::unordered_set<std::string> getAllAddresses();
    static std::unordered_set<std::string> getAllAddresses(const FilterOptions &filterOptions);

    static void clear();
    static bool remove(const std::string &name);
    static size_t removeAll(const FilterOptions &filterOptions);

private:
    static void *getAddressImpl(const std::string &name);
};

template <typename T>
inline bool MemoryMap::addAddress(const std::string &name, const T *address, const std::wstring &moduleName, const tags_t &tags)
{
    return addAddress(name, reinterpret_cast<size_t>(address), moduleName, tags);
}

template <typename T>
inline T MemoryMap::getAddress(const std::string &name)
{
    return reinterpret_cast<T>(MemoryMap::getAddressImpl(name));
}

#endif // MEMORYMAP_H
