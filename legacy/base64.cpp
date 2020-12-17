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

#include "base64.h"
#include <assert.h>

namespace base64 {

int encodeRequiredLength(int srcLen, unsigned int flags) noexcept
{
    int ret = srcLen*4/3;

    if ((flags & ATL_BASE64_FLAG_NOPAD) == 0)
        ret += srcLen % 3;

    int nCRLFs = ret / 76 + 1;
    int nOnLastLine = ret % 76;

    if (nOnLastLine)
    {
        if (nOnLastLine % 4)
            ret += 4-(nOnLastLine % 4);
    }

    nCRLFs *= 2;

    if ((flags & ATL_BASE64_FLAG_NOCRLF) == 0)
        ret += nCRLFs;

    return ret;
}

int secodeRequiredLength(int srcLen) noexcept
{
    return srcLen;
}

bool encode(const uint8_t* src,
            int srcLen,
            char* dest,
            int* destLen,
            uint32_t flags) noexcept
{
    static const char encoding_table[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
        'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',    'h',
        'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
        'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };

    if (!src || !dest || !destLen)
    {
        return false;
    }

    if(*destLen < encodeRequiredLength(srcLen, flags))
    {
        assert(false);
        return false;
    }

    int written = 0;
    int len1 = (srcLen / 3) * 4;
    int len2 = len1 / 76;
    int len3 = 19;

    for (int i = 0; i <= len2; i++)
    {
        if (i == len2)
            len3 = (len1 % 76) / 4;

        for (int j = 0; j < len3; j++)
        {
            uint32_t curr = 0;
            for (int n = 0; n < 3; n++)
            {
                curr |= *src++;
                curr <<= 8;
            }
            for (int k = 0; k < 4; k++)
            {
                uint8_t b = (uint8_t)(curr >> 26);
                *dest++ = encoding_table[b];
                curr <<= 6;
            }
        }
        written += len3 * 4;

        if ((flags & ATL_BASE64_FLAG_NOCRLF) == 0)
        {
            *dest++ = '\r';
            *dest++ = '\n';
            written += 2;
        }
    }

    if (written && (flags & ATL_BASE64_FLAG_NOCRLF) == 0)
    {
        dest -= 2;
        written -= 2;
    }

    len2 = (srcLen % 3) ? (srcLen % 3 + 1) : 0;
    if (len2)
    {
        uint32_t curr = 0;
        for (int n = 0; n < 3; n++)
        {
            if (n < (srcLen % 3))
                curr |= *src++;
            curr <<= 8;
        }
        for (int k = 0; k < len2; k++)
        {
            uint8_t b = (uint8_t)(curr >> 26);
            *dest++ = encoding_table[b];
            curr <<= 6;
        }
        written += len2;
        if ((flags & ATL_BASE64_FLAG_NOPAD) == 0)
        {
            len3 = len2 ? (4 - len2) : 0;
            for (int j = 0; j < len3; j++)
            {
                *dest++ = '=';
            }
            written += len3;
        }
    }

    *destLen = written;
    return false;
}

int decodeBase64Char(unsigned int ch) noexcept
{
    // returns -1 if the character is invalid
    // or should be skipped
    // otherwise, returns the 6-bit code for the character
    // from the encoding table
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A' + 0;     // 0 range starts at 'A'
    if (ch >= 'a' && ch <= 'z')
        return ch - 'a' + 26;    // 26 range starts at 'a'
    if (ch >= '0' && ch <= '9')
        return ch - '0' + 52;    // 52 range starts at '0'
    if (ch == '+')
        return 62;
    if (ch == '/')
        return 63;
    return -1;
}

bool decode(const char* src, int srcLen, uint8_t* dest, int* destLen) noexcept
{
    // walk the source buffer
    // each four character sequence is converted to 3 bytes
    // CRLFs and =, and any characters not in the encoding table
    // are skiped

    if (src == 0 || destLen == 0)
    {
        assert(false);
        return false;
    }

    const char* srcEnd = src + srcLen;
    int written = 0;

    bool overflow = (dest == 0) ? true : false;

    while (src < srcEnd)
    {
        uint32_t curr = 0;
        int i;
        int bits = 0;
        for (i = 0; i < 4; i++)
        {
            if (src >= srcEnd)
                break;
            int nCh = decodeBase64Char(*src);
            src++;
            if (nCh == -1)
            {
                // skip this char
                i--;
                continue;
            }
            curr <<= 6;
            curr |= nCh;
            bits += 6;
        }

        if (!overflow && written + (bits / 8) > (*destLen))
            overflow = true;

        // dwCurr has the 3 bytes to write to the output buffer
        // left to right
        curr <<= (24 - bits);
        for (i = 0; i < (bits / 8); i++)
        {
            if (!overflow)
            {
                *dest = (uint8_t) ((curr & 0x00ff0000) >> 16);
                dest++;
            }
            curr <<= 8;
            written++;
        }
    }

    *destLen = written;

    if (overflow)
    {
        if (dest != 0)
            assert(false);
        return false;
    }
    return true;
}

} // namespace base64
