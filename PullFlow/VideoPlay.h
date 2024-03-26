#ifndef VIDEOPLAY_H
#define VIDEOPLAY_H

#include <QObject>
#include <QThread>
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
};

#include <SDL.h>
#include <SDL_main.h>

typedef struct {
    time_t lasttime = 0;
    bool connected = false;
}RunnerV;


class VideoPlay : public QThread
{
    Q_OBJECT
public:
    VideoPlay(QObject *parent = nullptr);
    ~VideoPlay();

    int PullFlowInit(); //拉流信息初始化
    int SDLPlayerInit(); //SDL播放器初始化
    int SDLPlayerStart(); //开始拉流
    void PullFlowStop() { inRunning = false; }
    void run() override;

    bool IsRunning() { return inRunning; }

    void getWinId(void* id){ winid = id; }
    static int interrupt_callback(void *p); //超时回调

private:
    AVFormatContext* pInFormatCtx = NULL;
    AVFormatContext* pOutFormatCtx = nullptr;
    //视频解码器
    AVCodec* pCodec = NULL;
    AVCodecContext* pCodecCtx = NULL;

    //SDL播放器
    SDL_Window* screen;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* sdlTexture;
    SDL_Rect sdlRect;

    int videoIndex = -1; //视频流索引
    bool inRunning = true;

    QString url; //拉流地址
    int width = 0; //图像宽度
    int height = 0; //图像高度

    static RunnerV input_runner;
    void* winid = NULL;
    int flag = 0;
};

#endif // VIDEOPLAY_H
