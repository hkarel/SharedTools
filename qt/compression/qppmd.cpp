/*****************************************************************************
  The MIT License

  Copyright © 2016 Pavel Karelin (hkarel), <hkarel@yandex.ru>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*****************************************************************************/

#include "qppmd.h"
#include "p7zip/C/Alloc.h"
#include "p7zip/C/Ppmd7.h"
#include "simple_ptr.h"
#include "qt/stream_init.h"

#include <QDataStream>
#include <stdexcept>

#define IN_BUF_SIZE (1 << 18)
#define OUT_BUF_SIZE (1 << 18)

namespace {

static const UInt32 kPropSize = 5;
static const UInt32 kBufSize = (1 << 20);
static const Byte kOrders[10] = { 3, 4, 4, 5, 5, 6, 8, 16, 24, 32 };

//------------------------------- InBufPtr -----------------------------------

template<typename> struct BufAlloc
{
    static Byte* create() {return (Byte*)::MidAlloc(kBufSize);}
    static void destroy(Byte* x) {::MidFree(x);}
};
typedef simple_ptr<Byte, BufAlloc>  InBufPtr;
typedef simple_ptr<Byte, BufAlloc>  OutBufPtr;

//------------------------------ EncProps ------------------------------------

struct EncProps
{
  UInt32 MemSize;
  UInt32 ReduceSize;
  int Order;

  EncProps();
  void Normalize(int level);
};

EncProps::EncProps()
{
  MemSize = (UInt32)(Int32)-1;
  ReduceSize = (UInt32)(Int32)-1;
  Order = -1;
}

void EncProps::Normalize(int level)
{
  if (level < 0) level = 5;
  if (level > 9) level = 9;
  if (MemSize == (UInt32)(Int32)-1)
    MemSize = level >= 9 ? ((UInt32)192 << 20) : ((UInt32)1 << (level + 19));
  const unsigned kMult = 16;
  if (MemSize / kMult > ReduceSize)
  {
    for (unsigned i = 16; i <= 31; i++)
    {
      UInt32 m = (UInt32)1 << i;
      if (ReduceSize <= m / kMult)
      {
        if (MemSize > m)
          MemSize = m;
        break;
      }
    }
  }
  if (Order == -1) Order = kOrders[(unsigned)level];
}

//-------------------------------- ByteOutBuf --------------------------------

struct ByteOutBuf : IByteOut
{
    Byte* cur = {0};
    Byte* buf = {0};
    const Byte* lim = {0};
    size_t size = {0};
    QDataStream* stream;

    ByteOutBuf();
    ~ByteOutBuf();
    bool alloc(size_t size);
    void free();
    void init();
    void flush();
    void writeByte(Byte b);
    static void wrapWriteByte(void* pp, Byte b);
};

ByteOutBuf::ByteOutBuf()
{
    Write = wrapWriteByte;
}

ByteOutBuf::~ByteOutBuf()
{
    free();
}

bool ByteOutBuf::alloc(size_t size)
{
    if (buf == 0 || this->size != size)
    {
        free();
        buf = (Byte*)::MidAlloc(size);
        this->size = size;
    }
    return (buf != 0);
}

void ByteOutBuf::free()
{
    ::MidFree(buf);
    lim = cur = buf = 0;
}

void ByteOutBuf::init()
{
    cur = buf;
    lim = buf + size;
}

void ByteOutBuf::flush()
{
    size_t size = (cur - buf);
    stream->writeRawData((const char*) buf, size);
    cur = buf;
}

void ByteOutBuf::writeByte(Byte b)
{
    *cur++ = b;
    if (cur == lim)
        flush();
}

void ByteOutBuf::wrapWriteByte(void* pp, Byte b)
{
    static_cast<ByteOutBuf*>(pp)->writeByte(b);
//    ByteOutBuf *p = static_cast<ByteOutBuf*>(pp);
//    Byte* dest = p->cur;
//    *dest = b;
//    p->cur = ++dest;
//    if (dest == p->lim)
//        p->flush();
}

//------------------------------- ByteInBuf ----------------------------------

struct ByteInBuf : IByteIn
{
    Byte* buf = {0};
    const Byte* cur = {0};
    const Byte* lim = {0};
    size_t size = {0};
    QDataStream* stream;
    bool extra = {false};

    ByteInBuf();
    ~ByteInBuf();
    bool alloc(size_t size);
    void free();
    void init();
    Byte readByteFromNewBlock();
    Byte readByte();
    static Byte wrapReadByte(void* pp);
};

ByteInBuf::ByteInBuf(): buf(0)
{
    Read = wrapReadByte;
}

ByteInBuf::~ByteInBuf()
{
    free();
}

bool ByteInBuf::alloc(size_t size)
{
    if (buf == 0 || this->size != size)
    {
        free();
        lim = cur = buf = (Byte *)::MidAlloc(size);
        this->size = size;
    }
    return (buf != 0);
}

void ByteInBuf::free()
{
    ::MidFree(buf);
    lim = cur = buf = 0;
}

void ByteInBuf::init()
{
    lim = cur = buf;
    extra = false;
}

Byte ByteInBuf::readByteFromNewBlock()
{
    if (!extra)
    {
        size_t avail = stream->readRawData((char*)buf, size);
        cur = buf;
        lim = buf + avail;
        if (avail > 0)
            return *cur++;
    }
    extra = true;
    return 0;
}

Byte ByteInBuf::readByte()
{
    if (cur != lim)
        return *cur++;
    return readByteFromNewBlock();
}

Byte ByteInBuf::wrapReadByte(void* pp)
{
    return static_cast<ByteInBuf*>(pp)->readByte();
    //ByteInBuf *p = static_cast<ByteInBuf*>(pp);
    //if (p->cur != p->lim)
    //    return *p->cur++;
    //return p->readByteFromNewBlock();
}

} // namespace


