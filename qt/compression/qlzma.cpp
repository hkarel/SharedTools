#include "qlzma.h"
#include "p7zip/C/Alloc.h"
#include "p7zip/C/LzmaDec.h"
#include "p7zip/C/LzmaEnc.h"
#include "p7zip/C/Lzma2Dec.h"
#include "p7zip/C/Lzma2Enc.h"
#include "container_ptr.h"
#include "spin_locker.h"
#include "thread/thread_info.h"

#include <QDataStream>
#include <QIODevice>
#include <atomic>
#include <map>

#define IN_BUF_SIZE (1 << 16)
#define OUT_BUF_SIZE (1 << 16)

#ifndef Q_DATA_STREAM_VERSION
#  if QT_VERSION >= 0x050000
#    define Q_DATA_STREAM_VERSION QDataStream::Qt_5_5
#  else
#    define Q_DATA_STREAM_VERSION QDataStream::Qt_4_8
#  endif
#endif

namespace {

struct CLzmaEncT
{
    CLzmaEncHandle handle = {0};

    template<typename T>
    struct Alloc
    {
        static T* create() {
            T* enc = new T;
            enc->handle = LzmaEnc_Create(&g_Alloc);
            return enc;
        }
        static void destroy(T* enc) {
            LzmaEnc_Destroy(enc->handle, &g_Alloc, &g_Alloc);
            delete enc;
        }
    };
    typedef container_ptr<CLzmaEncT, Alloc> Ptr;
};

struct CLzma2EncT
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
    typedef container_ptr<CLzma2EncT, Alloc> Ptr;
};

static std::map<pid_t, CLzmaEncT::Ptr>  lzmaEncMap;
static std::atomic_flag lzmaEncMapLock = ATOMIC_FLAG_INIT;

static std::map<pid_t, CLzma2EncT::Ptr> lzma2EncMap;
static std::atomic_flag lzma2EncMapLock = ATOMIC_FLAG_INIT;

struct SeqInStream : ISeqInStream
{
    QIODevice* io;
};

struct SeqOutStream : ISeqOutStream
{
    QIODevice* io;
};

SRes ioRead(void* p, void* buf, size_t* size)
{
    SeqInStream* inStream = static_cast<SeqInStream*>(p);
    int r = inStream->io->read((char*)buf, *size);
    if (r >= 0)
    {
        *size = r;
        return SZ_OK;
    }
    return SZ_ERROR_READ;
}

size_t ioWrite(void* p, const void* buf, size_t size)
{
    SeqOutStream* outStream = static_cast<SeqOutStream*>(p);
    return outStream->io->write((const char*)buf, size);
}

inline void lzmaDec_Init(CLzmaDec* state)
{
    LzmaDec_Init(state);
}

inline void lzmaDec_Init(CLzma2Dec* state)
{
    Lzma2Dec_Init(state);
}

inline SRes lzmaDec_DecodeToBuf(CLzmaDec *p, Byte *dest, SizeT *destLen,
                                const Byte *src, SizeT *srcLen,
                                ELzmaFinishMode finishMode, ELzmaStatus *status)
{
    return LzmaDec_DecodeToBuf(p, dest, destLen, src, srcLen, finishMode, status);
}

inline SRes lzmaDec_DecodeToBuf(CLzma2Dec *p, Byte *dest, SizeT *destLen,
                                const Byte *src, SizeT *srcLen,
                                ELzmaFinishMode finishMode, ELzmaStatus *status)
{
    return Lzma2Dec_DecodeToBuf(p, dest, destLen, src, srcLen, finishMode, status);
}

