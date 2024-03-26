#ifndef AUDIOPLAY_H
#define AUDIOPLAY_H

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

#define MAX_AUDIO_FRAME_SIZE 192000

typedef struct {
    time_t lasttime;
    bool connected;
}RunnerA;

class AudioPlay : public QThread
{
    Q_OBJECT
public:
    AudioPlay(QObject *parent = nullptr);
    ~AudioPlay();


    int PullFlowInit(); //拉流信息初始化
    int SDLPlayerInit(); //SDL播放器初始化
    int SDLPlayerStart(); //开始拉流
    void PullFlowStop() { inRunning = false; }
    void run() override;

    bool IsRunning() { return inRunning; }

    static void fill_audio(void * udata, Uint8 * stream, int len); //音频播放回调函数
    static int interrupt_callback(void *p); //超时回调

private:
    AVFormatContext* pInFormatCtx = NULL;
    AVFormatContext* pOutFormatCtx = nullptr;
    //音频解码器
    AVCodec* pCodec = NULL;
    AVCodecContext* pCodecCtx = NULL;


    //音频流索引
    int audioIndex = -1;

    //SDL播放器
    SDL_Window* screen;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* sdlTexture;
    SDL_Rect sdlRect;


    bool inRunning = true;

    QString url; //拉流地址
    static RunnerA input_runner;
    //一帧PCM的数据长度
    static unsigned int audioLen;
    static unsigned char* audioChunk;
    //当前读取的位置
    static unsigned char* audioPos;

    int flag = 0;

};


#endif // AUDIOPLAY_H
