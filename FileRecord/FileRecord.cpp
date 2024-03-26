#include "FileRecord.h"
#include <QDebug>
#include <QSettings>
#include <QDateTime>
#include <QtConcurrent>


Runner FileRecord::input_runner = {0};

FileRecord::FileRecord(QObject *parent)
{
    Q_UNUSED (parent)
    avdevice_register_all(); //注册设备
    avformat_network_init(); //加载socket库以及网络加密协议相关的库
}

FileRecord::~FileRecord()
{
    /* close input */
    if (pVideoInFormatCtx)
        avformat_close_input(&pVideoInFormatCtx);
    if (pAudioInFormatCtx)
        avformat_close_input(&pAudioInFormatCtx);

    /* close codec context */
    if(pVideoCodecCtx)
        avcodec_close(pVideoCodecCtx);
    if(pAudioCodecCtx)
        avcodec_close(pAudioCodecCtx);
    if(pH264CodecCtx)
        avcodec_close(pH264CodecCtx);

    /* close output */
    if (pOutFormatCtx)
    {
        avio_close(pOutFormatCtx->pb);
        avformat_free_context(pOutFormatCtx);
    }
}

int FileRecord::Init()
{
    int ret = 0;
    ret = VideoCaptureInit();
    if(ret >= 0)
    {
        ret = H264RecodeInit();
        if(ret < 0) return ret;
    }
    else return ret;
    ret = AudioCaptureInit();
    if(ret >= 0)
    {
        ret = AacRecodeInit();
        if(ret < 0) return ret;
        ret = AudioFilterInit();
        if(ret < 0) return ret;
    }
    else return OutPutInit();

    return OutPutInit();
}

int FileRecord::VideoCaptureInit()
{
    int ret = 0;
    //参数设置
    AVDictionary* options = NULL;
    av_dict_set_int(&options, "framerate", GlobalData::m_pConfigInfo->frameRate, AV_DICT_MATCH_CASE); //帧率
    av_dict_set_int(&options, "offset_x", GlobalData::m_pConfigInfo->desktopRange_x, AV_DICT_MATCH_CASE); //录制区域的起始坐标-x
    av_dict_set_int(&options, "offset_y", GlobalData::m_pConfigInfo->desktopRange_y, AV_DICT_MATCH_CASE); //录制区域的起始坐标-y
    av_dict_set(&options, "video_size", QString("%1x%2").arg(GlobalData::m_pConfigInfo->desktopWidth).arg(GlobalData::m_pConfigInfo->desktopHeight).toLatin1(), AV_DICT_MATCH_CASE); //录制区域大小
    av_dict_set_int(&options, "draw_mouse", 1, AV_DICT_MATCH_CASE); //是否绘制鼠标

    //获取gdigrab
    AVInputFormat* ifmt = av_find_input_format("gdigrab");

    //打开gidgrab,传入参数,获取封装格式相关信息
    flag = 0;
    while(isRunning)
    {
        //创建AVformatContext句柄
        pVideoInFormatCtx = avformat_alloc_context();
        pVideoInFormatCtx->interrupt_callback.callback = interrupt_callback;
        pVideoInFormatCtx->interrupt_callback.opaque = &input_runner;
        input_runner.lasttime = time(NULL);

        ret = avformat_open_input(&pVideoInFormatCtx, "desktop", ifmt, &options);
        flag++;
        qDebug() << flag;
        if(ret >= 0)
        {
            break;
        }
    }
    if(ret < 0)
    {
        qDebug() << "Couldn't open input stream.";
        return ret;
    }

    //获取流信息
    ret = avformat_find_stream_info(pVideoInFormatCtx, NULL);
    if(ret < 0 )
    {
        qDebug() << "Couldn't find stream information.";
        return ret;
    }

    //获取流索引
    for(uint i = 0; i < pVideoInFormatCtx->nb_streams; i++)
    {
        if(pVideoInFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoIndex = i;
            break;
        }
    }
    if(videoIndex == -1)
    {
        qDebug() << "Didn't find a video stream.";
        return -1;
    }
    //av_dump_format(pInFormatCtx, 0, "desktop", 0);

    //为gidgrab创建对应的解码器
    pVideoCodec = avcodec_find_decoder(pVideoInFormatCtx->streams[videoIndex]->codecpar->codec_id);
    if(pVideoCodec == NULL)
    {
        qDebug() << "Codec not found.";
        return -1;
    }

    pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);
    if (!pVideoCodecCtx)
    {
        qDebug() << "can't alloc codec context";
        return -1;
    }

    //复制解码器信息
    ret = avcodec_parameters_to_context(pVideoCodecCtx, pVideoInFormatCtx->streams[videoIndex]->codecpar);
    if (ret < 0)
    {
        qDebug() << "Failed to copy context from input to output stream codec context.";
        return ret;
    }
    pVideoCodecCtx->codec_tag = 0;

    //打开解码器
    ret = avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL);
    if(ret < 0)
    {
        qDebug() << "Could not open codec.";
        return ret;
    }

    av_dict_free(&options);

    return ret;
}

