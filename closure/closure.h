/* clang-format off */
/**
  Closure library 1.1
  file "closure.h"

  Written by Ivan Yankov aka _Winnie (woowoowoow@bk.ru)
  Many thanks to Wolfhound

  Modyfied:  Pavel Karelin (hkarel), hkarel@yandex.ru
*/



#pragma once
#ifndef CLOSURE_HEADER_
#define CLOSURE_HEADER_


#define CLOSURE_TYPENAMES
#define CLOSURE_TYPES
#define CLOSURE_FUNC_PARAMS
#define CLOSURE_FUNC_ARGS
#define CLOSURE_COMMA
#define CLOSURE_NAMESPACE Closure0
#include "closure_impl.h"

#define CLOSURE_TYPENAMES  typename P0
#define CLOSURE_TYPES P0
#define CLOSURE_FUNC_PARAMS P0 p0
#define CLOSURE_FUNC_ARGS p0
#define CLOSURE_NAMESPACE Closure1
#include "closure_impl.h"

#define CLOSURE_TYPENAMES typename P0, typename P1
#define CLOSURE_TYPES P0, P1
#define CLOSURE_FUNC_PARAMS  P0 p0, P1 p1
#define CLOSURE_FUNC_ARGS p0, p1
#define CLOSURE_NAMESPACE Closure2
#include "closure_impl.h"

#define CLOSURE_TYPENAMES typename P0, typename P1, typename P2
#define CLOSURE_TYPES P0, P1, P2
#define CLOSURE_FUNC_PARAMS P0 p0, P1 p1, P2 p2
#define CLOSURE_FUNC_ARGS p0, p1, p2
#define CLOSURE_NAMESPACE Closure3
#include "closure_impl.h"

#define CLOSURE_TYPENAMES  typename P0, typename P1, typename P2, typename P3
#define CLOSURE_TYPES P0, P1, P2, P3
#define CLOSURE_FUNC_PARAMS P0 p0, P1 p1, P2 p2, P3 p3
#define CLOSURE_FUNC_ARGS p0, p1, p2, p3
#define CLOSURE_NAMESPACE Closure4
#include "closure_impl.h"

#define CLOSURE(PTR, MEM_PTR) (closure::CreateClosure(MEM_PTR).Init<MEM_PTR>(PTR))
//#define CLOSURE(PTR, MEM_PTR) detail::CreateClosure(PTR, MEM_PTR)

#endif

