#include "Universal.h"
#include <QDateTime>
#include <QSettings>
#include <Windows.h>
#include <WtsApi32.h>
#include <QMessageBox>
ConfigInfo* GlobalData::m_pConfigInfo = nullptr;

void ConfigInfo::init()
{
    QSettings* config = new QSettings("Config.ini", QSettings::IniFormat);


    //TCP-IP
    localIp = config->value("/Address/ServerAddress").toString();
    localPORT =  config->value("/Address/ServerPort").toUInt();

    //视频拉流
    url = config->value("/Video/Url").toString();
    width = config->value("/Video/VideoWidth").toInt();
    height = config->value("/Video/VideoHeight").toInt();
    pos_x = config->value("/Video/Pos_x").toInt();
    pos_y = config->value("/Video/Pos_y").toInt();

    //视频录制
    frameRate = config->value("/FileRecord/FrameRate").toInt();
    videoBitRate = config->value("/FileRecord/VideoBitRate").toInt();
    desktopRange_x = config->value("/FileRecord/Range_x").toInt();
    desktopRange_y = config->value("/FileRecord/Range_y").toInt();
    desktopWidth = config->value("/FileRecord/Width").toInt();
    desktopHeight = config->value("/FileRecord/Height").toInt();
    sampleRate = config->value("/FileRecord/SampleRate").toInt();
    audioBitRate = config->value("/FileRecord/AudioBitRate").toInt();
    maxTime = config->value("/FileRecord/MaxTime").toInt();

    //车站信息
    stationName = config->value("/station/Name").toString();
    sreverIP = config->value("/station/ServerIp").toString();
    userName = config->value("/station/UserName").toString();
    password = config->value("/station/Password").toString();
    savePath = config->value("/station/SavePath").toString();

    delete config;
}

int GlobalData::getBigEndian_2Bit(QByteArray bigArray)
{
    int array = ((unsigned char)bigArray[0] << 8) + (unsigned char)bigArray[1];
    int L = array / 0x100;
    int H = array % 0x100;
    return (H << 8) + L;
}

QDateTime GlobalData::getDateTimeByArray(QByteArray data)
{
    QDateTime dateTime;
    int year = getBigEndian_2Bit(data.left(2));
    int month = data[2];
    int day = data[3];
    int hour = data[4];
    int minute = data[5];
    int second = data[6];
    QDate date;
    date.setDate(year, month, day);
    QTime time;
    time.setHMS(hour, minute, second);
    dateTime.setDate(date);
    dateTime.setTime(time);

    return dateTime;
}

bool GlobalData::IsWindows7()
{
    bool bRet = false;
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    OSVERSIONINFOEX os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (GetVersionEx((OSVERSIONINFO*)&os))
    {
        if (os.dwMajorVersion == 6 && os.dwMinorVersion == 1)
            bRet = true;
    }
    return  bRet;
}

bool GlobalData::IsSessionLocked()
{
    typedef BOOL(PASCAL * WTSQuerySessionInformation)(HANDLE hServer, DWORD SessionId, WTS_INFO_CLASS WTSInfoClass, LPTSTR* ppBuffer, DWORD* pBytesReturned);
    typedef void (PASCAL * WTSFreeMemory)(PVOID pMemory);

    WTSINFOEXW * pInfo = NULL;
    WTS_INFO_CLASS wtsic = WTSSessionInfoEx;
    bool bRet = IsWindows7();
    //bool bRet = true;
    LPTSTR ppBuffer = NULL;
    DWORD dwBytesReturned = 0;
    LONG dwFlags = 0;
    WTSQuerySessionInformation pWTSQuerySessionInformation = NULL;
    WTSFreeMemory pWTSFreeMemory = NULL;

    HMODULE hLib = LoadLibrary(L"wtsapi32.dll");
    if (!hLib)
    {
        return false;
    }
    pWTSQuerySessionInformation = (WTSQuerySessionInformation)GetProcAddress(hLib, "WTSQuerySessionInformationW");
    if (pWTSQuerySessionInformation)
    {
        pWTSFreeMemory = (WTSFreeMemory)GetProcAddress(hLib, "WTSFreeMemory");
        if (pWTSFreeMemory != NULL)
        {
            DWORD dwSessionID = WTSGetActiveConsoleSessionId();
            if (pWTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, dwSessionID, wtsic, &ppBuffer, &dwBytesReturned))
            {
                if (dwBytesReturned > 0)
                {
                    pInfo = (WTSINFOEXW*)ppBuffer;
                    if (pInfo->Level == 1)
                    {
                        dwFlags = pInfo->Data.WTSInfoExLevel1.SessionFlags;
                    }
                    if (dwFlags == WTS_SESSIONSTATE_LOCK)
                    {
                        bRet = !bRet;
                    }
                }
                pWTSFreeMemory(ppBuffer);
                ppBuffer = NULL;
            }
        }
    }
    if (hLib != NULL)
    {
        FreeLibrary(hLib);
    }
    return bRet;
}