int FileRecord::AudioCaptureInit()
{
    int ret = 0;
    //打开收入流,传入参数,获取封装格式相关信息
    //AVInputFormat* ifmt = av_find_input_format("dshow");
    //QString url = QString("audio=virtual-audio-capturer");
    AVDictionary* opts = nullptr;
    av_dict_set_int(&opts, "audio_buffer_size", 30, 0);

    flag = 0;
    while(flag <= 3)
    {
        //pAudioInFormatCtx = avformat_alloc_context();
        //pAudioInFormatCtx->interrupt_callback.callback = interrupt_callback;
        //pAudioInFormatCtx->interrupt_callback.opaque = &input_runner;
        //input_runner.lasttime = time(NULL);

        //if(avformat_open_input(&pAudioInFormatCtx, GlobalData::m_pConfigInfo->url.toLatin1(), NULL, NULL) < 0)
        //{
        //    qDebug() << "Could not open input file.";
        //}
        ret = avformat_open_input(&pAudioInFormatCtx, GlobalData::m_pConfigInfo->url.toLatin1(), NULL, NULL);
        //ret = avformat_open_input(&pAudioInFormatCtx, url.toLatin1(), ifmt, &opts);
        flag++;
        qDebug() << flag;
        if(flag >= 5 && ret < 0)
        {
            isAudioInit = false;
            return -1;
        }
        if(ret >= 0)
        {
            break;
        }
    }
    //ret = avformat_open_input(&pAudioInFormatCtx, GlobalData::m_pConfigInfo->url.toLatin1(), NULL, &opts);
    if (ret < 0)
    {
        qDebug() << "Could not open input file.";
        return ret;
    }

    //获取音频流信息
    ret = avformat_find_stream_info(pAudioInFormatCtx, NULL);
    if (ret < 0)
    {
        qDebug() << "Failed to retrieve input stream information.";
        return ret;
    }

    //获取音频流索引
    for (uint i = 0; i < pAudioInFormatCtx->nb_streams; i++)
    {
        if (pAudioInFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioIndex = i;
            break;
        }
    }
    if (audioIndex == -1)
    {
        qDebug() << "Audio stream not found.";
        return -1;
    }
    //为视频流创建对应的解码器
    pAudioCodec = (AVCodec*)avcodec_find_decoder(pAudioInFormatCtx->streams[audioIndex]->codecpar->codec_id);
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
    avcodec_parameters_to_context(pAudioCodecCtx, pAudioInFormatCtx->streams[audioIndex]->codecpar);
    if (pAudioCodecCtx == NULL)
    {
        qDebug() << "Cannot alloc context.";
        return -1;
    }

    pAudioCodecCtx->channel_layout = av_get_default_channel_layout(pAudioCodecCtx->channels);
    pAudioCodecCtx->codec_tag = 0;


    //打开解码器
    ret = avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL);
    if (ret < 0)
    {
        qDebug() << "Could not open codec.";
        return ret;
    }

    return ret;
}

