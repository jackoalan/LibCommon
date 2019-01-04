#include "FileUtil.h"
#include "Macros.h"
#include "Common/FileIO/CFileInStream.h"
#include <system_error>

#include <fstream>

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(__APPLE__)
#define UNIX
#endif

#ifdef UNIX
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#else
// TODO: Windows support
#endif

namespace FileUtil
{

#define ToPath(Path) u8path(Path)

bool Exists(const TString &rkFilePath)
{
    std::ifstream Stream(*rkFilePath);
    return Stream.good();
}

bool IsRoot(const TString& rkPath)
{
    // todo: is this actually a good/multiplatform way of checking for root?
    TString AbsPath = MakeAbsolute(rkPath);
    TStringList Split = AbsPath.Split("\\/");
    return (Split.size() <= 1);
}

bool IsFile(const TString& rkFilePath)
{
#ifdef UNIX
    struct stat Stats = {};
    return stat(*rkFilePath, &Stats) == 0 && S_ISREG(Stats.st_mode);
#else
    return is_regular_file(ToPath(*rkFilePath));
#endif
}

bool IsDirectory(const TString& rkDirPath)
{
#ifdef UNIX
    struct stat Stats = {};
    return stat(*rkDirPath, &Stats) == 0 && S_ISDIR(Stats.st_mode);
#else
    return is_directory(ToPath(*rkDirPath));
#endif
}

bool IsAbsolute(const TString& rkDirPath)
{
#ifdef UNIX
    return rkDirPath.StartsWith('/');
#else
    return ToPath(*rkDirPath).is_absolute();
#endif
}

bool IsRelative(const TString& rkDirPath)
{
    return !IsAbsolute(rkDirPath);
    //return ToPath(*rkDirPath).is_relative();
}

bool IsEmpty(const TString& rkDirPath)
{
    if (!IsDirectory(rkDirPath))
    {
        errorf("Non-directory path passed to IsEmpty(): %s", *rkDirPath);
        return false;
    }

#ifdef UNIX
    bool IsEmpty = false;
    DIR *pDir;
    pDir = opendir(*rkDirPath);
    if (pDir) {
        if (readdir(pDir) == nullptr) {
            IsEmpty = true;
        }
        closedir(pDir);
    }
    return IsEmpty;
#else
    return is_empty(ToPath(*rkDirPath));
#endif
}

bool MakeDirectory(const TString& rkNewDir)
{
    if (!IsValidPath(rkNewDir, true))
    {
        errorf("Unable to create directory because name contains illegal characters: %s", *rkNewDir);
        return false;
    }

#ifdef UNIX
    return mkdir(*rkNewDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#else
    return create_directories(ToPath(*rkNewDir));
#endif
}

bool CopyFile(const TString& rkOrigPath, const TString& rkNewPath)
{
    if (!IsValidPath(rkNewPath, false))
    {
        errorf("Unable to copy file because destination name contains illegal characters: %s", *rkNewPath);
        return false;
    }

    MakeDirectory(rkNewPath.GetFileDirectory());

#ifdef UNIX
    std::ifstream InputStream(*rkOrigPath, std::ios::binary);
    std::ofstream OutputStream(*rkNewPath, std::ios::out | std::ios::binary);
    char buffer[1024];
    while (!InputStream.eof() && InputStream.good()) {
        auto ReadCount = InputStream.read(buffer, 1024).gcount();
        OutputStream.write(buffer, ReadCount);
    }
    return true;
#else
    std::error_code Error;
    // call std::filesystem::copy, not std::copy
    std::experimental::filesystem::copy(ToPath(*rkOrigPath), ToPath(*rkNewPath), Error);
    return (Error.value() == 0);
#endif
}

bool CopyDirectory(const TString& rkOrigPath, const TString& rkNewPath)
{
    if (!IsValidPath(rkNewPath, true))
    {
        errorf("Unable to copy directory because destination name contains illegal characters: %s", *rkNewPath);
        return false;
    }

    MakeDirectory(rkNewPath.GetFileDirectory());

#ifdef UNIX
    bool IsOk = true;
    DIR *pDir;
    dirent *pDirEnt;
    pDir = opendir(*rkOrigPath);
    if (!pDir) {
        return false;
    }

    while ((pDirEnt = readdir(pDir)) == nullptr) {
        TString EntryPath = rkOrigPath + '/' + pDirEnt->d_name;
        TString DestPath = rkNewPath + '/' + pDirEnt->d_name;
        if (IsDirectory(EntryPath)) {
            if (!CopyDirectory(EntryPath, DestPath)) {
                IsOk = false;
                break;
            }
        } else {
            if (!CopyFile(EntryPath, DestPath)) {
                IsOk = false;
                break;
            }
        }
    }
    closedir(pDir);
    return IsOk;

#else
    std::error_code Error;
    // call std::filesystem::copy, not std::copy
    std::experimental::filesystem::copy(ToPath(*rkOrigPath), ToPath(*rkNewPath), Error);
    return (Error.value() == 0);
#endif
}

bool MoveFile(const TString& rkOldPath, const TString& rkNewPath)
{
    if (!IsValidPath(rkNewPath, false))
    {
        errorf("Unable to move file because destination name contains illegal characters: %s", *rkNewPath);
        return false;
    }

    if (Exists(rkNewPath))
    {
        errorf("Unable to move file because there is an existing file at the destination path: %s", *rkNewPath);
        return false;
    }

#ifdef UNIX
    return rename(*rkOldPath, *rkNewPath) == 0;
#else
    std::error_code Error;
    rename(ToPath(*rkOldPath), ToPath(*rkNewPath), Error);
    return Error.value() == 0;
#endif
}

bool MoveDirectory(const TString& rkOldPath, const TString& rkNewPath)
{
    if (!IsValidPath(rkNewPath, true))
    {
        errorf("Unable to move directory because destination name contains illegal characters: %s", *rkNewPath);
        return false;
    }

    if (Exists(rkNewPath))
    {
        errorf("Unable to move directory because there is an existing directory at the destination path: %s", *rkNewPath);
        return false;
    }

#ifdef UNIX
    return rename(*rkOldPath, *rkNewPath) == 0;
#else
    std::error_code Error;
    rename(ToPath(*rkOldPath), ToPath(*rkNewPath), Error);
    return Error.value() == 0;
#endif
}

bool DeleteFile(const TString& rkFilePath)
{
#ifdef UNIX
    return remove(*rkFilePath) == 0;
#else
    if (!IsFile(rkFilePath)) return false;
    return remove(ToPath(*rkFilePath)) == 1;
#endif
}

bool DeleteDirectory(const TString& rkDirPath, bool FailIfNotEmpty)
{
    // This is an extremely destructive function, be careful using it!
    if (!IsDirectory(rkDirPath)) return false;

    // Sanity check - don't delete root
    bool Root = IsRoot(rkDirPath);

    if (Root)
    {
        ASSERT(false);
        fatalf("Attempted to delete root directory!");
        return false;
    }

    // Check if directory is empty
    if (FailIfNotEmpty && !IsEmpty(rkDirPath))
        return false;

#ifdef UNIX
    bool IsOk = true;
    DIR *pDir;
    dirent *pDirEnt;
    pDir = opendir(*rkDirPath);
    if (!pDir) {
        return false;
    }

    while ((pDirEnt = readdir(pDir)) == nullptr) {
        TString EntryPath = rkDirPath + '/' + pDirEnt->d_name;
        if (IsDirectory(EntryPath)) {
            if (!DeleteDirectory(EntryPath, false)) {
                IsOk = false;
                break;
            }
        } else {
            if (!DeleteFile(EntryPath)) {
                IsOk = false;
                break;
            }
        }
    }

    closedir(pDir);
    return IsOk;
#else
    // Delete directory
    std::error_code Error;
    remove_all(ToPath(*rkDirPath), Error);
    return (Error.value() == 0);
#endif
}

bool ClearDirectory(const TString& rkDirPath)
{
    // This is an extremely destructive function, be careful using it!
    if (!IsDirectory(rkDirPath)) return false;

    // Sanity check - don't clear root
    bool Root = IsRoot(rkDirPath);

    if (Root)
    {
        ASSERT(false);
        fatalf("Attempted to clear root directory!");
        return false;
    }

    // Delete directory contents
    TStringList DirContents;
    GetDirectoryContents(rkDirPath, DirContents, false);

    for (auto It = DirContents.begin(); It != DirContents.end(); It++)
    {
        bool Success = false;

        if (IsFile(*It))
            Success = DeleteFile(*It);
        else if (IsDirectory(*It))
            Success = DeleteDirectory(*It, false);

        if (!Success)
            errorf("Failed to delete filesystem object: %s", *TString(*It));
    }

    return true;
}

uint64 FileSize(const TString &rkFilePath)
{
#ifdef UNIX
    struct stat st = {};
    int status = stat(*rkFilePath, &st);
    if (status != 0) {
        return static_cast<uint64>(-1);
    }
    return static_cast<uint64>(st.st_size);
#else
    return (uint64) (Exists(rkFilePath) ? file_size(ToPath(*rkFilePath)) : -1);
#endif
}

uint64 LastModifiedTime(const TString& rkFilePath)
{
#ifdef UNIX
    struct stat st = {};
    int status = stat(*rkFilePath, &st);
    if (status != 0) {
        return 0;
    }
    return static_cast<uint64>(st.st_mtime);
#else
    return (uint64) last_write_time(ToPath(*rkFilePath)).time_since_epoch().count();
#endif
}

TString WorkingDirectory()
{
#ifdef UNIX
    uint32_t Size = 64;
    while (true)
    {
        char Path[Size];
        if (getcwd(Path, Size) == nullptr)
        {
            if (errno != ERANGE)
            {
                break;
            }
            Size += 64;
            continue;
        }

        return Path;
    }
    return "";
#else
    return current_path().string();
#endif
}

TString MakeAbsolute(TString Path)
{
    if (IsAbsolute(Path)) {
        return Path;
    }
    return WorkingDirectory() + "/" + Path;

//    TStringList Components = Path.Split("/\\");
//    TStringList::iterator Prev;
//
//    for (TStringList::iterator Iter = Components.begin(); Iter != Components.end(); Iter++)
//    {
//        if (*Iter == ".")
//            Iter = Components.erase(Iter);
//        else if (*Iter == "..")
//            Iter = std::prev(Components.erase(Prev, std::next(Iter)));
//
//        Prev = Iter;
//    }
//
//    TString Out;
//    for (auto it = Components.begin(); it != Components.end(); it++)
//        Out += *it + "/";
//
//    return Out;
}

TString MakeRelative(const TString& rkPath, const TString& rkRelativeTo /*= WorkingDirectory()*/)
{
    TString AbsPath = MakeAbsolute(rkPath);
    TString AbsRelTo = MakeAbsolute(rkRelativeTo);
    TStringList PathComponents = AbsPath.Split("/\\");
    TStringList RelToComponents = AbsRelTo.Split("/\\");

    // Find furthest common parent
    TStringList::iterator PathIter = PathComponents.begin();
    TStringList::iterator RelToIter = RelToComponents.begin();

    for (; PathIter != PathComponents.end() && RelToIter != RelToComponents.end(); PathIter++, RelToIter++)
    {
        if (*PathIter != *RelToIter)
            break;
    }

    // If there's no common components, then we can't construct a relative path...
    if (PathIter == PathComponents.begin())
        return AbsPath;

    // Construct output path
    TString Out;

    for (; RelToIter != RelToComponents.end(); RelToIter++)
        Out += "../";

    for (; PathIter != PathComponents.end(); PathIter++)
        Out += *PathIter + "/";

    // Attempt to detect if this path is a file as opposed to a directory; if so, remove trailing backslash
    if (PathComponents.back().Contains('.') && !rkPath.EndsWith('/') && !rkPath.EndsWith('\\'))
        Out = Out.ChopBack(1);

    return Out;
}

TString SimplifyRelativePath(const TString& rkPath)
{
    TStringList PathComponents = rkPath.Split("/\\");

    TStringList::iterator Iter = PathComponents.begin();
    TStringList::iterator PrevIter = Iter;

    for (auto Iter = PathComponents.begin(); Iter != PathComponents.end(); PrevIter = Iter, Iter++)
    {
        if (*Iter == ".." && *PrevIter != "..")
        {
            PrevIter = PathComponents.erase(PrevIter);
            PrevIter = PathComponents.erase(PrevIter);
            Iter = PrevIter;
            Iter--;
        }
    }

    TString Out;

    for (auto Iter = PathComponents.begin(); Iter != PathComponents.end(); Iter++)
        Out += *Iter + '/';

    return Out;
}

uint32 MaxFileNameLength()
{
    return 255;
}

static const char gskIllegalNameChars[] = {
    '<', '>', '\"', '/', '\\', '|', '?', '*', ':'
};

TString SanitizeName(TString Name, bool Directory, bool RootDir /*= false*/)
{
    // Windows only atm
    if (Directory && (Name == "." || Name == ".."))
        return Name;

    // Remove illegal characters from path
    for (uint32 iChr = 0; iChr < Name.Size(); iChr++)
    {
        char Chr = Name[iChr];
        bool Remove = false;

        if (Chr >= 0 && Chr <= 31)
            Remove = true;

        // Allow colon only as the last character of root
        bool IsLegalColon = (Chr == ':' && RootDir && iChr == Name.Size() - 1);

        if (!IsLegalColon && !IsValidFileNameCharacter(Chr))
            Remove = true;

        if (Remove)
        {
            Name.Remove(iChr, 1);
            iChr--;
        }
    }

    // For directories, space and dot are not allowed at the end of the path
    if (Directory)
    {
        int ChopNum = 0;

        for (int iChr = (int) Name.Size() - 1; iChr >= 0; iChr--)
        {
            char Chr = Name[iChr];

            if (Chr == ' ' || Chr == '.')
                ChopNum++;
            else
                break;
        }

        if (ChopNum > 0) Name = Name.ChopBack(ChopNum);
    }

    // Remove spaces from beginning of path
    uint NumLeadingSpaces = 0;
    while (NumLeadingSpaces < Name.Size() && Name[NumLeadingSpaces] == ' ')
        NumLeadingSpaces++;

    if (NumLeadingSpaces > 0)
        Name = Name.ChopFront(NumLeadingSpaces);

    // Ensure the name is below the character limit
    if (Name.Size() > MaxFileNameLength())
    {
        int ChopNum = Name.Size() - MaxFileNameLength();
        Name = Name.ChopBack(ChopNum);
    }

    return Name;
}

TString SanitizePath(TString Path, bool Directory)
{
    TStringList Components = Path.Split("\\/");
    uint32 CompIdx = 0;
    Path = "";

    for (auto It = Components.begin(); It != Components.end(); It++)
    {
        TString Comp = *It;
        bool IsDir = Directory || CompIdx < Components.size() - 1;
        bool IsRoot = CompIdx == 0;
        Comp = SanitizeName(Comp, IsDir, IsRoot);

        Path += Comp;
        if (IsDir) Path += '/';
        CompIdx++;
    }

    return Path;
}

bool IsValidFileNameCharacter(char Chr)
{
    static const uint32 skNumIllegalChars = sizeof(gskIllegalNameChars) / sizeof(char);

    if (Chr >= 0 && Chr <= 31)
        return false;

    for (uint32 BanIdx = 0; BanIdx < skNumIllegalChars; BanIdx++)
    {
        if (Chr == gskIllegalNameChars[BanIdx])
            return false;
    }

    return true;
}

bool IsValidName(const TString& rkName, bool Directory, bool RootDir /*= false*/)
{
    // Only accounting for Windows limitations right now. However, this function should
    // ideally return the same output on all platforms to ensure projects are cross platform.
    if (rkName.IsEmpty())
        return false;

    if (rkName.Size() > MaxFileNameLength())
        return false;

    if (Directory && (rkName == "." || rkName == ".."))
        return true;

    // Check for banned characters
    for (uint32 iChr = 0; iChr < rkName.Size(); iChr++)
    {
        char Chr = rkName[iChr];

        // Allow colon only as the last character of root
        bool IsLegalColon = (Chr == ':' && RootDir && iChr == rkName.Size() - 1);

        if (!IsLegalColon && !IsValidFileNameCharacter(Chr))
            return false;
    }

    if (Directory && (rkName.Back() == ' ' || rkName.Back() == '.'))
        return false;

    return true;
}

bool IsValidPath(const TString& rkPath, bool Directory)
{
    // Only accounting for Windows limitations right now. However, this function should
    // ideally return the same output on all platforms to ensure projects are cross platform.
    TStringList Components = rkPath.Split("\\/");

    // Validate other components
    uint32 CompIdx = 0;

    for (auto It = Components.begin(); It != Components.end(); It++)
    {
        bool IsRoot = CompIdx == 0;
        bool IsDir = Directory || CompIdx < (Components.size() - 1);

        if (!IsValidName(*It, IsDir, IsRoot))
            return false;

        CompIdx++;
    }

    return true;
}

void GetDirectoryContents(TString DirPath, TStringList& rOut, bool Recursive /*= true*/, bool IncludeFiles /*= true*/, bool IncludeDirs /*= true*/)
{
    if (IsDirectory(DirPath))
    {
#ifdef UNIX
        DIR *pDir;
        dirent *pDirEnt;
        pDir = opendir(*DirPath);
        if (!pDir) {
            return;
        }

        while ((pDirEnt = readdir(pDir)) == nullptr) {
            TString EntryPath = DirPath + '/' + pDirEnt->d_name;
            if (IsDirectory(EntryPath)) {
                if (IncludeDirs) {
                    rOut.push_back(EntryPath);
                }
                if (Recursive) {
                    GetDirectoryContents(EntryPath, rOut, true, IncludeFiles, IncludeDirs);
                }
            } else {
                if (IncludeFiles) {
                    rOut.push_back(EntryPath);
                }
            }
        }

        closedir(pDir);
#else
        DirPath.Replace("\\", "/");
        bool IncludeAll = IncludeFiles && IncludeDirs;

        auto AddFileLambda = [IncludeFiles, IncludeDirs, IncludeAll, &rOut](const TString& rkPath) -> void {
            bool ShouldAddFile = IncludeAll || (IncludeFiles && IsFile(rkPath)) || (IncludeDirs && IsDirectory(rkPath));

            if (ShouldAddFile)
                rOut.push_back(rkPath);
        };

        if (Recursive)
        {
            for (recursive_directory_iterator It(ToPath(*DirPath)); It != recursive_directory_iterator(); ++It)
            {
                AddFileLambda(It->path().string());
            }
        }

        else
        {
            for (directory_iterator It(ToPath(*DirPath)); It != directory_iterator(); ++It)
            {
                AddFileLambda(It->path().string());
            }
        }
#endif
    }
}

TString FindFileExtension(const TString& rkDir, const TString& rkName)
{
#ifdef UNIX
    DIR *pDir;
    dirent *pDirEnt;
    pDir = opendir(*rkDir);
    if (!pDir) {
        return "";
    }

    TString Result = "";

    while ((pDirEnt = readdir(pDir)) == nullptr) {
        TString EntryPath = rkDir + '/' + pDirEnt->d_name;
        if (IsFile(EntryPath)) {
            if (EntryPath.GetFileName(false) == rkName) {
                Result = EntryPath.GetFileExtension();
            }
        }
    }

    closedir(pDir);
    return Result;
#else
    for (directory_iterator It(ToPath(*rkDir)); It != directory_iterator(); ++It)
    {
        TString Name = It->path().filename().string();
        if (Name.GetFileName(false) == rkName) return Name.GetFileExtension();
    }

    return "";
#endif
}

bool LoadFileToString(const TString& rkFilePath, TString& rOut)
{
    CFileInStream File(rkFilePath);

    if (File.IsValid())
    {
        rOut = TString(File.Size());
        File.ReadBytes(&rOut[0], rOut.Size());
        return true;
    }
    else
        return false;
}

}
