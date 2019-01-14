#ifndef CFILELOCK_H
#define CFILELOCK_H

#include <cstdio>
#include <fstream>
#include "Common/TString.h"

// Maintain a file handle to prevent other processes from accessing the file.
class CFileLock
{
    std::ifstream mFile;

public:
    CFileLock()
    {}

    ~CFileLock()
    {
        Release();
    }

    void Lock(const TString& rkPath)
    {
        Release();
        mFile.open(*rkPath, std::ios::app);
    }

    void Release()
    {
        mFile.close();
    }
};

#endif // CFILELOCK_H