int FileRecord::H264RecodeInit()
{
    int ret = 0;
    AVDictionary* options = NULL;
    //查找H264编码器
    pH264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!pH264Codec)
    {
        qDebug() << "Could not find h264 codec.";
        return -1;
    }

    // 设置参数
    pH264CodecCtx = avcodec_alloc_context3(pH264Codec);
    pH264CodecCtx->codec_id = AV_CODEC_ID_H264;
    pH264CodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pH264CodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pH264CodecCtx->width = GlobalData::m_pConfigInfo->desktopWidth;
    pH264CodecCtx->height = GlobalData::m_pConfigInfo->desktopHeight;
    pH264CodecCtx->time_base.num = 1;
    pH264CodecCtx->time_base.den = GlobalData::m_pConfigInfo->frameRate;	//帧率（即一秒钟多少张图片）
    pH264CodecCtx->bit_rate = GlobalData::m_pConfigInfo->videoBitRate;	//比特率（调节这个大小可以改变编码后视频的质量）
    pH264CodecCtx->rc_max_rate = GlobalData::m_pConfigInfo->videoBitRate;
    pH264CodecCtx->rc_min_rate = GlobalData::m_pConfigInfo->videoBitRate;
    pH264CodecCtx->gop_size = 2;
    pH264CodecCtx->qmin = 10;
    pH264CodecCtx->qmax = 51;
    pH264CodecCtx->max_b_frames = 0;
    pH264CodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER | AV_CODEC_FLAG_LOW_DELAY;

    //打开H.264编码器
    av_dict_set(&options, "preset", "superfast", 0);
    av_dict_set(&options, "tune", "zerolatency", 0);//实现实时编码
    av_dict_set(&options, "rtbufsize", "1000k", 0);
    av_dict_set(&options, "start_time_realtime", 0, 0);
    av_dict_set(&options, "crf", "15", 0);
    av_dict_set(&options, "level", "3",0);

    ret = avcodec_open2(pH264CodecCtx, pH264Codec, &options);
    if ( ret< 0)
    {
        qDebug() << "Could not open video encoder.";
        return ret;
    }

    return ret;
}

int FileRecord::AacRecodeInit()
{
    int ret = 0;
    AVDictionary* options = NULL;
    //查找音频编码器
    pAacCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!pAacCodec)
    {
        qDebug() << "Could not find h264 codec.";
        return -1;
    }

    //设置参数
    pAacCodecCtx = avcodec_alloc_context3(pAacCodec);
    pAacCodecCtx->codec = pAacCodec;
    pAacCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    pAacCodecCtx->sample_rate= GlobalData::m_pConfigInfo->sampleRate; //音频采样率
    pAacCodecCtx->channels = 2;
    pAacCodecCtx->channel_layout = av_get_default_channel_layout(pAacCodecCtx->channels);
    pAacCodecCtx->bit_rate = GlobalData::m_pConfigInfo->audioBitRate;
    pAacCodecCtx->codec_tag = 0;
    pAacCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    av_dict_set(&options, "preset", "superfast", 0);
    av_dict_set(&options, "tune", "zerolatency", 0);//实现实时编码
    av_dict_set(&options, "rtbufsize", "200k", 0);
    av_dict_set(&options, "start_time_realtime", 0, 0);

    //打开音频编码器
    ret = avcodec_open2(pAacCodecCtx, pAacCodec, &options);
    if ( ret< 0)
    {
        qDebug() << "Could not open audio encoder.";
        return ret;
    }

    return ret;
}

