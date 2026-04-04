#ifndef RTTIHELPER_H
#define RTTIHELPER_H

//
// R E F E R E N C E S:
//
// ---- MSVC ----
//
// Official:
//      [01] https://learn.microsoft.com/en-us/cpp/cpp/run-time-type-information?view=msvc-170
//
// Non-official:
//      [01] https://blog.rop.la/en/reversing/2022/12/13/identifying-vftables-through-ms-cpp-rtti.html
//      [02] https://blog.quarkslab.com/visual-c-rtti-inspection.html
//      [03] https://martinkysel.com/demystifying-virtual-tables-in-c-part-3-virtual-tables
//      [04] https://www.lukaszlipski.dev/post/rtti-msvc
//      [05] https://wyfeizj.wordpress.com/2011/03/25/the-rtti-implementation-of-vs2010
//      [06] http://www.hexblog.com/wp-content/uploads/2012/06/Recon-2012-Skochinsky-Compiler-Internals.pdf
//      [07] https://www.openrce.org/articles/full_view/23
//      [08] https://github.com/nihilus/RECPP/blob/master/RECPP
//      [09] https://github.com/nihilus/RECPP/blob/master/RECPP/RTTIBaseClassDescriptor.h
//      [10] https://clang.llvm.org/docs/MSVCCompatibility.html
//      [11] https://github.com/llvm/llvm-project/blob/main/libcxxabi/include/__cxxabi_config.h
//      [12] https://github.com/llvm/llvm-project/blob/902fb1b4653d5a23613492406cd5693446f06ab6/libcxxabi/src/private_typeinfo.cpp#L121
//      [13] https://libcxx.llvm.org/BuildingLibcxx.html
//      [14] https://github.com/theluc4s/RTTI-Finder-Dumper/blob/master/RTTI/rtti.cpp
//
// ---- GCC ----
//
// Official:
//      [01] https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/tinfo.h
//      [02] https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/typeinfo
//      [03] https://itanium-cxx-abi.github.io/cxx-abi/abi.html
//      [04] https://github.com/libcxxrt/libcxxrt/blob/master/src/typeinfo.h
//

//
// NOTE: a list of available RTTI in a binary can be obtained from exception tables
//       Refer to Khalil estell talk about C++ exceptions at cppcon 2024:
//          https://www.youtube.com/watch?v=bY2FlayomlE

#include <cstddef>

//
// Code Source:
// https://github.com/nihilus/RECPP/blob/master/RECPP/RTTIBaseClassDescriptor.h
//

typedef const struct TypeDescriptor
{
    // vtable of type_info class
    const void *pVFTable;
    // used to keep the demangled name returned by type_info::name()
    void *spare;
    // mangled type name, e.g. ".H" = "int", ".?AUA@@" = "struct A", ".?AVA@@" = "class A"
    char name[0];
} _TypeDescriptor;

// member displacement info
typedef struct PMD2
{
    ptrdiff_t mdisp; // member displacement
    ptrdiff_t pdisp; // vbtable offset
    ptrdiff_t vdisp; // displacement inside vbtable
} _PMD;

// describes all base classes together with information
// which allows compiler to cast the derived class to any of them
// during execution of the _dynamic_cast_ operator.
typedef const struct _s__RTTIBaseClassDescriptor
{
    _TypeDescriptor *pTypeDescriptor; // type descriptor of the class
    unsigned long numContainedBases;  // number of nested classes following in the Base Class Array
    _PMD where;                       // pointer-to-member displacement info
    unsigned long attributes;         // flags, usually 0
} __RTTIBaseClassDescriptor;

#pragma warning(disable : 4200)
typedef const struct _s__RTTIBaseClassArray
{
    __RTTIBaseClassDescriptor *arrayOfBaseClassDescriptors[];
} __RTTIBaseClassArray;
#pragma warning(default : 4200)

// describes the inheritance hierarchy of the class. It is shared by all COLs for a class.
typedef const struct _s__RTTIClassHierarchyDescriptor
{
    unsigned long signature;      // always zero?
    unsigned long attributes;     // bit 0 set = multiple inheritance, bit 1 set = virtual inheritance
    unsigned long numBaseClasses; // number of classes in pBaseClassArray
    __RTTIBaseClassArray *pBaseClassArray;
} __RTTIClassHierarchyDescriptor;

// allows the compiler to find the location of the complete object
// from a specific vftable pointer (since a class can have several of them)
typedef const struct _s__RTTICompleteObjectLocator
{
    unsigned long signature;                          // always zero?
    unsigned long offset;                             // offset of this vtable in the complete class
    unsigned long cdOffset;                           // constructor displacement offset
    _TypeDescriptor *pTypeDescriptor;                 // TypeDescriptor of the complete class
    __RTTIClassHierarchyDescriptor *pClassDescriptor; // describes inheritance hierarchy
} __RTTICompleteObjectLocator;

#endif // RTTIHELPER_H
