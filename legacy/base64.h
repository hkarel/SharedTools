/* clang-format off */
/*****************************************************************************
  This is a part of the Active Template Library.
  Copyright (C) Microsoft Corporation
  All rights reserved.

  This source code is only intended as a supplement to the
  Active Template Library Reference and related
  electronic documentation provided with the library.
  See these sources for detailed information regarding the
  Active Template Library product.
*****************************************************************************/

#pragma  once
#include <stdint.h>

namespace base64 {

//=======================================================================
// Base64Encode/Base64Decode
// compliant with RFC 2045
//=======================================================================
//
#define ATL_BASE64_FLAG_NONE	0
#define ATL_BASE64_FLAG_NOPAD	1
#define ATL_BASE64_FLAG_NOCRLF  2

// Возвращает размер буффера необходимый для размещения результата кодирования
// данных длиной srcLen
int encodeRequiredLength(int srcLen, uint32_t flags = ATL_BASE64_FLAG_NONE) noexcept;

// Выполняет действие обратное функции encodeRequiredLength()
int decodeRequiredLength(int srcLen) noexcept;

// Выполняет перобразование бинарного массива src длиной srcLen в представление
// BASE64.
bool encode(const uint8_t* src,
            int srcLen,
            char* dest,
            int *destLen,
            uint32_t flags = ATL_BASE64_FLAG_NONE) noexcept;

//int decodeBase64Char(unsigned int ch) noexcept;

bool decode(const char* src,
            int srcLen,
            uint8_t* dest,
            int* destLen) noexcept;

} // namespace base64
