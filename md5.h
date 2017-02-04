/*****************************************************************************
  Расчет хеша по массиву данных в памяти.
*****************************************************************************/

//#include <stdafx.h>
#pragma once

//#define MD5_DIGEST_LENGTH 16  // 16 bytes == 128 bit digest

//#ifdef _WIN32
//#include <wtypes.h>
//#endif


// MD5 Hash
struct MD5Context
{
  unsigned int buf[4];
  unsigned int bits[2];
  unsigned char in[64];
};

void  MD5Init(MD5Context *context);
void  MD5Update(MD5Context *context, unsigned char const *buf, unsigned int len);
void  MD5Final(unsigned char digest[16], MD5Context *context);
char* MD5Print(unsigned char digest[16]);

// Расчитать хэш значения из строки BSTR
// bool MakeDigestFromBSTR(unsigned short* str, unsigned char digest[16]);
// bool MakeDigestFromSTR(char *str, unsigned char digest[16]);