int FileRecord::AudioFilterInit()
{
    char args[512];
    int ret;
    AVFilter* abuffersrc = (AVFilter*)avfilter_get_by_name("abuffer");
    AVFilter* abuffersink = (AVFilter*)avfilter_get_by_name("abuffersink");
    AVFilterInOut* outputs = avfilter_inout_alloc();
    AVFilterInOut* inputs = avfilter_inout_alloc();

    AVFilterGraph* filterGraph = avfilter_graph_alloc();
    filterGraph->nb_threads = 1;

    static const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_NONE };
    static const int64_t out_channel_layouts[] = { (uint8_t)pAacCodecCtx->channel_layout, -1 };
    static const int out_sample_rates[] = { pAacCodecCtx->sample_rate , -1 };

    AVRational time_base = pAudioInFormatCtx->streams[audioIndex]->time_base;
    sprintf_s(args, sizeof(args),
        "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
        time_base.num, time_base.den, pAudioCodecCtx->sample_rate,
        av_get_sample_fmt_name(pAudioCodecCtx->sample_fmt), av_get_default_channel_layout(pAudioCodecCtx->channels));

    ret = avfilter_graph_create_filter(&pAudioBuffersrcCtx, abuffersrc, "in", args, NULL, filterGraph);
    if (ret < 0)
    {
        qDebug() << "Cannot create audio buffer source.";
        return ret;
    }

    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&pAudioBuffersinkCtx, abuffersink, "out", NULL, NULL, filterGraph);
    if (ret < 0)
    {
        qDebug() << "Cannot create audio buffer sink.";
        return ret;
    }

    ret = av_opt_set_int_list(pAudioBuffersinkCtx, "sample_fmts", out_sample_fmts, -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        qDebug() << "Cannot set output sample format.";
        return ret;
    }

    ret = av_opt_set_int_list(pAudioBuffersinkCtx, "channel_layouts", out_channel_layouts, -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        qDebug() << "Cannot set output channel layout.";
        return ret;
    }

    ret = av_opt_set_int_list(pAudioBuffersinkCtx, "sample_rates", out_sample_rates, -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        qDebug() << "Cannot set output sample rate.";
        return ret;
    }

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = pAudioBuffersrcCtx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = pAudioBuffersinkCtx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    ret = avfilter_graph_parse_ptr(filterGraph, "anull", &inputs, &outputs, nullptr);
    if (ret < 0)
    {
        qDebug() << "Add audio graph described failed.";
        return ret;
    }

    ret = avfilter_graph_config(filterGraph, NULL);
    if (ret < 0)
    {
        qDebug() << "Audio filter init failed.";
        return ret;
    }

    //av_buffersink_set_frame_size(pAudioBuffersinkCtx, 1024);
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return 0;
}

int FileRecord::OutPutInit()
{
    int ret = 0;
    AVStream* outVideoStream = NULL;
    AVStream* outAudioStream = NULL;
    AVDictionary * options = NULL;
    av_dict_set(&options, "flvflags", "no_duration_filesize", 0);
    //分配输出ctx

    fileName = QString("%1.flv").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh_mm"));
    avformat_alloc_output_context2(&pOutFormatCtx, NULL, "flv", fileName.toLatin1());
    if (!pOutFormatCtx)
    {
        qDebug() << "Could not create output context";
        return -1;
    }

    //创建输出视频流
    outVideoStream = avformat_new_stream(pOutFormatCtx, pH264Codec);
    if (!outVideoStream)
    {
        qDebug() << "Failed allocating output stream";
        return -1;
    }
    avcodec_parameters_from_context(outVideoStream->codecpar, pH264CodecCtx);
    outVideoStream->codecpar->codec_tag = 0;
    outVideoIndex = outVideoStream->index;

    //创建输出音频流
    if(isAudioInit)
    {
        outAudioStream = avformat_new_stream(pOutFormatCtx, pAacCodec);
        if (!outAudioStream)
        {
            qDebug() << "Failed allocating output stream";
            return -1;
        }
    //    avcodec_copy_context(outAudioStream->codec, pAacCodecCtx);
        avcodec_parameters_from_context(outAudioStream->codecpar, pAacCodecCtx);
        outAudioStream->codecpar->codec_tag = 0;
        outAudioIndex = outAudioStream->index;
    }


    //打开输出url
    if (!(pOutFormatCtx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&pOutFormatCtx->pb, fileName.toLatin1(), AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            qDebug() << "Failed to open output url: " << fileName << ". Error Code : " << ret;
            return ret;
        }
    }

    //写文件头
    ret = avformat_write_header(pOutFormatCtx, options ? &options : NULL);
    if (ret < 0)
    {
        qDebug() << "Failed to write header. Error Code : " << ret;
        return ret;
    }
    return ret;
}

