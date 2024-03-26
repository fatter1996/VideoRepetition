#include "PullFlow.h"
#include <QDebug>
#include <QSettings>
#include <QtConcurrent>

//一帧PCM的数据长度
unsigned int PullFlow::audioLen = 0;
unsigned char* PullFlow::audioChunk = NULL;
//当前读取的位置
unsigned char* PullFlow::audioPos = NULL;

Runner PullFlow::input_runner = {0};

PullFlow::PullFlow(QString rtmp, QObject *parent) : url(rtmp)
{
    Q_UNUSED (parent)
    avdevice_register_all(); //注册设备
    avformat_network_init(); //加载socket库以及网络加密协议相关的库
    connect(this, &PullFlow::Stop, this, &PullFlow::onPullFlowStop);
    timeId = startTimer(3000);
    //log = new QFile("log.txt");
}

PullFlow::~PullFlow()
{
    onPullFlowStop();

    if (pInFormatCtx)
        avformat_close_input(&pInFormatCtx);

    if(pVidoeCodecCtx)
        avcodec_close(pVidoeCodecCtx);
    if(pAudioCodecCtx)
        avcodec_close(pAudioCodecCtx);

    if(pVideoFifo)
        av_fifo_free(pVideoFifo);
    if(pAudioFifo)
        av_audio_fifo_free(pAudioFifo);
}

void PullFlow::Init(void* winid)
{
    if(PullFlowInit() < 0 && !isPullFlowInit)
    {
        if (pInFormatCtx)
            avformat_close_input(&pInFormatCtx);
        if(pVidoeCodecCtx)
            avcodec_close(pVidoeCodecCtx);
        if(pAudioCodecCtx)
            avcodec_close(pAudioCodecCtx);

        videoIndex = -1;
        audioIndex = -1;

        return Init(winid);
    }


    if(SDLPlayerInit(winid) < 0 && !isSDLPlayerInit)
    {
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyTexture(sdlTexture);
        SDL_DestroyWindow(pScreen);
        SDL_CloseAudio();
        SDL_Quit();

        return Init(winid);
    }
}

void PullFlow::PullFlowStop()
{
    isRunning = false;
    msleep(500);
    emit Stop();
}

int PullFlow::PullFlowInit()
{
    int ret = 0;
    //打开收入流,传入参数,获取封装格式相关信息
    flag = 1;
    while(isRunning)
    {
        pInFormatCtx = avformat_alloc_context();
        pInFormatCtx->interrupt_callback.callback = interrupt_callback;
        pInFormatCtx->interrupt_callback.opaque = &input_runner;
        input_runner.lasttime = time(NULL);
        qDebug() << "try to open input time " << flag;
        flag++;
        if(avformat_open_input(&pInFormatCtx, url.toLatin1(), NULL, NULL) < 0)
        {
            qDebug() << "Could not open input file.";
        }
        else break;
    }

    //获取流信息
    flag = 1;
    while(isRunning)
    {
        input_runner.lasttime = time(NULL);
        qDebug() << "try to open input time " << flag;
        flag++;
        if(avformat_find_stream_info(pInFormatCtx, NULL) < 0)
        {
            qDebug() << "Failed to retrieve input stream information.";
        }
        else break;
    }

    ret = VideoStreamInit();
    if (ret < 0)
    {
        qDebug() << "Failed to open video stream.";
        return ret;
    }

    ret = AudioStreamInit();
    if (ret < 0)
    {
        qDebug() << "Failed to open audio stream.";
        return ret;
    }
    isPullFlowInit = true;
    return ret;
}

int PullFlow::VideoStreamInit()
{
    //获取视频流索引
    for (uint i = 0; i < pInFormatCtx->nb_streams; i++)
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
        return -1;
    }

    //为视频流创建对应的解码器
    pVidoeCodec = (AVCodec*)avcodec_find_decoder(pInFormatCtx->streams[videoIndex]->codecpar->codec_id);
    if (pVidoeCodec == NULL)
    {
        qDebug() << "Codec not found.";
        return -1;
    }

    pVidoeCodecCtx = avcodec_alloc_context3(pVidoeCodec);
    if (!pVidoeCodecCtx)
    {
        qDebug() << "can't alloc codec context";
        return -1;
    }

    //复制解码器信息
    avcodec_parameters_to_context(pVidoeCodecCtx, pInFormatCtx->streams[videoIndex]->codecpar);
    if (pVidoeCodecCtx == NULL)
    {
        qDebug() << "Cannot alloc context.";
        return -1;
    }

    //打开解码器
    if (avcodec_open2(pVidoeCodecCtx, pVidoeCodec, NULL) < 0)
    {
        qDebug() << "Could not open codec.";
        return -1;
    }

    return 0;
}

