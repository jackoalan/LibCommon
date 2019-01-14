#include "CFileInStream.h"

CFileInStream::CFileInStream()
{
}

CFileInStream::CFileInStream(const TString& rkFile)
{
    Open(rkFile, EEndian::BigEndian);
}

CFileInStream::CFileInStream(const TString& rkFile, EEndian FileEndianness)
{
    Open(rkFile, FileEndianness);
}

CFileInStream::CFileInStream(const CFileInStream& rkSrc)
{
    Open(rkSrc.mName, rkSrc.mDataEndianness);

    if (rkSrc.IsValid())
        Seek(rkSrc.Tell(), SEEK_SET);
}

CFileInStream::~CFileInStream()
{
    if (IsValid())
        Close();
}

void CFileInStream::Open(const TString& rkFile, EEndian FileEndianness)
{
    if (IsValid())
        Close();

    mFStream = std::ifstream(rkFile.Data(), std::ios::binary);
    //mpFStream = fopen(ToWChar(rkFile), L"rb");
    mName = rkFile;
    mDataEndianness = FileEndianness;

    if (IsValid())
    {
        Seek(0x0, SEEK_END);
        mFileSize = Tell();
        Seek(0x0, SEEK_SET);
    }
    else
        mFileSize = 0;

    SetSourceString(rkFile.GetFileName());
}

void CFileInStream::Close()
{
    mFStream.close();
}

void CFileInStream::ReadBytes(void *pDst, uint32 Count)
{
    if (!IsValid()) return;
    mFStream.read(static_cast<char *>(pDst), Count);
}

bool CFileInStream::Seek(int32 Offset, uint32 Origin)
{
    if (!IsValid()) return false;
    return Seek64(Offset, Origin);
}

bool CFileInStream::Seek64(int64 Offset, uint32 Origin)
{
    if (!IsValid()) return false;

    std::ios::seekdir Dir = {};
    switch (Origin) {
        case SEEK_SET: Dir = std::ios::beg; break;
        case SEEK_CUR: Dir = std::ios::cur; break;
        case SEEK_END: Dir = std::ios::end; break;
    }
    mFStream.seekg(Offset, Dir);
    return !mFStream.fail();
}

uint32 CFileInStream::Tell() const
{
    if (!IsValid()) return 0;
    return static_cast<uint32>(mFStream.tellg());
}

uint64 CFileInStream::Tell64() const
{
    if (!IsValid()) return 0;
    return static_cast<uint64>(mFStream.tellg());
}

bool CFileInStream::EoF() const
{
    return (Tell() >= mFileSize);
}

bool CFileInStream::IsValid() const
{
    return mFStream.is_open() && !mFStream.bad();
}

uint32 CFileInStream::Size() const
{
    return mFileSize;
}

TString CFileInStream::FileName() const
{
    return mName;
}