int FileRecord::VideoCaptureStart()
{
    int ret = 0;
    //桌面视频流Frame初始化
    SwsContext* pSwsCtx = NULL;
    AVPacket* pPacket = av_packet_alloc();
    AVPacket* pOutPacket = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pFrameYUV = av_frame_alloc();

    unsigned char* DesktopBuffer = (unsigned char*)av_malloc(av_image_get_buffer_size(
                              AV_PIX_FMT_YUV420P, pVideoCodecCtx->width, pVideoCodecCtx->height, 1));

    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, DesktopBuffer,
            AV_PIX_FMT_YUV420P, pVideoCodecCtx->width, pVideoCodecCtx->height, 1);

    pSwsCtx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height,
            pVideoCodecCtx->pix_fmt, pVideoCodecCtx->width, pVideoCodecCtx->height,
            AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    //申请60帧缓存
    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pVideoCodecCtx->width, pVideoCodecCtx->height, 1);
    pVideoFifo = av_fifo_alloc(10 * size);

    while (isRunning)
    {
        if(!pVideoInFormatCtx)
            break;

        input_runner.lasttime = time(NULL);
        ret = av_read_frame(pVideoInFormatCtx, pPacket);
        if (ret < 0)
        {
            qDebug() << "Read desktop frame failed.";
            break;
        }

        if (pPacket->stream_index == videoIndex)
        {
            ret = avcodec_send_packet(pVideoCodecCtx, pPacket);
            if (ret < 0)
            {
                qDebug() << "Decode packet failed.";
                break;
            }

            ret = avcodec_receive_frame(pVideoCodecCtx, pFrame);
            if (ret >= 0)
            {
                pFrameYUV->format = AV_PIX_FMT_YUV420P;
                pFrameYUV->width = pVideoCodecCtx->width;
                pFrameYUV->height = pVideoCodecCtx->height;
                av_frame_get_buffer(pFrameYUV, 1);


                sws_scale(pSwsCtx,(const unsigned char* const*)pFrame->data, pFrame->linesize,
                          0, pVideoCodecCtx->height, pFrameYUV->data,pFrameYUV->linesize);



                if (av_fifo_space(pVideoFifo) > size)
                {
                    av_fifo_generic_write(pVideoFifo, pFrameYUV->data[0], pFrameYUV->linesize[0] * pH264CodecCtx->height, NULL);
                    av_fifo_generic_write(pVideoFifo, pFrameYUV->data[1], pFrameYUV->linesize[1] * pH264CodecCtx->height / 2, NULL);
                    av_fifo_generic_write(pVideoFifo, pFrameYUV->data[2], pFrameYUV->linesize[2] * pH264CodecCtx->height / 2, NULL);
                    //qDebug() << "fifo video frame. size = " << av_fifo_size(pVideoFifo);
                }
                av_frame_unref(pFrame);
                av_frame_unref(pFrameYUV);
            }
        }
        av_packet_unref(pPacket);
        av_packet_unref(pOutPacket);
    }

    av_packet_free(&pPacket);
    av_packet_free(&pOutPacket);

    av_frame_free(&pFrame);
    av_frame_free(&pFrameYUV);

    return ret;
}