int PullFlow::AudioStreamInit()
{
    //获取音频流索引
    for (uint i = 0; i < pInFormatCtx->nb_streams; i++)
    {
        if (pInFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioIndex = i;
            break;
        }
    }
    if (audioIndex == -1)
    {
        qDebug() << "audio stream not found.";
        return -1;
    }

    //为音频流创建对应的解码器
    pAudioCodec = (AVCodec*)avcodec_find_decoder(pInFormatCtx->streams[audioIndex]->codecpar->codec_id);
    if (pAudioCodec == NULL)
    {
        qDebug() << "Codec not found.";
        return -1;
    }

    pAudioCodecCtx = avcodec_alloc_context3(pAudioCodec);
    if (!pAudioCodecCtx)
    {
        qDebug() << "can't alloc codec context";
        return -1;
    }

    //复制解码器信息
    avcodec_parameters_to_context(pAudioCodecCtx, pInFormatCtx->streams[audioIndex]->codecpar);
    if (pAudioCodecCtx == NULL)
    {
        qDebug() << "Cannot alloc context.";
        return -1;
    }

    //打开解码器
    if (avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0)
    {
        qDebug() << "Could not open codec.";
        return -1;
    }

    return 0;
}

int PullFlow::SDLPlayerInit(void* winid)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    //视频播放初始化
    //播放窗口
    pScreen = SDL_CreateWindowFrom(winid);
    if (!pScreen)
    {
        qDebug() << "SDL: could not create window - exiting:" <<  SDL_GetError();
        return -1;
    }

    //渲染器初始化
    sdlRenderer = SDL_CreateRenderer(pScreen, -1, 0);
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, GlobalData::m_pConfigInfo->width, GlobalData::m_pConfigInfo->height);

    sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = GlobalData::m_pConfigInfo->width;
    sdlRect.h = GlobalData::m_pConfigInfo->height;

    //音频播放初始化
    SDL_AudioSpec wantSpec;
    wantSpec.freq = pAudioCodecCtx->sample_rate;
    wantSpec.format = AUDIO_S16SYS;
    wantSpec.channels = pAudioCodecCtx->channels;
    wantSpec.silence = 0;
    wantSpec.samples = pAudioCodecCtx->frame_size;
    wantSpec.callback = fill_audio;
    wantSpec.userdata = pAudioCodecCtx;

    if(SDL_OpenAudio(&wantSpec, NULL) < 0)
    {
        qDebug() << "Can not open SDL!";
        isAudioinit = false;
        return 0;
        //return -1;
    }

    SDL_PauseAudio(0);
    isAudioinit = true;
    isSDLPlayerInit = true;
    return 0;
}

