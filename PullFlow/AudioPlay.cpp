#include "AudioPlay.h"
#include <QDebug>
#include <QSettings>
#include "Log/log.h"

//一帧PCM的数据长度
unsigned int AudioPlay::audioLen = 0;
unsigned char* AudioPlay::audioChunk = NULL;
//当前读取的位置
unsigned char* AudioPlay::audioPos = NULL;

RunnerA AudioPlay::input_runner = {0};

AudioPlay::AudioPlay(QObject *parent)
{
    QSettings *config = new QSettings("Config.ini", QSettings::IniFormat);

    url = config->value("/Audio/Url").toString();

    avdevice_register_all(); //注册设备
    avformat_network_init(); //加载socket库以及网络加密协议相关的库
}

AudioPlay::~AudioPlay()
{

}

void AudioPlay::fill_audio(void * udata, Uint8 * stream, int len)
{
    SDL_memset(stream, 0, len);

    if(audioLen == 0)
        return;

    len = (len>audioLen ? audioLen : len);

    SDL_MixAudio(stream, audioPos, len, SDL_MIX_MAXVOLUME);

    audioPos += len;
    audioLen -= len;
}


int AudioPlay::PullFlowInit()
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

        ret = avformat_open_input(&pInFormatCtx, url.toLatin1(), NULL, NULL);
        flag++;
        qDebug() << flag;
        //QLOG_INFO() << "Try to open audio input: " << flag;
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

    //获取音频流信息
    ret = avformat_find_stream_info(pInFormatCtx, NULL);
    if (ret < 0)
    {
        qDebug() << "Failed to retrieve input stream information.";
        //QLOG_INFO() << "Failed to retrieve input stream information.";
        goto error;
    }

    //获取音频流索引
    for (int i = 0; i < pInFormatCtx->nb_streams; i++)
    {
        if (pInFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioIndex = i;
            break;
        }
    }
    if (audioIndex == -1)
    {
        qDebug() << "Audio stream not found.";
        //QLOG_INFO() << "Audio stream not found.";
        ret = -1;
        goto error;
    }
    //为视频流创建对应的解码器
    pCodec = (AVCodec*)avcodec_find_decoder(pInFormatCtx->streams[audioIndex]->codecpar->codec_id);
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
    avcodec_parameters_to_context(pCodecCtx, pInFormatCtx->streams[audioIndex]->codecpar);
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

int AudioPlay::SDLPlayerInit()
{
    //   SDL
    if(SDL_Init(SDL_INIT_AUDIO))
    {
        qDebug() << "Init audio subsysytem failed.";
        //QLOG_INFO() << "Init audio subsysytem failed.";
        return 0;
    }

    SDL_AudioSpec wantSpec;
    // 和SwrContext的音频重采样参数保持一致
    wantSpec.freq = pCodecCtx->sample_rate;
    wantSpec.format = AUDIO_S16SYS;
    wantSpec.channels = pCodecCtx->channels;
    wantSpec.silence = 0;
    wantSpec.samples = pCodecCtx->frame_size;
    wantSpec.callback = fill_audio;
    wantSpec.userdata = pCodecCtx;

    if(SDL_OpenAudio(&wantSpec, NULL) < 0)
    {
        qDebug() << "Can not open SDL!";
        //QLOG_INFO() << "Can not open SDL!";
        return -1;
    }

    SDL_PauseAudio(0);
    return 0;
}

int AudioPlay::SDLPlayerStart()
{
    int ret = 0;
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    // 重采样contex
    SwrContext *au_convert_ctx = swr_alloc_set_opts(NULL,
                           av_get_default_channel_layout(pCodecCtx->channels), AV_SAMPLE_FMT_S16, pCodecCtx->sample_rate,
                           pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);

    swr_init(au_convert_ctx);

    int out_buffer_size = av_samples_get_buffer_size(NULL, pCodecCtx->channels, pCodecCtx->frame_size, AV_SAMPLE_FMT_S16, 1);
    unsigned char *outBuff = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * pCodecCtx->channels);

    while(1)
    {
        if((ret = av_read_frame(pInFormatCtx, packet)) < 0)
        {
            qDebug() << "Get an audio AVPacket error.";
            goto end;
        }


        if(packet->stream_index == audioIndex)
        {
            ret = avcodec_send_packet(pCodecCtx, packet); // 送一帧到解码器

            while(ret >= 0)
            {
                ret = avcodec_receive_frame(pCodecCtx, frame);  // 从解码器取得解码后的数据
                if(ret >= 0)
                {
                    swr_convert(au_convert_ctx, &outBuff, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);

                    while(audioLen > 0)
                        SDL_Delay(1);
                    audioChunk = (unsigned char *)outBuff;
                    audioPos = audioChunk;
                    audioLen = out_buffer_size;

                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(packet);
    }
end:
    SDL_Quit();

    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pInFormatCtx);
    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&au_convert_ctx);
    return 0;
}

void AudioPlay::run()
{
    SDLPlayerStart();
}

int AudioPlay::interrupt_callback(void *p)
{
    RunnerA *r = (RunnerA *)p;

    if(r->lasttime > 0)
    {
        if(time(NULL) - r->lasttime > 3 && !input_runner.connected)
        {
            return 1;
        }
    }
    return 0;
}
