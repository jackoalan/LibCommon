#include "CTimer.h"
#include "Log.h"
#include "Macros.h"
#include "TString.h"

#include <ctime>
#include <iostream>
#include <fstream>

namespace NLog
{

TStringList gErrorLog;
TString gLogFilename;
//FILE *gpLogFile;
std::ofstream gLogFile;

double gAppStartTime = CTimer::GlobalTime();

bool gInitialized = false;
TStringList gPreInitLogs;


bool InitLog(const TString& rkFilename)
{
    gLogFile.open(*rkFilename, std::ios::out);
    gLogFilename = rkFilename;

    if (!gLogFile.good())
    {
        TString FileName = rkFilename.GetFileName(false);
        TString Extension = rkFilename.GetFileExtension();
        int Num = 0;

        while (!gLogFile.good())
        {
            if (Num > 999) break;
            TString NewFilename = FileName + "_" + TString::FromInt32(Num, 0, 10) + "." + Extension;
            gLogFile.open(*NewFilename, std::ios::out);
            Num++;
        }

        if (!gLogFile.good())
            return false;
    }

    // Print initial message to log
    time_t RawTime;
    time(&RawTime);

    tm TimeInfo;
#ifdef _MSC_VER
    localtime_s(&TimeInfo, &RawTime);
#else
    localtime_r(&RawTime, &TimeInfo);
#endif

    tm *pTimeInfo = localtime(&RawTime);

    char Buffer[80];
    strftime(Buffer, 80, "%m/%d/%y %H:%M:%S", &TimeInfo);

    gLogFile << "Opened log file at " << Buffer << std::endl;
    gLogFile.flush();

#ifdef APP_FULL_NAME
    // Print app name and version
    fprintf(gpLogFile, APP_FULL_NAME"\n");
#endif
    gInitialized = true;

    // Print any messages that were attempted before we initialized
    if (!gPreInitLogs.empty())
    {
        for (auto it = gPreInitLogs.begin(); it != gPreInitLogs.end(); it++)
            Writef(**it);

        gPreInitLogs.clear();
    }

    return true;
}

void WriteInternal(EMsgType Type, const char* pkMsg, va_list& VarArgs)
{
    char LineBuffer[512];
    double Time = CTimer::GlobalTime() - gAppStartTime;
    int Offset = sprintf(LineBuffer, "[%08.3f] ", Time);

    vsnprintf(&LineBuffer[Offset], 512, pkMsg, VarArgs);


    // Write to log file
    if (!gInitialized)
        gPreInitLogs.push_back(LineBuffer);

    else if (gLogFile.good())
    {
        gLogFile << LineBuffer << std::endl;
        gLogFile.flush();
    }

    std::cout << LineBuffer << "\n";
}

#define DEFINE_LOG_FUNC(MsgType) \
    va_list VarArgs; \
    va_start(VarArgs, pkMsg); \
    WriteInternal(MsgType, pkMsg, VarArgs); \
    va_end(VarArgs); \

void Writef(const char* pkMsg, ...)
{
    DEFINE_LOG_FUNC(EMsgType::Standard);
}

void Warnf(const char* pkMsg, ...)
{
    DEFINE_LOG_FUNC(EMsgType::Warning);
}

void Errorf(const char* pkMsg, ...)
{
    DEFINE_LOG_FUNC(EMsgType::Error);
    DEBUG_BREAK;
}

void Fatalf(const char* pkMsg, ...)
{
    DEFINE_LOG_FUNC(EMsgType::Fatal);
    DEBUG_BREAK;
    abort();
}

const TStringList& GetErrorLog()
{
    return gErrorLog;
}

void ClearErrorLog()
{
    gErrorLog.clear();
}

}