int PullFlow::SDLPlayerStart()
{
    int ret = 0;

    AVPacket* packet = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pOutFrame = av_frame_alloc();

    //视频重采样
    SwsContext* pImgConvertCtx = sws_getContext(pVidoeCodecCtx->width, pVidoeCodecCtx->height, pVidoeCodecCtx->pix_fmt,
            GlobalData::m_pConfigInfo->width, GlobalData::m_pConfigInfo->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    //音频重采样
    SwrContext *au_convert_ctx = swr_alloc_set_opts(NULL,
                           pAudioCodecCtx->channel_layout, AV_SAMPLE_FMT_S16, pAudioCodecCtx->sample_rate,
                           pAudioCodecCtx->channel_layout, pAudioCodecCtx->sample_fmt, pAudioCodecCtx->sample_rate, 0, NULL);

    swr_init(au_convert_ctx);

    int out_buffer_size = av_samples_get_buffer_size(NULL, pAudioCodecCtx->channels, pAudioCodecCtx->frame_size, AV_SAMPLE_FMT_S16, 1);

    //申请缓存
    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, GlobalData::m_pConfigInfo->width, GlobalData::m_pConfigInfo->height, 1);
    pVideoFifo = av_fifo_alloc(30 * size);
    pAudioFifo = av_audio_fifo_alloc(pAudioCodecCtx->sample_fmt, pAudioCodecCtx->channels, 30 * out_buffer_size);

    while (isRunning)
    {
        //Get an AVPacket
        input_runner.lasttime = time(NULL);
        ret = av_read_frame(pInFormatCtx, packet);
        if (ret < 0)
        {
            qDebug() << "Read frame failed.";
            break;
        }

        if (packet->stream_index == videoIndex)
        {
            ret = avcodec_send_packet(pVidoeCodecCtx, packet);
            if (ret < 0)
            {
                qDebug() << "Decode packet failed.";
                break;
            }

            while (avcodec_receive_frame(pVidoeCodecCtx, pFrame) >= 0)
            {

                //qDebug() << "read video frame. pts = " << pFrame->pts;

                pOutFrame->width = GlobalData::m_pConfigInfo->width;
                pOutFrame->height = GlobalData::m_pConfigInfo->height;
                pOutFrame->format = AV_PIX_FMT_YUV420P;
                av_frame_get_buffer(pOutFrame, 1);

                sws_scale(pImgConvertCtx, (const unsigned char* const*)pFrame->data, pFrame->linesize,
                     0, pVidoeCodecCtx->height, pOutFrame->data, pOutFrame->linesize);

                if (av_fifo_space(pVideoFifo) > size)
                {
                    av_fifo_generic_write(pVideoFifo, pOutFrame->data[0], pOutFrame->linesize[0] * GlobalData::m_pConfigInfo->height, NULL);
                    av_fifo_generic_write(pVideoFifo, pOutFrame->data[1], pOutFrame->linesize[1] * GlobalData::m_pConfigInfo->height / 2, NULL);
                    av_fifo_generic_write(pVideoFifo, pOutFrame->data[2], pOutFrame->linesize[2] * GlobalData::m_pConfigInfo->height / 2, NULL);
                    //qDebug() << "fifo video frame. size = " << av_fifo_size(pVideoFifo);
                }
            }
            av_frame_unref(pFrame);
            av_frame_unref(pOutFrame);
        }
        else if (packet->stream_index == audioIndex)
        {
            ret = avcodec_send_packet(pAudioCodecCtx, packet);
            if (ret < 0)
            {
                qDebug() << "Decode packet failed.";
                break;
            }

            if (avcodec_receive_frame(pAudioCodecCtx, pFrame) >= 0)
            {
                pOutFrame->nb_samples = out_buffer_size;
                pOutFrame->channel_layout = pAudioCodecCtx->channel_layout;
                pOutFrame->format = pAudioCodecCtx->sample_fmt;
                pOutFrame->sample_rate = pAudioCodecCtx->sample_rate;
                av_frame_get_buffer(pOutFrame, 0);

                swr_convert(au_convert_ctx, pOutFrame->data, pOutFrame->nb_samples, (const uint8_t **)pFrame->data, pFrame->nb_samples);

                //log->open(QIODevice::WriteOnly | QIODevice::Append);
                //log->write(QString("Audio frame fifo space1 = %1\n").arg(av_audio_fifo_space(pAudioFifo)).toStdString().c_str());
                //log->close();
                if (av_audio_fifo_space(pAudioFifo) > pOutFrame->nb_samples)
                {

                    av_audio_fifo_write(pAudioFifo, (void **)pOutFrame->data, pOutFrame->nb_samples);
                    //log->open(QIODevice::WriteOnly | QIODevice::Append);
                    //log->write(QString("Audio frame fifo space2 = %1\n").arg(av_audio_fifo_space(pAudioFifo)).toStdString().c_str());
                    //log->close();
                    //qDebug() << "fifo audio frame. size = " << av_audio_fifo_size(pAudioFifo);
                }

                //log->open(QIODevice::WriteOnly | QIODevice::Append);
                //log->write(QString("Audio frame fifo space = %1\n").arg(av_audio_fifo_space(pAudioFifo)).toStdString().c_str());
                //log->close();
            }
            av_frame_unref(pFrame);
            av_frame_unref(pOutFrame);
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&pFrame);
    av_frame_free(&pOutFrame);

    emit Stop();
    if(ret < 0)
        emit Error();

    return ret;
}

void PullFlow::VideoPlay()
{
    AVFrame* pOutFrame = av_frame_alloc();
    //缓冲数据初始化
    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, GlobalData::m_pConfigInfo->width, GlobalData::m_pConfigInfo->height, 1);
    uint8_t* frame_buf = new uint8_t[size];
    memset(frame_buf, 0, size);
    av_image_fill_arrays(pOutFrame->data, pOutFrame->linesize, frame_buf, AV_PIX_FMT_YUV420P, GlobalData::m_pConfigInfo->width, GlobalData::m_pConfigInfo->height, 1);

    while (isRunning)
    {
        if (!pVideoFifo)
            continue;
        if(av_fifo_size(pVideoFifo) <= 0)
            continue;
        memset(frame_buf, 0, size);
        av_fifo_generic_read(pVideoFifo, frame_buf, size, NULL);
        av_image_fill_arrays(pOutFrame->data, pOutFrame->linesize, frame_buf, AV_PIX_FMT_YUV420P, GlobalData::m_pConfigInfo->width, GlobalData::m_pConfigInfo->height, 1);

        if(sdlTexture && sdlRenderer)
        {
            SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
                pOutFrame->data[0], pOutFrame->linesize[0],
                pOutFrame->data[1], pOutFrame->linesize[1],
                pOutFrame->data[2], pOutFrame->linesize[2]);

            SDL_RenderClear(sdlRenderer);
            SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
            SDL_RenderPresent(sdlRenderer);
            SDL_Delay(10);
        }

        av_frame_unref(pOutFrame);
    }
    av_frame_free(&pOutFrame);
    delete[] frame_buf;
}

