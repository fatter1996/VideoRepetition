#ifndef TEXTANALYSIS_H
#define TEXTANALYSIS_H
#pragma execution_character_set("utf-8")
#include <QByteArray>
#include <QObject>
#include <QDateTime>

struct TextFrame
{
    unsigned char captureState = 0; //录制状态,0xA5:开始录制，0X5A：结束录制，0xAA录制中状态，0x00其他状态
    unsigned char textDisplayState= 0; //文字显示状态,0xB5:显示文字，0X5B：不显示文字，0x00其他
    QDateTime time; //时间;
    QPoint point = { 0,0 }; //坐标
    int duration = 0; //时长
    int textLenght = 0; //文字长度
    unsigned int textColor = 0; //文字颜色
    QByteArray text; //显示内容
};

struct StreamFrame
{
    int urlLenght = 0; //推流地址长度
    QByteArray url; //推流地址
    int nameLenght = 0; //车站名称长度
    QByteArray name; //车站名称
};

struct StationFrame
{
    int nameLenght = 0; //车站名称长度
    QByteArray name; //车站名称
    int loginNameLenght = 0; //操作人员名称长度
    QByteArray loginName; //操作人员名称
};

class TextAnalysis
{
public:
    TextAnalysis();
    ~TextAnalysis();

    TextFrame* TextFrameUnpack(QByteArray dataArr);
    StreamFrame* StreamFrameUnpack(QByteArray dataArr);
    StationFrame* StationFrameUnpack(QByteArray dataArr);
};

#endif // TEXTANALYSIS_H