namespace qppmd {

int compress(const QByteArray& in, QByteArray& out, int compressionLevel)
{
    compressionLevel = qBound(-1, compressionLevel, 9);
    if (compressionLevel == -1)
        compressionLevel = 6;

    quint64 uncompressedSize = in.size();

    out.clear();
    out.reserve(uncompressedSize * 0.7);

    InBufPtr inBuf = InBufPtr::create_ptr();
    if (inBuf.get() == 0)
        return SZ_ERROR_MEM;

    { //Block for QDataStream
        QDataStream instream(in);
        STREAM_INIT(instream);

        QDataStream outstream(&out, QIODevice::WriteOnly);
        STREAM_INIT(outstream);

        EncProps encProps;
        encProps.Normalize(compressionLevel);

        Byte props[kPropSize];
        props[0] = (Byte)encProps.Order;
        SetUi32(props + 1, encProps.MemSize);

        outstream << quint8(7); // ppmd version
        outstream << uncompressedSize;
        if (outstream.writeRawData((const char*) props, kPropSize) != kPropSize)
            return SZ_ERROR_PARAM;

        ByteOutBuf outStream;
        if (!outStream.alloc(1 << 20))
            return SZ_ERROR_MEM;

        outStream.stream = &outstream;
        outStream.init();

        CPpmd7z_RangeEnc rangeEnc;
        rangeEnc.Stream = &outStream;

        CPpmd7 ppmd;
        Ppmd7_Construct(&ppmd);
        if (!Ppmd7_Alloc(&ppmd, encProps.MemSize, &g_Alloc))
            return SZ_ERROR_MEM;

        Ppmd7z_RangeEnc_Init(&rangeEnc);
        Ppmd7_Init(&ppmd, encProps.Order);

        for (;;)
        {
            int size = instream.readRawData((char*)inBuf.get(), kBufSize);
            if (size <= 0)
            {
                // We don't write EndMark in PPMD-7z.
                // Ppmd7_EncodeSymbol(&_ppmd, &_rangeEnc, -1);
                Ppmd7z_RangeEnc_FlushData(&rangeEnc);
                outStream.flush();
                break;
            }
            for (int i = 0; i < size; ++i)
            {
                Ppmd7_EncodeSymbol(&ppmd, &rangeEnc, inBuf.get()[i]);
            }
        }
        Ppmd7_Free(&ppmd, &g_Alloc);
    }
    return SZ_OK;
}

int decompress(const QByteArray& in, QByteArray& out)
{
    QDataStream instream(in);
    STREAM_INIT(instream);

    quint8 ppmdVersion;
    instream >> ppmdVersion;
    if (ppmdVersion != 7)
        throw std::logic_error("Unsupported ppmd version");

    quint64 uncompressedSize;
    instream >> uncompressedSize;

    Byte props[kPropSize];
    if (instream.readRawData((char*) props, kPropSize) != kPropSize)
        return SZ_ERROR_MEM;

    out.clear();
    out.reserve(in.size() * 2);

    OutBufPtr outBuf = OutBufPtr::create_ptr();
    if (outBuf.get() == 0)
        return SZ_ERROR_MEM;

    SRes res = SZ_OK;
    { //Block for QDataStream
        QDataStream outstream(&out, QIODevice::WriteOnly);
        STREAM_INIT(outstream);

        Byte order = props[0];
        UInt32 memSize = GetUi32(props + 1);
        if (order < PPMD7_MIN_ORDER
            || order > PPMD7_MAX_ORDER
            || memSize < PPMD7_MIN_MEM_SIZE
            || memSize > PPMD7_MAX_MEM_SIZE)
            return SZ_ERROR_PARAM;

        ByteInBuf inStream;
        if (!inStream.alloc(1 << 20))
            return SZ_ERROR_MEM;

        inStream.stream = &instream;
        inStream.init();

        CPpmd7z_RangeDec rangeDec;
        Ppmd7z_RangeDec_CreateVTable(&rangeDec);
        rangeDec.Stream = &inStream;

        CPpmd7 ppmd;
        Ppmd7_Construct(&ppmd);
        if (!Ppmd7_Alloc(&ppmd, memSize, &g_Alloc))
            return SZ_ERROR_MEM;

        if (!Ppmd7z_RangeDec_Init(&rangeDec))
            return SZ_ERROR_READ;

        Ppmd7_Init(&ppmd, order);
        quint64 processedSize = 0;

        do {
            const quint64 startPos = processedSize;

            UInt32 bufSize = kBufSize;
            UInt32 rem = UInt32(uncompressedSize - processedSize);
            if (bufSize > rem)
                bufSize = rem;

            UInt32 i;
            int sym = 0;
            for (i = 0; i != bufSize; i++)
            {
                sym = Ppmd7_DecodeSymbol(&ppmd, &rangeDec.p);
                if (inStream.extra || (sym < 0))
                    break;
                outBuf.get()[i] = (Byte)sym;
            }
            processedSize += i;
            int processed = int(processedSize - startPos);
            if (outstream.writeRawData((const char*)outBuf.get(), processed) != processed)
            {
                res = SZ_ERROR_MEM;
                break;
            }
            if (inStream.extra)
            {
                res = SZ_ERROR_READ;
                break;
            }
            if (sym < 0)
            {
                if (sym < -1)
                    res = SZ_ERROR_FAIL;
                break;
            }
        }
        while (processedSize < uncompressedSize);

        Ppmd7_Free(&ppmd, &g_Alloc);
    }
    if (res != SZ_OK)
        out.clear();

    return res;
}

} // namespace qppmd