void PullFlow::AudioPlay()
{
    AVFrame* pOutFrame = av_frame_alloc();
    isAudioinit = false;
    while (isRunning)
    {
        if (!isAudioinit)
            continue;

        if (!pAudioFifo)
            continue;   

        pOutFrame->nb_samples = av_samples_get_buffer_size(NULL, pAudioCodecCtx->channels, pAudioCodecCtx->frame_size, AV_SAMPLE_FMT_S16, 1);
        pOutFrame->channel_layout = pAudioCodecCtx->channel_layout;
        pOutFrame->format = pAudioCodecCtx->sample_fmt;
        pOutFrame->sample_rate = pAudioCodecCtx->sample_rate;

        if(av_audio_fifo_size(pAudioFifo) <= pOutFrame->nb_samples)
            continue;


        av_frame_get_buffer(pOutFrame, 0);

        av_audio_fifo_read(pAudioFifo, (void **)pOutFrame->data, pOutFrame->nb_samples);
        qDebug() << "Audio frame fifo space = " << av_audio_fifo_space(pAudioFifo);
        //log->open(QIODevice::WriteOnly | QIODevice::Append);
        //log->write(QString("Audio frame fifo space3 = %1\n").arg(av_audio_fifo_space(pAudioFifo)).toStdString().c_str());
        //log->close();
        //while(audioLen > 0)
        //  SDL_Delay(1);
        //
        //audioChunk = (unsigned char*)*pOutFrame->data;
        //audioPos = audioChunk;
        //audioLen = pOutFrame->nb_samples;
        av_frame_unref(pOutFrame);
    }
    av_frame_free(&pOutFrame);
}

void PullFlow::run()
{
    isRunning = true;
    QtConcurrent::run(this, &PullFlow::VideoPlay);
    QtConcurrent::run(this, &PullFlow::AudioPlay);
    msleep(10);
    SDLPlayerStart();
}

void PullFlow::onPullFlowStop()
{
    isRunning = false;
    killTimer(timeId);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyWindow(pScreen);
    SDL_CloseAudio();
    SDL_Quit();
}

void PullFlow::timerEvent(QTimerEvent * ev)
{
    if(ev->timerId() == timeId)
    {
        if(lockedState != GlobalData::IsSessionLocked())
        {
            isLockedStateChange = true;
            lockedState = !lockedState;
            if(lockedState)
            {
                SDL_DestroyRenderer(sdlRenderer);
                SDL_DestroyTexture(sdlTexture);
            }
            else
            {
                sdlRenderer = SDL_CreateRenderer(pScreen, -1, 0);
                sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, GlobalData::m_pConfigInfo->width, GlobalData::m_pConfigInfo->height);
            }
        }
    }
}

int PullFlow::interrupt_callback(void* p)
{
    Runner* r = (Runner*)p;
    if(r->lasttime > 0)
    {
        if(time(NULL) - r->lasttime > 3 && !input_runner.connected)
        {
            return 1;
        }
    }
    return 0;
}

void PullFlow::fill_audio(void * udata, Uint8 * stream, int len)
{
    Q_UNUSED (udata)
    SDL_memset(stream, 0, len);

    if(audioLen == 0)
        return;

    len = (len > (int)audioLen ? (int)audioLen : len);

    SDL_MixAudio(stream, audioPos, len, SDL_MIX_MAXVOLUME);

    audioPos += len;
    audioLen -= len;
}
