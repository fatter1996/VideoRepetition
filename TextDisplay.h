#ifndef TEXTDISPLAY_H
#define TEXTDISPLAY_H
#pragma execution_character_set("utf-8")
#include <QWidget>
#include "TextAnalysis/TextAnalysis.h"
namespace Ui {
class TextDisplay;
}

class TextDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit TextDisplay(QWidget *parent = nullptr);
    ~TextDisplay();
    void Display(TextFrame* data);
    void timerEvent(QTimerEvent * ev);

private:
    Ui::TextDisplay *ui;
    int timeId = 0;
};

#endif // TEXTDISPLAY_H