template<typename CLzmaDecType>
SRes lzmaDecompress(CLzmaDecType* state,
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
    lzmaDec_Init(state);
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

            res = lzmaDec_DecodeToBuf(state,
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

int lzmaCompress(const QByteArray& in, QByteArray& out, int compressionLevel,
                 quint8 lzmaVersion)
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
        instream.setVersion(Q_DATA_STREAM_VERSION);

        QDataStream outstream(&out, QIODevice::WriteOnly);
        outstream.setVersion(Q_DATA_STREAM_VERSION);

        SeqInStream inStream;
        inStream.Read = &ioRead;
        inStream.io = instream.device();

        SeqOutStream outStream;
        outStream.Write = &ioWrite;
        outStream.io = outstream.device();

        const int callsCountLimit = 5000;

        if (lzmaVersion == 2)
        {
            CLzma2EncT::Ptr enc;
            static int callsCount {0};
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
                    enc = CLzma2EncT::Ptr::create_ptr();
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
                outstream << quint8(2); // lzma version;
                outstream << uncompressedSize;
                outstream << lzmaProp;
                res = Lzma2Enc_Encode(enc->handle, &outStream, &inStream, NULL);
            }
        }
        else
        {
            CLzmaEncT::Ptr enc;
            static int callsCount {0};
            { //Block for SpinLocker
                SpinLocker locker(lzmaEncMapLock); (void) locker;
                if (++callsCount > callsCountLimit)
                {
                    // Чистим "мусор" оставшийся от завершенных потоков
                    lzmaEncMap.clear();
                    callsCount = 0;
                }
                pid_t tid = trd::gettid();
                enc = lzmaEncMap[tid];
                if (enc.empty())
                {
                    enc = CLzmaEncT::Ptr::create_ptr();
                    if (enc->handle == 0)
                        return SZ_ERROR_MEM;
                    lzmaEncMap[tid] = enc;
                }
            }

            CLzmaEncProps props;
            LzmaEncProps_Init(&props);
            props.level = compressionLevel;
            res = LzmaEnc_SetProps(enc->handle, &props);
            if (res != SZ_OK)
                return res;

            Byte lzmaHeader[LZMA_PROPS_SIZE];
            size_t lzmaHeaderSize = LZMA_PROPS_SIZE;
            res = LzmaEnc_WriteProperties(enc->handle, lzmaHeader, &lzmaHeaderSize);
            if (res != SZ_OK)
                return res;

            outstream << quint8(1); // lzma version;
            outstream << uncompressedSize;
            if (outstream.writeRawData((const char*) lzmaHeader, LZMA_PROPS_SIZE) != LZMA_PROPS_SIZE)
                return SZ_ERROR_PARAM;

            res = LzmaEnc_Encode(enc->handle, &outStream, &inStream, NULL, &g_Alloc, &g_Alloc);
        }
    }
    if (res != SZ_OK)
        out.clear();

    return res;
}


} // namespace


namespace qlzma {

void removeRancidEncoders()
{
    {
        SpinLocker locker(lzmaEncMapLock); (void) locker;
        lzmaEncMap.clear();
    }
    {
        SpinLocker locker(lzma2EncMapLock); (void) locker;
        lzma2EncMap.clear();
    }
}

int compress(const QByteArray& in, QByteArray& out, int compressionLevel)
{
    return lzmaCompress(in, out, compressionLevel, 1);
}

int compress2(const QByteArray& in, QByteArray& out, int compressionLevel)
{
    return lzmaCompress(in, out, compressionLevel, 2);
}

int decompress(const QByteArray& in, QByteArray& out)
{
    SRes res = SZ_OK;

    QDataStream inStream(in);
    inStream.setVersion(Q_DATA_STREAM_VERSION);

    quint8 lzmaVersion;
    inStream >> lzmaVersion;

    quint64 uncompressedSize;
    inStream >> uncompressedSize;

    out.clear();
    out.reserve(uncompressedSize * 1.05);

    if (lzmaVersion == 1)
    {
        QDataStream outStream(&out, QIODevice::WriteOnly);
        outStream.setVersion(Q_DATA_STREAM_VERSION);

        /* header: 5 bytes of LZMA properties */
        unsigned char header[LZMA_PROPS_SIZE];
        if (inStream.readRawData((char*) header, LZMA_PROPS_SIZE) == LZMA_PROPS_SIZE)
        {
            CLzmaDec state;
            LzmaDec_Construct(&state);
            res = LzmaDec_Allocate(&state, header, LZMA_PROPS_SIZE, &g_Alloc);
            if (res == SZ_OK)
            {
                res = lzmaDecompress(&state, &inStream, &outStream, uncompressedSize);
                LzmaDec_Free(&state, &g_Alloc);
            }
        }
        else
            res = SZ_ERROR_PARAM;
    }
    else if (lzmaVersion == 2)
    {
        QDataStream outStream(&out, QIODevice::WriteOnly);
        outStream.setVersion(Q_DATA_STREAM_VERSION);

        quint8 lzmaProp;
        inStream >> lzmaProp;

        CLzma2Dec state;
        Lzma2Dec_Construct(&state);
        res = (Lzma2Dec_Allocate(&state, lzmaProp, &g_Alloc));
        if (res == SZ_OK)
        {
            res = lzmaDecompress(&state, &inStream, &outStream, uncompressedSize);
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
