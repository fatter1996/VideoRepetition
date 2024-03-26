#ifndef PULLFLOW_H
#define PULLFLOW_H

#include <QObject>
#include <QThread>
#include <QFile>
#include "Universal.h"
//Windows
#define __STDC_CONSTANT_MACROS
#define SDL_MAIN_HANDLED

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libavutil/audio_fifo.h"
};

#include <SDL.h>
#include <SDL_main.h>

class PullFlow : public QThread
{
    Q_OBJECT
public:
    PullFlow(QString rtmp = QString(), QObject *parent = nullptr);
    ~PullFlow();

public:
    void Init(void* winid); //初始化函数,递归调用,初始化失败会阻塞
    void PullFlowStop(); //停止拉流
    //bool IsRunning() { return isRunning; }

private:
    int PullFlowInit(); //拉流信息初始化
    int SDLPlayerInit(void* winid); //SDL播放器初始化
    int VideoStreamInit(); //视频流信息初始化
    int AudioStreamInit(); //音频流信息初始化
    int SDLPlayerStart(); //开始拉流
    void VideoPlay(); //视频播放
    void AudioPlay(); //音频播放
    void timerEvent(QTimerEvent * ev);

signals:
    void Stop();
    void Error();

public slots:
    void onPullFlowStop();

protected:
    virtual void run() override;

private:
    static int interrupt_callback(void* p); //超时回调
    static void fill_audio(void* udata, Uint8* stream, int len); //音频播放回调函数


private:
    QString url; //拉流地址
    int flag = 0; //超时标志
    int timeId = 0; //锁屏检测
    bool isRunning = true;
    bool isPullFlowInit = false;
    bool isSDLPlayerInit = false;
    bool isAudioinit = false;
    bool lockedState = false;
    bool isLockedStateChange = false;
    static Runner input_runner;
    //QFile* log = nullptr;

/***********************输入************************/
    AVFormatContext* pInFormatCtx = NULL;

/********************视频解码器***********************/
    AVCodec* pVidoeCodec = NULL;
    AVCodecContext* pVidoeCodecCtx = NULL;

/********************音频解码器***********************/
    AVCodec* pAudioCodec = NULL;
    AVCodecContext* pAudioCodecCtx = NULL;

/********************缓冲***********************/
    AVFifoBuffer* pVideoFifo = NULL; //视频缓冲
    AVAudioFifo* pAudioFifo = NULL; //音频缓冲

/********************视频播放器***********************/
    SDL_Window* pScreen = NULL; //视频显示窗口
    SDL_Renderer* sdlRenderer = NULL; //视频渲染
    SDL_Texture* sdlTexture = NULL; //视频纹理
    SDL_Rect sdlRect = {0,0,0,0}; //视频播放区域

/********************音频播放器***********************/
    static unsigned int audioLen; //一帧PCM的数据长度
    static unsigned char* audioChunk; //PCM数据
    static unsigned char* audioPos; //当前读取的位置

/********************流索引***********************/
    int videoIndex = -1; //视频流索引
    int audioIndex = -1; //视频流索引
};

#endif // PULLFLOW_H
