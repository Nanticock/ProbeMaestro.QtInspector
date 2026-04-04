#include "MemoryMap.h"
#include "MemoryMap_p.h"

// FIXME: get rid of this include file
#include <ExecutableLoader/ExecutableLoader_p.h>

namespace
{
const char MEMORY_MAP_ENTRY_NAME_KEY[] = "name";
const char MEMORY_MAP_ENTRY_MODULE_NAME_KEY[] = "moduleName";
const char MEMORY_MAP_ENTRY_TAGS_KEY[] = "tags";
} // namespace

std::list<MemoryMapPrivate::MemoryMapEntry> MemoryMapPrivate::addressEntries;
std::unordered_map<std::string, std::unordered_multimap<std::string, MemoryMapPrivate::MemoryMapEntry *>> MemoryMapPrivate::propertiesIndex;

bool MemoryMapPrivate::addAddress(const std::string &name, size_t address, const std::string &moduleName, const MemoryMap::tags_t &tags)
{
    // since the address name is a unique identifier, we cannot have an empty name
    if (name.empty()) // ## maybe display some warning
        return false;

    // since the address name is a unique identifier, we cannot add two addresses with the same name
    if (addressExists(name))
        return false;

    addressEntries.emplace_back(MemoryMapEntry{reinterpret_cast<void *>(address), name, moduleName, tags});
    MemoryMapEntry *entry = &addressEntries.back();

    addToPropertyIndex(MEMORY_MAP_ENTRY_NAME_KEY, name, entry);
    addToPropertyIndex(MEMORY_MAP_ENTRY_MODULE_NAME_KEY, moduleName, entry);

    for (const std::string &tag : tags)
        addToPropertyIndex(MEMORY_MAP_ENTRY_TAGS_KEY, tag, entry);

    return true;
}

void MemoryMapPrivate::removeEntry(const MemoryMapPrivate::MemoryMapEntry &entry)
{
    MemoryMapEntry *entryPtr = const_cast<MemoryMapEntry *>(&entry);

    // remove from propertiesIndex
    removeFromPropertyIndex(MEMORY_MAP_ENTRY_NAME_KEY, entry.name, entryPtr);
    removeFromPropertyIndex(MEMORY_MAP_ENTRY_MODULE_NAME_KEY, entry.moduleName, entryPtr);
    for (const std::string &tag : entry.tags)
        removeFromPropertyIndex(MEMORY_MAP_ENTRY_TAGS_KEY, tag, entryPtr);

    // Remove from addressEntries
    addressEntries.remove_if([&entry](const MemoryMapEntry &e) { return &e == &entry; });
}

std::unordered_set<MemoryMapPrivate::MemoryMapEntry *> MemoryMapPrivate::getAddressesByModuleName(const std::wstring &moduleName)
{
    return getEntriesWithPropertyValue(MEMORY_MAP_ENTRY_MODULE_NAME_KEY, internal::toString(moduleName));
}

std::unordered_set<MemoryMapPrivate::MemoryMapEntry *> MemoryMapPrivate::filter(const MemoryMap::FilterOptions &filterOptions)
{
    if (filterOptions.tags.empty() && filterOptions.moduleName.empty())
        return allAddresses();

    std::unordered_set<MemoryMapPrivate::MemoryMapEntry *> result;

    if (!filterOptions.moduleName.empty())
    {
        result = getAddressesByModuleName(filterOptions.moduleName);
    }
    else // no module name, then all addresses are included
    {
        for (auto it = MemoryMapPrivate::addressEntries.begin(); it != MemoryMapPrivate::addressEntries.end(); ++it)
            result.insert(&(*it));
    }

    return filterByTags(result, filterOptions.tags);
}

std::unordered_set<MemoryMapPrivate::MemoryMapEntry *> MemoryMapPrivate::filterByTags(const std::unordered_set<MemoryMapEntry *> &input,
                                                                                      const MemoryMap::tags_t &tags)
{
    if (input.empty() || tags.empty())
        return input;

    std::unordered_set<MemoryMapPrivate::MemoryMapEntry *> result;

    // only keep entries that have all the provided tags
    for (auto &entry : input)
    {
        bool allTagsMatch = true;

        for (const auto &tag : tags)
        {
            if (entry->tags.find(tag) == entry->tags.end())
            {
                allTagsMatch = false;

                break;
            }
        }

        if (allTagsMatch)
            result.insert(entry);
    }

    return result;
}

/**
 * @brief Optimize the properties index by removing empty properties or property values.
 *
 * This function iterates through the propertiesIndex map, which maps property names
 * (like "name", "moduleName", and "tags") to multimaps of property values and MemoryMapEntry pointers.
 * It removes any entries from these multimaps where the pointer is null, effectively cleaning up
 * stale or invalid references. After cleaning each multimap, it checks if the multimap itself is
 * empty and, if so, removes the property from the propertiesIndex.
 *
 * This helps keep the propertiesIndex map lean and efficient by ensuring that only valid and
 * necessary entries are stored, improving the overall performance and memory usage.
 */
