#ifndef FILERECORD_H
#define FILERECORD_H

#include <QObject>
#include <QThread>
#include <QDateTime>
#include <Windows.h>
#include "Universal.h"
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include <libavutil/time.h>
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include "libavutil/audio_fifo.h"

};

#define MAX_AUDIO_FRAME_SIZE 192000

class FileRecord : public QThread
{
    Q_OBJECT
public:
    FileRecord(QObject *parent = nullptr);
    ~FileRecord();

public:
    int Init();
    int RecordStart(); //开始录制
    QString RecordStop(); //停止推流

private:
    int VideoCaptureInit(); //视频采集初始化
    int AudioCaptureInit(); //音频采集初始化
    int H264RecodeInit(); //H264编码器初始化
    int AacRecodeInit(); //aac编码器初始化
    int AudioFilterInit(); //音频滤镜初始化,用于重采样
    int OutPutInit(); //输出初始化

    int VideoCaptureStart(); //开始视频录制
    int AudioCaptureStart(); //开始音频录制

protected:
    virtual void run() override;

private:
    static int interrupt_callback(void *p); //超时回调

signals:
    void RecordEnd();

private:
    bool isRunning = true;
    bool isAudioInit = true;
    static Runner input_runner;
    int flag = 0; //超时标志
    QDateTime startTime;
    QString fileName;

/********************视频捕捉***********************/
    AVFormatContext* pVideoInFormatCtx = NULL; //桌面输入句柄
    const AVCodec* pVideoCodec = NULL; //桌面解码器
    AVCodecContext* pVideoCodecCtx = NULL; //桌面解码器句柄


/********************音频捕捉***********************/
    AVFormatContext* pAudioInFormatCtx = NULL; //麦克风输入句柄
    const AVCodec* pAudioCodec = NULL; //麦克风解码器
    AVCodecContext* pAudioCodecCtx = NULL;  //麦克风解码器句柄
    AVFilterContext* pAudioBuffersrcCtx = NULL;
    AVFilterContext* pAudioBuffersinkCtx = NULL;

/*********************输出**********************/
    AVFormatContext* pOutFormatCtx = NULL; //输出上下文
    const AVCodec* pH264Codec = NULL; //H264视频编码器
    AVCodecContext* pH264CodecCtx= NULL; //H264视频编码器句柄
    const AVCodec* pAacCodec = NULL; //aac音频编码器
    AVCodecContext* pAacCodecCtx= NULL; //aac音频编码器句柄

/********************缓冲***********************/
    AVFifoBuffer* pVideoFifo = NULL; //视频缓冲
    AVAudioFifo* pAudioFifo = NULL; //音频缓冲

/********************流索引***********************/
    int videoIndex = -1; //桌面视频流索引
    int audioIndex = -1; //麦克风音频流索引
    int outVideoIndex = -1; //视频输出流索引
    int outAudioIndex = -1; //音频输出流索引

};

#endif // FILERECORD_H
