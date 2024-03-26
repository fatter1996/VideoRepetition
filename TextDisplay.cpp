#include "TextDisplay.h"
#include "ui_TextDisplay.h"
#include <QDebug>

TextDisplay::TextDisplay(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TextDisplay)
{

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);//去掉标题栏
    setAttribute(Qt::WA_TranslucentBackground, true);//设置窗口背景透明
    ui->setupUi(this);
}

TextDisplay::~TextDisplay()
{
    delete ui;
}

void TextDisplay::Display(TextFrame* data)
{
    QPalette pe;
    switch (data->textColor)
    {
    case 0:  pe.setColor(QPalette::WindowText, Qt::black); break;
    case 1:  pe.setColor(QPalette::WindowText, Qt::red); break;
    case 2:  pe.setColor(QPalette::WindowText, Qt::darkYellow); break;
    case 3:  pe.setColor(QPalette::WindowText, Qt::yellow); break;
    case 4:  pe.setColor(QPalette::WindowText, Qt::green); break;
    case 5:  pe.setColor(QPalette::WindowText, Qt::cyan); break;
    case 6:  pe.setColor(QPalette::WindowText, Qt::blue); break;
    case 7:  pe.setColor(QPalette::WindowText, Qt::darkBlue); break;
    default: pe.setColor(QPalette::WindowText, Qt::white); break;
    }
    ui->label->setPalette(pe);

    ui->label->setText(QString::fromLocal8Bit(data->text));

    timeId = startTimer(data->duration * 1000);
}

void TextDisplay::timerEvent(QTimerEvent * ev)
{
    if(ev->timerId() == timeId)
    {
        this->close();
    }
}
