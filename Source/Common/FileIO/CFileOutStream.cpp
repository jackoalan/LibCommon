#include "CFileOutStream.h"

CFileOutStream::CFileOutStream()
//    : mpFStream(nullptr)
    : mSize(0)
{
}

CFileOutStream::CFileOutStream(const TString& rkFile)
//    : mpFStream(nullptr)
{
    Open(rkFile, EEndian::BigEndian);
}

CFileOutStream::CFileOutStream(const TString& rkFile, EEndian FileEndianness)
//    : mpFStream(nullptr)
{
    Open(rkFile, FileEndianness);
}

CFileOutStream::CFileOutStream(const CFileOutStream& rkSrc)
//    : mpFStream(nullptr)
{
    Open(rkSrc.mName, rkSrc.mDataEndianness);

    if (rkSrc.IsValid())
        Seek(rkSrc.Tell(), SEEK_SET);
}

CFileOutStream::~CFileOutStream()
{
//    if (IsValid())
//        Close();
}

void CFileOutStream::Open(const TString& rkFile, EEndian FileEndianness)
{
    if (IsValid())
        Close();

    mFStream = std::ofstream(rkFile.Data(), std::ios::out | std::ios::binary);
    mName = rkFile;
    mDataEndianness = FileEndianness;
    mSize = 0;
}

void CFileOutStream::Update(const TString& rkFile, EEndian FileEndianness)
{
    if (IsValid())
        Close();

    mFStream = std::ofstream(rkFile.Data(), std::ios::out | std::ios::binary | std::ios::app);
    mName = rkFile;
    mDataEndianness = FileEndianness;

    Seek(0x0, SEEK_END);
    mSize = Tell();
    Seek(0x0, SEEK_SET);
}

void CFileOutStream::Close()
{
    mFStream.close();
    mSize = 0;
}

void CFileOutStream::WriteBytes(const void *pkSrc, uint32 Count)
{
    if (!IsValid()) return;
    mFStream.write(static_cast<const char *>(pkSrc), Count);
    if (Tell() > mSize) mSize = Tell();
}

bool CFileOutStream::Seek(int32 Offset, uint32 Origin)
{
    if (!IsValid()) return false;
    return Seek64(Offset, Origin);
}

bool CFileOutStream::Seek64(int64 Offset, uint32 Origin)
{
    if (!IsValid()) return false;
    std::ios::seekdir Dir = {};
    switch (Origin) {
        case SEEK_SET: Dir = std::ios::beg; break;
        case SEEK_CUR: Dir = std::ios::cur; break;
        case SEEK_END: Dir = std::ios::end; break;
    }

    mFStream.seekp(Offset, Dir);
    return !mFStream.fail();
}

uint32 CFileOutStream::Tell() const
{
    if (!IsValid()) return 0;
    return static_cast<uint32>(mFStream.tellp());
}

uint64 CFileOutStream::Tell64() const
{
    if (!IsValid()) return 0;
    return static_cast<uint64>(mFStream.tellp());
}

bool CFileOutStream::EoF() const
{
    return (Tell() == Size());
}

bool CFileOutStream::IsValid() const
{
    return mFStream.good();
}

uint32 CFileOutStream::Size() const
{
    if (!IsValid()) return 0;
    return mSize;
}

TString CFileOutStream::FileName() const
{
    return mName;
}