int FileRecord::AudioCaptureStart()
{
    int ret = 0;

    AVPacket* pPacket = av_packet_alloc();
    AVFrame* pFrame = av_frame_alloc();
    AVFrame* pOutFrame = av_frame_alloc();
    pAudioFifo = av_audio_fifo_alloc(pAacCodecCtx->sample_fmt,
        pAacCodecCtx->channels, 30 * 1024);

    while (isRunning)
    {
        input_runner.lasttime = time(NULL);
        //从输入流读取一个packet
        ret = av_read_frame(pAudioInFormatCtx, pPacket);
        if (ret < 0)
        {
            qDebug() << "Read audio frame failed.";
            break;
        }
        //qDebug() << pPacket->stream_index;
        if (pPacket->stream_index == audioIndex)
        {

            ret = avcodec_send_packet(pAudioCodecCtx, pPacket);
            if (ret < 0)
            {
                qDebug() << "Decode audio frame failed.";
                break;
            }

            if(avcodec_receive_frame(pAudioCodecCtx, pFrame) >= 0)
            {
                ret = av_buffersrc_add_frame_flags(pAudioBuffersrcCtx, pFrame, AV_BUFFERSRC_FLAG_PUSH);
                if (ret < 0)
                {
                    qDebug() << "Error while feeding the filtergraph buffersrc_add_frame.";
                    break;
                }

                ret = av_buffersink_get_frame_flags(pAudioBuffersinkCtx, pOutFrame, AV_BUFFERSINK_FLAG_NO_REQUEST);
                if (ret < 0)
                {
                    qDebug() << "Error while feeding the filtergraph.";
                    break;
                }

                if (av_audio_fifo_space(pAudioFifo) >= pOutFrame->nb_samples)
                {
                    av_audio_fifo_write(pAudioFifo, (void **)pOutFrame->data, pOutFrame->nb_samples);
                    //qDebug() << "fifo audio frame. size = " << av_audio_fifo_size(pAudioFifo);
                }
                qDebug() << "Audio frame fifo space = " << av_audio_fifo_space(pAudioFifo);
            }
        }
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
        av_frame_unref(pOutFrame);
    }

    av_frame_free(&pFrame);
    av_frame_free(&pOutFrame);
    av_packet_free(&pPacket);

    return ret;
}

