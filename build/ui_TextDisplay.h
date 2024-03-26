/********************************************************************************
** Form generated from reading UI file 'TextDisplay.ui'
**
** Created by: Qt User Interface Compiler version 5.14.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TEXTDISPLAY_H
#define UI_TEXTDISPLAY_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TextDisplay
{
public:
    QHBoxLayout *horizontalLayout;
    QLabel *label;

    void setupUi(QWidget *TextDisplay)
    {
        if (TextDisplay->objectName().isEmpty())
            TextDisplay->setObjectName(QString::fromUtf8("TextDisplay"));
        TextDisplay->resize(400, 300);
        QFont font;
        font.setFamily(QString::fromUtf8("\346\226\260\345\256\213\344\275\223"));
        font.setPointSize(16);
        TextDisplay->setFont(font);
        horizontalLayout = new QHBoxLayout(TextDisplay);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(TextDisplay);
        label->setObjectName(QString::fromUtf8("label"));
        label->setStyleSheet(QString::fromUtf8("background-color: rgba(26, 167, 255, 200);\n"
"font: 16pt \"\346\226\260\345\256\213\344\275\223\";"));
        label->setAlignment(Qt::AlignCenter);

        horizontalLayout->addWidget(label);


        retranslateUi(TextDisplay);

        QMetaObject::connectSlotsByName(TextDisplay);
    } // setupUi

    void retranslateUi(QWidget *TextDisplay)
    {
        TextDisplay->setWindowTitle(QCoreApplication::translate("TextDisplay", "Form", nullptr));
        label->setText(QCoreApplication::translate("TextDisplay", "TextLabel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TextDisplay: public Ui_TextDisplay {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TEXTDISPLAY_H
