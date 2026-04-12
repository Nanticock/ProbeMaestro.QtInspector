//
// NOTE: This code is based on this article
//       https://www.lukaszlipski.dev/post/rtti-msvc/
//

//
// NOTE: The complete implementation can be found in clang here
//       https://github.com/llvm/llvm-project/blob/b68340c83529bd8d13d1c4441777baa31863d1cf/clang/include/clang/AST/Mangle.h
//       https://github.com/llvm/llvm-project/blob/7ed36b9ec6147fbada27592292bca28f9f76c983/clang/lib/AST/MicrosoftMangle.cpp
//

#include <windows.h>

#include <rttidata.h>

#include <iostream>

inline HMODULE getModuleFromAddress(const void *address)
{
    HMODULE hModule = nullptr;
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, LPCTSTR(address), &hModule))
        return hModule;

    return nullptr;
}

inline void *getCurrentModuleBase()
{
    HMODULE hModule = GetModuleHandle(nullptr);

    return static_cast<void *>(hModule);
}

template <typename T>
inline T *addressFromRelativeOffset(size_t offset, size_t base = size_t(getCurrentModuleBase()))
{
    size_t ptr = base + offset;

    return reinterpret_cast<T *>(ptr);
}

//
// FIXME: add safety checks to make sure provided addresses has the required permessions
//
inline void printRTTIInfo(void *obj, size_t imageBase)
{
    if (!obj)
    {
        std::cerr << "Null object pointer!" << std::endl;

        return;
    }

    // Assuming obj is a pointer to a polymorphic type
    void **vfptr = *reinterpret_cast<void ***>(obj);

    // std::cout << "vf table:" << std::endl;
    // int i = 0;
    // do
    // {
    //     std::cout << "\t[" << i << "] " << vfptr[i] << std::endl;
    //     ++i;
    // }
    // while (vfptr[i] != nullptr);
    // std::cout << std::endl;

    //
    // NOTE: the RTTI complete object locator is the element before the first vfunction pointer in the vtable
    //
    _RTTICompleteObjectLocator *rttiCompleteObjectLocator = reinterpret_cast<_RTTICompleteObjectLocator *>(vfptr[-1]);
    if (!rttiCompleteObjectLocator || rttiCompleteObjectLocator->signature != COL_SIG_REV1)
    {
        std::cerr << "No RTTI information available!" << std::endl;

        return;
    }

    // for x64 is set to COL_SIG_REV1 which means pTypeDescriptor, pClassDescriptor and pSelf are going to be image base relative offsets.
    std::cout << "\tsignature: " << (rttiCompleteObjectLocator->signature == COL_SIG_REV0 ? "COL_SIG_REV0" : "COL_SIG_REV1") << " = "
              << rttiCompleteObjectLocator->signature << std::endl;
    // the offset from the complete object to the current sub-object from which we’ve taken RTTICompleteObjectLocator.
    std::cout << "\toffset: " << rttiCompleteObjectLocator->offset << std::endl;
    // the constructor displacement’s offset. It’s relevant only in particular situations when using virtual inheritance.
    // This is Microsoft’s specific way to optimize data generation needed to handle some cases when virtual inheritance is used.
    std::cout << "\tcdOffset: " << rttiCompleteObjectLocator->cdOffset << std::endl;
    // contains the offset from the image base to complete the object’s TypeDescriptor.
    std::cout << "\tpTypeDescriptor: " << rttiCompleteObjectLocator->pTypeDescriptor << std::endl;
    // contains the offset from the image base to RTTIClassHierarchyDescriptor.
    std::cout << "\tpClassDescriptor: " << rttiCompleteObjectLocator->pClassDescriptor << std::endl;
    // contains the offset from image base to the current RTTICompleteObjectLocator.
    // This gives us a simple way to get the image base which we can use to get pTypeDescriptor and pClassDescriptor.
    std::cout << "\tpSelf: " << rttiCompleteObjectLocator->pSelf << std::endl;

    //
    // NOTE: it is possible to do this in the following way
    //       TypeDescriptor *typeDescriptor = COL_PTD_IB(*rttiCompleteObjectLocator, size_t(getCurrentModuleBase()));
    //
    TypeDescriptor *typeDescriptor = addressFromRelativeOffset<TypeDescriptor>(rttiCompleteObjectLocator->pTypeDescriptor, imageBase);
    _RTTIClassHierarchyDescriptor *classHierarchyDescriptor =
        addressFromRelativeOffset<_RTTIClassHierarchyDescriptor>(rttiCompleteObjectLocator->pClassDescriptor, imageBase);
    const _s_RTTICompleteObjectLocator *pSelf =
        addressFromRelativeOffset<const _s_RTTICompleteObjectLocator>(rttiCompleteObjectLocator->pSelf, imageBase);

    std::cout << std::endl;
    std::cout << "Class name: " << typeDescriptor->name << std::endl;
    std::cout << "Number of bases: " << classHierarchyDescriptor->numBaseClasses << std::endl;

    for (unsigned long i = 0; i < classHierarchyDescriptor->numBaseClasses; ++i)
    {
        _RTTIBaseClassArray *baseClassArray = addressFromRelativeOffset<_RTTIBaseClassArray>(classHierarchyDescriptor->pBaseClassArray, imageBase);
        _RTTIBaseClassDescriptor *baseClassDescriptor =
            addressFromRelativeOffset<_RTTIBaseClassDescriptor>(baseClassArray->arrayOfBaseClassDescriptors[i], imageBase);
        TypeDescriptor *baseTypeDescriptor = addressFromRelativeOffset<TypeDescriptor>(baseClassDescriptor->pTypeDescriptor, imageBase);

        std::cout << "\tBase class " << i << ": " << baseTypeDescriptor->name << std::endl;
    }
}

inline void printRTTIInfo(void *obj)
{
    size_t imageBase = size_t(getModuleFromAddress(obj));

    printRTTIInfo(obj, imageBase);
}

struct ParentA
{
    virtual ~ParentA() = default;
};

struct ParentB
{
    virtual ~ParentB() = default;
};

class SomeClass : public ParentA, public ParentB
{
public:
    virtual ~SomeClass() = default;

    virtual int getNum()
    {
        return 2;
    }
};

inline void testRttiTraversal_msvc()
{
    SomeClass obj;
    printRTTIInfo(&obj);
}