void MemoryMapPrivate::optimizePropertiesIndex()
{
    for (auto it = propertiesIndex.begin(); it != propertiesIndex.end();)
    {
        std::unordered_multimap<std::string, MemoryMapEntry *> &propertyValueToEntriesMultimap = it->second;

        for (auto mit = propertyValueToEntriesMultimap.begin(); mit != propertyValueToEntriesMultimap.end();)
        {
            // remove values with null pointers
            if (mit->second == nullptr)
                mit = propertyValueToEntriesMultimap.erase(mit);
            else
                ++mit;
        }

        if (propertyValueToEntriesMultimap.empty())
            it = propertiesIndex.erase(it);
        else
            ++it;
    }
}

MemoryMapPrivate::MemoryMapEntry *MemoryMapPrivate::getAddressByName(const std::string &name)
{
    auto range = propertiesIndex[MEMORY_MAP_ENTRY_NAME_KEY].equal_range(name);
    if (range.first != range.second)
        return range.first->second;

    return nullptr;
}

bool MemoryMapPrivate::satisfiesFilter(const std::string &name, const MemoryMap::FilterOptions &filterOptions)
{
    MemoryMapEntry *entry = getAddressByName(name);

    if (entry == nullptr)
        return false;

    return satisfiesFilter(*entry, filterOptions);
}

bool MemoryMapPrivate::satisfiesFilter(const MemoryMapEntry &entry, const MemoryMap::FilterOptions &filterOptions)
{
    if (!filterOptions.moduleName.empty() && entry.moduleName != internal::toString(filterOptions.moduleName))
        return false;

    if (!filterOptions.tags.empty() && !internal::containsAllElements(entry.tags, filterOptions.tags))
        return false;

    return true;
}

MemoryMap::FilterOptions::FilterOptions() : FilterOptions("", {})
{
}

MemoryMap::FilterOptions::FilterOptions(const std::string &moduleName, const MemoryMap::tags_t &tags) :
    FilterOptions(internal::toWString(moduleName), tags)
{
}

MemoryMap::FilterOptions::FilterOptions(const std::wstring &moduleName, const MemoryMap::tags_t &tags) : moduleName(moduleName), tags(tags)
{
}

bool MemoryMap::addAddress(const std::string &name, size_t address, const std::wstring &moduleName, const tags_t &tags)
{
    return MemoryMapPrivate::addAddress(name, address, internal::toString(moduleName), tags);
}

std::unordered_set<std::string> MemoryMap::getAllAddresses()
{
    std::unordered_set<std::string> result;

    for (auto it = MemoryMapPrivate::addressEntries.begin(); it != MemoryMapPrivate::addressEntries.end(); ++it)
        result.insert(it->name);

    return result;
}

std::unordered_set<std::string> MemoryMap::getAllAddresses(const FilterOptions &filterOptions)
{
    std::unordered_set<std::string> result;

    auto add = MemoryMapPrivate::addressEntries;
    auto pi = MemoryMapPrivate::propertiesIndex;

    for (MemoryMapPrivate::MemoryMapEntry *entry : MemoryMapPrivate::filter(filterOptions))
        result.insert(entry->name);

    return result;
}

void MemoryMap::clear()
{
    MemoryMapPrivate::addressEntries.clear();
    MemoryMapPrivate::propertiesIndex.clear();
}

bool MemoryMap::remove(const std::string &name)
{
    MemoryMapPrivate::MemoryMapEntry *entry = MemoryMapPrivate::getAddressByName(name);

    if (entry == nullptr)
        return false;

    MemoryMapPrivate::removeEntry(*entry);
    MemoryMapPrivate::optimizePropertiesIndex();

    return true;
}

size_t MemoryMap::removeAll(const FilterOptions &filterOptions)
{
    auto entriesToRemove = MemoryMapPrivate::filter(filterOptions);

    // Remove the entries
    for (MemoryMapPrivate::MemoryMapEntry *entry : entriesToRemove)
        MemoryMapPrivate::removeEntry(*entry);

    MemoryMapPrivate::optimizePropertiesIndex();

    return entriesToRemove.size();
}

void *MemoryMap::getAddressImpl(const std::string &name)
{
    MemoryMapPrivate::MemoryMapEntry *result = MemoryMapPrivate::getAddressByName(name);

    if (result == nullptr)
        return nullptr;

    return result->address;
}

std::wstring placeholder::currentModuleName()
{
    return internal::getModuleFileName(internal::getCurrentModuleHandle());
}
