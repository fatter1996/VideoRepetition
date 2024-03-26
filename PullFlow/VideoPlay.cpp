#include "VideoPlay.h"
#include <QDebug>
#include <QSettings>
#include "Log/log.h"

RunnerV VideoPlay::input_runner = {0};
VideoPlay::VideoPlay(QObject *parent)
{
    QSettings *config = new QSettings("Config.ini", QSettings::IniFormat);

    url = config->value("/Video/Url").toString();
    width = config->value("/Video/VideoWidth").toInt();
    height = config->value("/Video/VideoHeight").toInt();


    avdevice_register_all(); //注册设备
    avformat_network_init(); //加载socket库以及网络加密协议相关的库
}

VideoPlay::~VideoPlay()
{

}

int VideoPlay::PullFlowInit()
{
    int ret = 0;
    //打开收入流,传入参数,获取封装格式相关信息

    //AVDictionary* opts = NULL;
    //av_dict_set(&opts, "probesize", "2048", 0);
    //av_dict_set(&opts, "max_analyze_duration", "10", 0);


    while(flag <= 5)
    {
        pInFormatCtx = avformat_alloc_context();
        pInFormatCtx->interrupt_callback.callback = interrupt_callback;
        pInFormatCtx->interrupt_callback.opaque = &input_runner;
        input_runner.lasttime = time(NULL);

        flag++;
        qDebug() << flag;
        //QLOG_INFO() << "Try to open video input: " << flag;

        //ret = avformat_open_input(&pInFormatCtx, url.toLatin1(), NULL, &opts);
        ret = avformat_open_input(&pInFormatCtx, url.toLatin1(), NULL, NULL);
        if(ret >= 0)
        {
            //QLOG_INFO() << "Open audio success.";
            break;
        }
    }
    if (ret < 0)
    {
        qDebug() << "Could not open input file.";
        //QLOG_INFO() << "Could not open input file.";
        goto error;
    }

    //获取视频流信息
    ret = avformat_find_stream_info(pInFormatCtx, NULL);
    if (ret < 0)
    {
        qDebug() << "Failed to retrieve input stream information.";
        //QLOG_INFO() << "Failed to retrieve input stream information.";
        goto error;
    }

    //获取视频流索引
    for (int i = 0; i < pInFormatCtx->nb_streams; i++)
    {
        if (pInFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoIndex = i;
            break;
        }
    }
    if (videoIndex == -1)
    {
        qDebug() << "Video stream not found.";
        //QLOG_INFO() << "Video stream not found.";
        ret = -1;
        goto error;
    }
    //为视频流创建对应的解码器
    pCodec = (AVCodec*)avcodec_find_decoder(pInFormatCtx->streams[videoIndex]->codecpar->codec_id);
    if (pCodec == NULL)
    {
        qDebug() << "Codec not found.";
        //QLOG_INFO() << "Codec not found.";
        ret = -1;
        goto error;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx)
    {
        qDebug() << "can't alloc codec context";
        //QLOG_INFO() << "can't alloc codec context";
        ret = -1;
        goto error;
    }

    //复制解码器信息
    avcodec_parameters_to_context(pCodecCtx, pInFormatCtx->streams[videoIndex]->codecpar);
    if (pCodecCtx == NULL)
    {
        qDebug() << "Cannot alloc context.";
        //QLOG_INFO() << "Cannot alloc context.";
        goto error;
    }

    //打开解码器
    ret = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (ret < 0)
    {
        qDebug() << "Could not open codec.";
        //QLOG_INFO() << "Could not open codec.";
        goto error;
    }

    return ret;

error:
    if (pInFormatCtx)
    {
        avformat_close_input(&pInFormatCtx);
    }
    avcodec_close(pCodecCtx);
    return ret;
}

int VideoPlay::SDLPlayerInit()
{
    SDL_Init(SDL_INIT_VIDEO);

    screen = SDL_CreateWindowFrom(winid);

    if (!screen)
    {
        qDebug() << "SDL: could not create window - exiting:" <<  SDL_GetError();
        //QLOG_INFO() << "SDL: could not create window - exiting:" <<  SDL_GetError();
        return -1;
    }
    sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = width;
    sdlRect.h = height;


    return 0;
}

int VideoPlay::SDLPlayerStart()
{
    int ret = 0;

    SwsContext* pImgConvertCtx = NULL;
    AVPacket* packet = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pFrameYUV = av_frame_alloc();

    pFrameYUV->width = pCodecCtx->width;
    pFrameYUV->height = pCodecCtx->height;
    pFrameYUV->format = AV_PIX_FMT_YUV420P;


    pImgConvertCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
            width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    while (inRunning)
    {
        //Get an AVPacket
        input_runner.lasttime = time(NULL);
        ret = av_read_frame(pInFormatCtx, packet);
        if (ret < 0)
        {
            qDebug() << "Get an video AVPacket error.";
            goto end;
        }

        if (packet->stream_index == videoIndex)
        {
            if (avcodec_send_packet(pCodecCtx, packet) == 0)
            {
                while (avcodec_receive_frame(pCodecCtx, pFrame) == 0)
                {
                    sws_scale(pImgConvertCtx, (const unsigned char* const*)pFrame->data,
                        pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

                    SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
                        pFrameYUV->data[0], pFrameYUV->linesize[0],
                        pFrameYUV->data[1], pFrameYUV->linesize[1],
                        pFrameYUV->data[2], pFrameYUV->linesize[2]);

                    SDL_RenderClear(sdlRenderer);
                    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
                    SDL_RenderPresent(sdlRenderer);

                    SDL_Delay(10);
                }
            }
        }
        av_packet_unref(packet);
    }

end:
    av_packet_free(&packet);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameYUV);

    if (pCodecCtx)
    {
        avcodec_close(pCodecCtx);
    }

    SDL_CloseAudio(); //Close SDL
    SDL_Quit();

    avformat_close_input(&pInFormatCtx);

    if (ret < 0 && ret != AVERROR_EOF)
    {
        printf("Error occurred.\n");
        return ret;
    }
    return ret;
}

void VideoPlay::run()
{
    SDLPlayerStart();
}

int VideoPlay::interrupt_callback(void *p)
{
    RunnerV *r = (RunnerV *)p;

    if(r->lasttime > 0)
    {
        if(time(NULL) - r->lasttime > 3 && !input_runner.connected)
        {
            return 1;
        }
    }
    return 0;
}
