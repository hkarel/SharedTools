#pragma once

#if !defined(_MSC_VER)
#include <cxxabi.h>
#endif

template<typename T> inline char* abi_type_name()
{
#if defined(_MSC_VER)
    return typeid(T).name();
#else
    return abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
}
