#include "TextAnalysis.h"
#include "Universal.h"

TextAnalysis::TextAnalysis()
{

}

TextAnalysis::~TextAnalysis()
{

}


TextFrame* TextAnalysis::TextFrameUnpack(QByteArray dataArr)
{
    TextFrame* data = new TextFrame;
    int flag = 9;

    data->captureState = dataArr[flag++];
    data->textDisplayState = dataArr[flag++];
    data->time = GlobalData::getDateTimeByArray(dataArr.mid(flag, 7));
    flag += 7;
    data->point.setX(GlobalData::getBigEndian_2Bit(dataArr.mid(flag, 2)));
    flag += 2;
    data->point.setY(GlobalData::getBigEndian_2Bit(dataArr.mid(flag, 2)));
    flag += 2;
    data->duration = dataArr[flag++];
    data->textLenght = dataArr[flag++];
    data->textColor = dataArr[flag++];
    data->text = dataArr.mid(flag, data->textLenght);
    flag += data->textLenght;

    return data;
}

StreamFrame* TextAnalysis::StreamFrameUnpack(QByteArray dataArr)
{
    StreamFrame* data = new StreamFrame;
    int flag = 10;

    data->urlLenght = GlobalData::getBigEndian_2Bit(dataArr.mid(flag, 2));
    flag += 2;
    data->url = dataArr.mid(flag, data->urlLenght);
    flag += data->urlLenght;

    data->nameLenght = GlobalData::getBigEndian_2Bit(dataArr.mid(flag, 2));
    flag += 2;
    data->name = dataArr.mid(flag, data->nameLenght);
    flag += data->nameLenght;

    return data;
}

StationFrame* TextAnalysis::StationFrameUnpack(QByteArray dataArr)
{
    StationFrame* data = new StationFrame;
    int flag = 10;

    data->nameLenght = GlobalData::getBigEndian_2Bit(dataArr.mid(flag, 2));
    flag += 2;
    data->name = dataArr.mid(flag, data->nameLenght);
    flag += data->nameLenght;

    data->loginNameLenght = GlobalData::getBigEndian_2Bit(dataArr.mid(flag, 2));
    flag += 2;
    data->loginName = dataArr.mid(flag, data->loginNameLenght);
    flag += data->loginNameLenght;

    return data;
}
