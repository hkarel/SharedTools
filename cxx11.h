/**
  Модуль определяет поддержку стандарта с++11
*/

#pragma once

// #if !defined(__SUPPORT_CXX_11__)
// #  if defined(_MSC_VER)
// #    if _MSC_VER > 1500
// #      define __SUPPORT_CXX_11__
// #    endif
// #  endif
// #  if defined(__GXX_EXPERIMENTAL_CXX0X__)
// #    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 404
//        /* C++0x features supported in GCC 4.4: */
// #      define __SUPPORT_CXX_11__
// #    endif
// #  endif
// #endif

// #if !defined(__SUPPORT_CXX_11__)
// #  if defined(_MSC_VER)
// #    if _MSC_VER > 1500
// #      define __SUPPORT_CXX_11__
// #    endif
// #  else
// #    if __cplusplus >= 201103L
// #      define __SUPPORT_CXX_11__
// #    endif
// #  endif
// #endif

#ifndef __SUPPORT_CXX_11__
#  ifdef _MSC_VER
#    if _MSC_VER > 1500
#      define __SUPPORT_CXX_11__
#    endif
#  else
#    ifdef __GXX_EXPERIMENTAL_CXX0X__
#      define __SUPPORT_CXX_11__
#    endif
#  endif
#endif

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif

