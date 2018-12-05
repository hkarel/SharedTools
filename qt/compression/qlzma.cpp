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

#include "qlzma.h"
#include "p7zip/C/Alloc.h"
#include "p7zip/C/LzmaDec.h"
#include "p7zip/C/LzmaEnc.h"
#include "p7zip/C/Lzma2Dec.h"
#include "p7zip/C/Lzma2Enc.h"
#include "container_ptr.h"
#include "spin_locker.h"
#include "qt/stream_init.h"
#include "thread/thread_info.h"

#include <QDataStream>
#include <stdexcept>
#include <atomic>
#include <map>

#define IN_BUF_SIZE (1 << 18)
#define OUT_BUF_SIZE (1 << 18)

namespace {

struct Lzma2Encoder
{
    CLzma2EncHandle handle = {0};

    template<typename T>
    struct Alloc
    {
        static T* create() {
            T* enc = new T;
            enc->handle = Lzma2Enc_Create(&g_Alloc, &g_Alloc);
            return enc;
        }
        static void destroy(T* enc) {
            Lzma2Enc_Destroy(enc->handle);
            delete enc;
        }
    };
    typedef container_ptr<Lzma2Encoder, Alloc> Ptr;
};

static std::map<pid_t, Lzma2Encoder::Ptr> lzma2EncMap;
static std::atomic_flag lzma2EncMapLock = ATOMIC_FLAG_INIT;

struct SeqInStream : ISeqInStream
{
    SeqInStream(QDataStream* s)
    {
        Read = ReadFunc;
        Stream = s;
    }
    static SRes ReadFunc(void* p, void* buf, size_t* size)
    {
        SeqInStream* inStream = static_cast<SeqInStream*>(p);
        int r = inStream->Stream->readRawData((char*)buf, *size);
        if (r >= 0)
        {
            *size = r;
            return SZ_OK;
        }
        return SZ_ERROR_READ;
    }
    QDataStream* Stream;
};

struct SeqOutStream : ISeqOutStream
{
    SeqOutStream(QDataStream* s)
    {
        Write = WriteFunc;
        Stream = s;
    }
    static size_t WriteFunc(void* p, const void* buf, size_t size)
    {
        SeqOutStream* outStream = static_cast<SeqOutStream*>(p);
        return outStream->Stream->writeRawData((const char*)buf, size);
    }
    QDataStream* Stream;
};

SRes lzmaDecode(CLzma2Dec* state,
                QDataStream* inStream,
                QDataStream* outStream,
                quint64 uncompressedSize)
{
    bool thereIsSize = (uncompressedSize != quint64(-1));
    Byte inBuf[IN_BUF_SIZE];
    Byte outBuf[OUT_BUF_SIZE];
    qint64 inPos = 0;
    qint64 inSize = 0;
    qint64 outPos = 0;
    Lzma2Dec_Init(state);
    for (;;)
    {
        if (inPos == inSize)
        {
            inSize = inStream->readRawData((char*)&inBuf, IN_BUF_SIZE);
            if (inSize == -1)
                return SZ_ERROR_READ;
            inPos = 0;
        }
        {
            SRes res;
            SizeT inProcessed = inSize - inPos;
            SizeT outProcessed = OUT_BUF_SIZE - outPos;
            ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
            ELzmaStatus status;
            if (thereIsSize && (outProcessed > uncompressedSize))
            {
                outProcessed = SizeT(uncompressedSize);
                finishMode = LZMA_FINISH_END;
            }

            res = Lzma2Dec_DecodeToBuf(state,
                                       outBuf + outPos,
                                       &outProcessed,
                                       inBuf + inPos,
                                       &inProcessed,
                                       finishMode,
                                       &status);
            inPos += inProcessed;
            outPos += outProcessed;
            uncompressedSize -= outProcessed;

            if (outStream->writeRawData((char*)&outBuf, outPos) != outPos)
                return SZ_ERROR_WRITE;

            outPos = 0;

            if (res != SZ_OK
                || (thereIsSize && uncompressedSize == 0))
                return res;

            if (inProcessed == 0 && outProcessed == 0)
            {
                if (thereIsSize
                    || status != LZMA_STATUS_FINISHED_WITH_MARK)
                    res =  SZ_ERROR_DATA;
                return res;
            }
        }
    }
}

} // namespace


namespace qlzma {

void removeRancidEncoders()
{
    SpinLocker locker(lzma2EncMapLock); (void) locker;
    lzma2EncMap.clear();
}

int compress(const QByteArray& in, QByteArray& out, int compressionLevel)
{
    compressionLevel = qBound(-1, compressionLevel, 9);
    if (compressionLevel == -1)
        compressionLevel = 6;

    quint64 uncompressedSize = in.size();

    out.clear();
    out.reserve(uncompressedSize * 0.7);

    SRes res = SZ_OK;
    { //Block for QDataStream
        QDataStream instream(in);
        STREAM_INIT(instream);

        QDataStream outstream(&out, QIODevice::WriteOnly);
        STREAM_INIT(outstream);

        SeqInStream inStream(&instream);
        SeqOutStream outStream(&outstream);

        static int callsCount {0};
        const  int callsCountLimit = 5000;

        Lzma2Encoder::Ptr enc;
        { //Block for SpinLocker
            SpinLocker locker(lzma2EncMapLock); (void) locker;
            if (++callsCount > callsCountLimit)
            {
                // Чистим "мусор" оставшийся от завершенных потоков
                lzma2EncMap.clear();
                callsCount = 0;
            }
            pid_t tid = trd::gettid();
            enc = lzma2EncMap[tid];
            if (enc.empty())
            {
                enc = Lzma2Encoder::Ptr::create_ptr();
                if (enc->handle == 0)
                    return SZ_ERROR_MEM;
                lzma2EncMap[tid] = enc;
            }
        }

        CLzma2EncProps props;
        Lzma2EncProps_Init(&props);
        props.lzmaProps.level = compressionLevel;

        res = Lzma2Enc_SetProps(enc->handle, &props);
        if (res == SZ_OK)
        {
            quint8 lzmaProp = Lzma2Enc_WriteProperties(enc->handle);
            outstream << quint8(2); // lzma version
            outstream << uncompressedSize;
            outstream << lzmaProp;
            res = Lzma2Enc_Encode(enc->handle, &outStream, &inStream, NULL);
        }
    }
    if (res != SZ_OK)
        out.clear();

    return res;
}

int decompress(const QByteArray& in, QByteArray& out)
{
    SRes res = SZ_OK;

    QDataStream inStream(in);
    STREAM_INIT(inStream);

    quint8 lzmaVersion;
    inStream >> lzmaVersion;

    quint64 uncompressedSize;
    inStream >> uncompressedSize;

    out.clear();
    out.reserve(uncompressedSize * 1.05);

    if (lzmaVersion == 2)
    {
        QDataStream outStream(&out, QIODevice::WriteOnly);
        STREAM_INIT(outStream);

        quint8 lzmaProp;
        inStream >> lzmaProp;

        CLzma2Dec state;
        Lzma2Dec_Construct(&state);
        res = (Lzma2Dec_Allocate(&state, lzmaProp, &g_Alloc));
        if (res == SZ_OK)
        {
            res = lzmaDecode(&state, &inStream, &outStream, uncompressedSize);
            Lzma2Dec_Free(&state, &g_Alloc);
        }
    }
    else
        throw std::logic_error("Unsupported lzma version");

    if (res != SZ_OK)
        out.clear();

    return res;
}

} // namespace qlzma
