# Architecture

## Basic functionality

* Address addition
```cpp
    // basic form
    bool result = MemoryMap::addAddress("module1:object1", 0x77b6529ef, moduleHandle, moduleImageBase, {"tag1", "tag2"});

    // compact form 1 (maybe?)
    result = MemoryMap::addAddress("module1:object1", 0x77b6529ef, loader, {"tag1", "tag2"});

    // compact form 2 (maybe?)
    result = MemoryMap::addAddress("module1:object1", "module1.dll", 0x77b6529ef, {"tag1", "tag2"}); // automatically retrives the moduleHandle and moduleImageBase when the module is loaded

    // compact form 3 (maybe?)
    result = MemoryMap::addAddress("module1:object1", 0x77b6529ef, {"tag1", "tag2"}); // automatically calculates the `moduleHandle` and `moduleImageBase` from the given address 

    // NOTE: 
    //       1. Address name is a unique identifier, cannot register two different addresses with the same name.
    //       2. In case multiple modules have objects with the same name, they have to be registered with two different names.
    //          it's good practice to add the module name as a prefix to the address name, e.g. "{module name}:{address name}"
    //       3. An address can be for anything, e.g. function, object, image header field, etc..
    //       4. An address can point to anything in the memory space of the current process,
    //          doesn't matter if this address is inside a statically or dynamically loaded module.
    //       5. The Memory map can be modified easily through both code and {JSON} files.
    //       6. `tags` are meant to allow the users to categorize different addresses based on any criteria the user wants,
    //          This gives users the ability to implement their own MemoryMap-like interfaces on top of the existing MemoryMap Interface
    //          to support complex data types such as QMetaObjects, RTTI, QML singletons, etc.. 
```
* Address usage
```cpp
    // basic form
    auto address1 = MemoryMap::getAddress("address1");

    // compact form
    const char *string1 = MemoryMap::getAddress<const char *>("module1:string1");
```
* Listing available addresses
```cpp
    // basic form
    auto addresses = MemoryMap::getAddresses();

    // advanced form (maybe?)
    addresses = MemoryMap::getAddresses({moduleName, {"tag1", "tag2"}}); // filtering options
```
* Removing addresses
```cpp
    MemoryMap::clear();

    // basic form
    MemoryMap::removeAddress("object1");

    // advanced form (maybe?)
    MemoryMap::removeAddresses({moduleName, {"tag1", "tag2"}}); // filtering options
```
* Address struct
```cpp
struct AddressEntry
{
    void *address;
    std::unordered_set<std::string> tags;
    std::string name;       // maybe?
    std::string moduleName; // maybe?
};
```

## Implementation details

The best implementation would be something that provides O(1) or ammortized O(1) complexity for all basic operations, while having low complexity and capability for scaling effectively.</br>
The following implementation method satisfies this criteria. Basically we maintain an index for all values that are assigned to each property in the `AddressEntry` structure. each unique value has a list that points to all entries that have this value.

Here's a simplified implementation for this method:
```cpp
struct AddressEntry 
{
    void *address;
    std::string name;
    std::string moduleName;
    std::unordered_set<std::string> tags;
};

static std::list<AddressEntry> addresses;
// 
// NOTE: This structure is designed to allow fast access to all entries that have a specific value for any given property
//
// USAGE: To get all entries with a given property value: propertiesIndex[property name][property value]
//
static std::unordered_map<std::string, std::unordered_multimap<std::string, AddressEntry *>> propertiesIndex;

void addAddress(const std::string &moduleName, const std::string &name, size_t address, const std::unordered_set<std::string> &tags) 
{
    addresses.emplace_back(AddressEntry{reinterpret_cast<void *>(address), name, moduleName, tags});
    AddressEntry *entryPtr = &addresses.back();
    
    propertiesIndex["name"].emplace(name, entryPtr);
    propertiesIndex["moduleName"].emplace(moduleName, entryPtr);

    for (const std::string &tag : tags)
        propertiesIndex["tags"].emplace(tag, entryPtr);
}

std::unordered_set<AddressEntry *> getAddressesByModuleName(const std::string &moduleName) 
{
    std::unordered_set<AddressEntry *> result;

    auto range = propertiesIndex["moduleName"].equal_range(moduleName);
    for (auto it = range.first; it != range.second; ++it)
        result.insert(it->second);

    return result;
}

AddressEntry *getAddressByName(const std::string &name)
{
	auto range = propertiesIndex["name"].equal_range(name);
	if (range.first != range.second)
		return range.first->second;
	
	return nullptr;
}

template <typename T>
std::unordered_set<T> intersect(const std::unordered_set<T> &set1, const std::unordered_set<T> &set2) 
{
    std::unordered_set<T> result;

    for (const T &element : set1)
    {
        if (set2.find(element) == set2.end())
            continue;

        result.insert(element);
    }

    return result;
}
```

## Nice-to-haves

| Feature | Description | Priority |
| --- | --- | --- |
| Preserve `const` correctness | It would be good if it's somehow possible to preserve const correctness, so that read only addresses are returned as const pointers. | very low |
| Support `FileOffsets` | For now we only support addresses as memory offsets (`MemoryOffset`). In the future we should also support addresses as file offsets (`FileOffset`). | medium |
| Auto-remove addresses when their modules are unloaded | It maybe a good idea to have the ability to allow the `MemoryMap` to automatically manage the life-time of addresses based on the life time of the module that they are pointing to. so when a module is unloaded, all addresses that are associated with this module gets removed automatically | very low |
