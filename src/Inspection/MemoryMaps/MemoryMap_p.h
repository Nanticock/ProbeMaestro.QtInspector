#ifndef MEMORYMAP_P_H
#define MEMORYMAP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the ProbeMaestro API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
//

#include "MemoryMap.h"

#include <unordered_map>
#include <unordered_set>

class MemoryMapPrivate
{
public:
    struct MemoryMapEntry
    {
        void *address;
        std::string name;
        // NOTE: Although moduleName is stored internally as an ASCII string,
        //       the public interface supports both ASCII and Uni-Code.
        //       Internally a lossless conversion method is used to ensure that both
        //       ASCII and Uni-Code values can be stored and retrived safely in the same variable
        std::string moduleName;
        MemoryMap::tags_t tags;
    };

public:
    static void removeEntry(const MemoryMapEntry &entry);
    static bool addAddress(const std::string &name, size_t address, const std::string &moduleName, const MemoryMap::tags_t &tags);

    // search functionality
    static bool addressExists(const std::string &name);
    static std::unordered_set<MemoryMapEntry *> allAddresses();
    static MemoryMapEntry *getAddressByName(const std::string &name);
    static bool satisfiesFilter(const std::string &name, const MemoryMap::FilterOptions &filterOptions);
    static bool satisfiesFilter(const MemoryMapEntry &entry, const MemoryMap::FilterOptions &filterOptions);
    static std::unordered_set<MemoryMapEntry *> getAddressesByModuleName(const std::wstring &moduleName);

    static std::unordered_set<MemoryMapEntry *> filter(const MemoryMap::FilterOptions &filterOptions);
    static std::unordered_set<MemoryMapEntry *> filterByTags(const std::unordered_set<MemoryMapEntry *> &input, const MemoryMap::tags_t &tags);

    // indexing functionality
    static void optimizePropertiesIndex();
    static void removeFromPropertyIndex(const std::string &propertyName, const std::string &value, MemoryMapEntry *entry);
    static void addToPropertyIndex(const std::string &propertyName, const std::string &propertyValue, MemoryMapEntry *entry);
    static std::unordered_set<MemoryMapEntry *> getEntriesWithPropertyValue(const std::string &propertyName, const std::string &propertyValue);

public: // TODO: make these variables thread-safe
    static std::list<MemoryMapEntry> addressEntries;
    //
    // NOTE: This structure is designed to allow fast access to all entries that have a specific value for any given property
    //
    // USAGE: To get all entries with a given property value: propertiesIndex[property name][property value]
    //
    static std::unordered_map<std::string, std::unordered_multimap<std::string, MemoryMapEntry *>> propertiesIndex;
};

inline bool MemoryMapPrivate::addressExists(const std::string &name)
{
    return getAddressByName(name) != nullptr;
}

inline std::unordered_set<MemoryMapPrivate::MemoryMapEntry *> MemoryMapPrivate::allAddresses()
{
    std::unordered_set<MemoryMapPrivate::MemoryMapEntry *> result;

    for (auto it = addressEntries.begin(); it != addressEntries.end(); ++it)
        result.insert(&(*it));

    return result;
}

inline void MemoryMapPrivate::removeFromPropertyIndex(const std::string &propertyName, const std::string &value, MemoryMapEntry *entry)
{
    auto range = propertiesIndex[propertyName].equal_range(value);

    for (auto it = range.first; it != range.second;)
    {
        if (it->second == entry)
            it = propertiesIndex[propertyName].erase(it);
        else
            ++it;
    }
}

inline void MemoryMapPrivate::addToPropertyIndex(const std::string &propertyName, const std::string &propertyValue, MemoryMapEntry *entry)
{
    propertiesIndex[propertyName].emplace(propertyValue, entry);
}

inline std::unordered_set<MemoryMapPrivate::MemoryMapEntry *> MemoryMapPrivate::getEntriesWithPropertyValue(const std::string &propertyName,
                                                                                                            const std::string &propertyValue)
{
    std::unordered_set<MemoryMapEntry *> result;

    auto range = propertiesIndex[propertyName].equal_range(propertyValue);
    for (auto it = range.first; it != range.second; ++it)
        result.insert(it->second);

    return result;
}

// TODO: move to a more appropriate location
namespace internal
{
template <typename T>
bool containsAllElements(const std::unordered_set<T> &superset, const std::unordered_set<T> &subset)
{
    for (const T &element : subset)
    {
        if (superset.find(element) == superset.end())
            return false;
    }

    return true;
}
} // namespace internal

#endif // MEMORYMAP_P_H
