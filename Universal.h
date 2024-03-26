#ifndef UNIVERSAL_H
#define UNIVERSAL_H

#include <QObject>


#define MODE_ONLY
//#define MODE_MORE

#define MAX_AUDIO_FRAME_SIZE 192000


typedef struct {
    time_t lasttime;
    bool connected;
}Runner;

struct ConfigInfo
{
    //TCP-IP
    QString localIp;
    uint localPORT;

    //视频拉流
    QString url; //拉流地址
    int width = 0; //显示宽度
    int height = 0; //显示高度
    int pos_x = 0; //显示起始坐标-x
    int pos_y = 0; //显示起始坐标-y

    //视频录制
    int frameRate = 0; //帧率
    int videoBitRate = 0; //比特率
    int desktopRange_x = 0; //录制区域的起始坐标-x
    int desktopRange_y = 0; //录制区域的起始坐标-y
    int desktopWidth = 0; //图像宽度
    int desktopHeight = 0; //图像高度
    int sampleRate = 0; //采样率
    int audioBitRate = 0; //比特率
    uint maxTime = 0; //最大录制时间

    //车站信息
    QString stationName; //站名
    QString sreverIP; //共享服务器名称
    QString userName; //共享用户名
    QString password; //共享用户密码
    QString savePath; //共享文件夹路径

public:
    void init();
};

class GlobalData
{
public:
    static ConfigInfo* m_pConfigInfo;
    static int getBigEndian_2Bit(QByteArray bigArray);
    static QDateTime getDateTimeByArray(QByteArray data);
    static bool IsWindows7();
    static bool IsSessionLocked();
};



#endif // UNIVERSAL_H