int FileRecord::RecordStart()
{
    int ret = 0;
    int64_t cur_pts_v=0;
    int64_t cur_pts_a=0;
    int VideoFrameIndex = 0;
    int AudioFrameIndex = 0;
    int ptsInterval = 0;

    AVFrame* pOutFrame = av_frame_alloc();
    AVPacket* pOutPkt = av_packet_alloc();

    //缓冲数据初始化
    uint8_t* frame_buf = NULL;
    int size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, GlobalData::m_pConfigInfo->desktopWidth, GlobalData::m_pConfigInfo->desktopHeight, 1);
    frame_buf = new uint8_t[size];
    memset(frame_buf, 0, size);
    av_image_fill_arrays(pOutFrame->data, pOutFrame->linesize, frame_buf, AV_PIX_FMT_YUV420P,
           GlobalData::m_pConfigInfo->desktopWidth, GlobalData::m_pConfigInfo->desktopHeight, 1);

    startTime = QDateTime::currentDateTime();
    QDateTime currTime;
    while (isRunning)
    {
        currTime = QDateTime::currentDateTime();
        if(currTime.toTime_t() - startTime.toTime_t() > GlobalData::m_pConfigInfo->maxTime)
        {
            emit RecordEnd();
            break;
        }

        if(isAudioInit)
        {
            //比较pst,判断帧种类
            ptsInterval = av_compare_ts(cur_pts_v, pOutFormatCtx->streams[outVideoIndex]->time_base,
                                cur_pts_a,pOutFormatCtx->streams[outAudioIndex]->time_base);
        }

        if(ptsInterval <= 0 || !isAudioInit) //视频
        {
            if (!pVideoFifo)
                continue;
            if(av_fifo_size(pVideoFifo) <= 0)
                continue;
            memset(frame_buf, 0, size);
            av_fifo_generic_read(pVideoFifo, frame_buf, size, NULL);
            av_image_fill_arrays(pOutFrame->data, pOutFrame->linesize, frame_buf, AV_PIX_FMT_YUV420P,
                   GlobalData::m_pConfigInfo->desktopWidth, GlobalData::m_pConfigInfo->desktopHeight, 1);

            ret = avcodec_send_frame(pH264CodecCtx, pOutFrame);
            if (ret < 0)
            {
                qDebug() << "Failed to encode frame.";
                break;
            }

            if(avcodec_receive_packet(pH264CodecCtx, pOutPkt) >= 0)
            {
                pOutPkt->stream_index = outVideoIndex;

                pOutPkt->pts = VideoFrameIndex * pOutFormatCtx->streams[outVideoIndex]->time_base.den / GlobalData::m_pConfigInfo->frameRate;
                pOutPkt->dts = pOutPkt->pts;
                cur_pts_v = pOutPkt->pts;

                pOutPkt->duration = ((pOutFormatCtx->streams[0]->time_base.den / pOutFormatCtx->streams[videoIndex]->time_base.num) / GlobalData::m_pConfigInfo->videoBitRate);

                ret = av_interleaved_write_frame(pOutFormatCtx, pOutPkt);
                if (ret < 0)
                {
                    qDebug() << "Failed to write video frame to url";
                    break;
                }
                //else qDebug() << "Send" << VideoFrameIndex << " video packet to url successfully. pts = " << cur_pts_v;
                VideoFrameIndex++;
                av_packet_unref(pOutPkt);
            }
        }
        else //音频
        {
            if (!isAudioInit)
                continue;

            if (!pAudioFifo)
                continue;

            if(av_audio_fifo_size(pAudioFifo) <= 0)
                continue;

            pOutFrame->nb_samples = pAudioCodecCtx->frame_size;
            pOutFrame->channel_layout = pAudioCodecCtx->channel_layout;
            pOutFrame->format = pAudioCodecCtx->sample_fmt;
            pOutFrame->sample_rate = pAudioCodecCtx->sample_rate;
            av_frame_get_buffer(pOutFrame, 0);

            av_audio_fifo_read(pAudioFifo, (void **)pOutFrame->data, pOutFrame->nb_samples);
            qDebug() << "Audio frame fifo space before = " << av_audio_fifo_space(pAudioFifo);
            ret = avcodec_send_frame(pAacCodecCtx, pOutFrame);
            if (ret < 0)
            {
                qDebug() << "Failed to encode frame.";
                //goto end;
                continue;
            }

            if(avcodec_receive_packet(pAacCodecCtx, pOutPkt) >= 0)
            {
                pOutPkt->stream_index = outAudioIndex;

                uint64_t streamTimeBase = pOutFormatCtx->streams[outAudioIndex]->time_base.den;
                pOutPkt->pts = (uint64_t)(1024 * streamTimeBase * AudioFrameIndex) / pAacCodecCtx->sample_rate;
                pOutPkt->dts = pOutPkt->pts;
                cur_pts_a = pOutPkt->pts;
                pOutPkt->duration = pAacCodecCtx->frame_size;


                ret = av_interleaved_write_frame(pOutFormatCtx, pOutPkt);
                if (ret < 0)
                {
                    qDebug() << "Failed to write audio frame to url";
                    break;
                }
                //else qDebug() << "Send" << AudioFrameIndex << " audio packet to url successfully. pts = " << cur_pts_a;
                AudioFrameIndex++;
                av_packet_unref(pOutPkt);
            }
        }
        av_frame_unref(pOutFrame);
    }
    //写文件尾
    av_write_trailer(pOutFormatCtx);

    av_frame_free(&pOutFrame);
    av_packet_free(&pOutPkt);

    return ret;
}

QString FileRecord::RecordStop()
{
    isRunning = false;
    this->wait(500);
    return fileName;
}

void FileRecord::run()
{
    QtConcurrent::run(this, &FileRecord::VideoCaptureStart);
    if(isAudioInit)
    {
        QtConcurrent::run(this, &FileRecord::AudioCaptureStart);
    }

    RecordStart();
}

int FileRecord::interrupt_callback(void *p)
{
    Runner *r = (Runner *)p;

    if(r->lasttime > 0)
    {
        if(time(NULL) - r->lasttime > 1 && !input_runner.connected)
        {
            return 1;
        }
    }
    return 0;
}
