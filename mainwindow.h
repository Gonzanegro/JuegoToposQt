#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QMessageBox>
#include <QTimer>
#include "qpaintbox.h"

#define CBWAITING 0 //indices de la combobox
#define CBOPENP 1
#define CBALIVE 2
#define CBGETLED 3
#define CBSETLED 4


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_comboBox_currentIndexChanged(int index);
    void on_pushButton_clicked();
    void onQSerialPort1Rx();
    void SetBufTX(uint8_t ID);
    void decodeData();
    void miTimerOnTime();
private:
    Ui::MainWindow *ui;

    QSerialPort *QSerialPort1;
    QString strRx;
    QTimer * miTimer;
    QPaintBox *miPaintBox;

    typedef enum{
        START,
        HEADER_1,
        HEADER_2,
        HEADER_3,
        NBYTES,
        TOKEN,
        PAYLOAD
    }_eProtocol;

    _eProtocol estado;

    typedef enum{
        OPENPORT=0,
        ALIVE=0xF0,
        GET_LEDS=0xFB,
        SET_LEDS=0xFC,
        GET_BUTTONS=0xFD,
        BUTTONEVENT=0xFA,
    }_eID;

    _eID comandos;
    typedef struct{
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[50];
        uint8_t nBytes;
        uint8_t index;
    }_sDatos ;

    _sDatos bufRX, bufTX;

    typedef union {
        float f32;
        int i32;
        unsigned int ui32;
        unsigned short ui16[2];
        short i16[2];
        uint8_t ui8[4];
        char chr[4];
        unsigned char uchr[4];
    }_udat;

    _udat myWord;
    uint8_t  index, nbytes, cks, header, timeoutRx,numLed,ledState;
};
#endif // MAINWINDOW_H
