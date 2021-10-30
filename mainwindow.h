#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QMessageBox>
#include <QTimer>
#include "qpaintbox.h"
#include <stdlib.h>
#include <QDateTime>


#define CBWAITING 0 //indices de la combobox
#define CBOPENP 1
#define CBALIVE 2
#define CBGETLED 3
#define CBSETLED 4
#define ONTIME 50
#define MAXTGO 6000
#define MINTGO 1000
#define MAXTOUTSIDE 5000
#define MINTOUTSIDE 2000
#define TIMEBEFORE 3000
#define FALLING 2
#define RISING 3

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
    /**
     * @brief on_comboBox_currentIndexChanged sirve para seleccionar un comando en la combo box
     * @param index indice seleccionado actualmente
     */
    void on_comboBox_currentIndexChanged(int index);
    /**
     * @brief on_pushButton_clicked en caso de presionar el boton envia el comando correspondiente seleccionado
     * previamente en la combo box
     */
    void on_pushButton_clicked();
    /**
     * @brief onQSerialPort1Rx funcion del buffer de recepcion, conectada a la recepcion de datos
     */
    void onQSerialPort1Rx();
    /**
     * @brief SetBufTX esta funcion se encarga de cargar el buffer de salida con los datos que se desea enviar a la bluepill
     * @param ID este es el id del comando que se va a decodificar en la bluepill
     */
    void SetBufTX(uint8_t ID);
    /**
     * @brief decodeData funcion que se encarga de identificar y utilizar el ID del buffer de recepcion
     */
    void decodeData();
    /**
     * @brief miTimerOnTime funcion para el ticker del buffer de recepcion
     */
    void miTimerOnTime();
    /**
     * @brief juegoOnTime funcion para el ticker del juego
     */
    void juegoOnTime();
    /**
     * @brief paint funcion para inicializar la paintbox con los leds y los botones
     */
    void paint();
    /**
     * @brief refreshButtons se usa cuando se recive un buttonevent para pintar los botones presionados
     * @param index es el numero de boton que se recibe de la bluepill
     */
    void refreshButtons(uint index );
    /**
     * @brief refreshLeds igual que la de los botones dibuja y colorea los leds que estan encendidos
     * @param mask mascara para saber si los leds estan encedidos o no
     * @param index indice del led que se quiere pintar o despintar
     */
    void refreshLeds(uint16_t mask,uint8_t index);
    /**
     * @brief doGame maquina de estados que regula y ejecuta el juego llamada por el ticker del juego
     */
    void doGame();
private:
    Ui::MainWindow *ui;

    QSerialPort *QSerialPort1;
    QString strRx;
    QTimer * miTimer;
    QTimer * timerJuego;
    QPaintBox *miPaintBox;
    /**
     * @brief Unión para armar flags en campo de bits
     *
     */
typedef union{
            unsigned char gameStarted: 1;
            unsigned char b1: 1;
            unsigned char b2: 1;
            unsigned char b3: 1;
            unsigned char b4: 1;
            unsigned char b5: 1;
            unsigned char b6: 1;
            unsigned char b7: 1;
    }_uflag;
_uflag myFlags;
/**
 * @brief enum de estados para la maquina de recepcion de datos
 *
 */
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
    /**
     * @brief enum de estados de la maquina del juego
     *
     */
    typedef enum{
        WAIT,
        BEGIN,
        PLAYING,
    }_eGame;
    _eGame gameState;
    /**
     * @brief enumeracion para tener ordenados los ID de los comandos (para enviar y decodificar)
     *
     */
    typedef enum{
        OPENPORT=0,
        ALIVE=0xF0,
        GET_LEDS=0xFB,
        SET_LEDS=0xFC,
        GET_BUTTONS=0xFD,
        BUTTONEVENT=0xFA,
    }_eID;

    _eID comandos;

    /**
     * @brief struct para regular los datos de los dos buffers
     *
     */
    typedef struct{
        uint8_t timeOut;
        uint8_t cheksum;
        uint8_t payLoad[50];
        uint8_t nBytes;
        uint8_t index;
    }_sDatos ;

    _sDatos bufRX, bufTX;
    /**
     * @brief Unión para manejar los datos de distintos tamaños
     *
     */
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
    /**
     * @brief struct para el control de los tiempos y banderas de los leds en el juego
     *
     */
    typedef struct{
     uint16_t timeToGo;
     uint16_t timeOutside;
     uint16_t userTimeReaction;
     uint16_t timeActual;
     unsigned char isOutside;
     unsigned char gotTime;
    }_sledTime;

    _sledTime ledsGame[4]; //array de estructura de leds

    uint8_t  index, nbytes, cks, header, timeoutRx,numLed,ledState,numButton,flanco;
    uint8_t  errores=0,fallos=0,aciertos=0;
    int puntos=0;
    uint16_t arrayLeds,buttonArray,gameTime=0;
    uint32_t timerRead=0,timeFalling=0,timeRising=0;

};
#endif // MAINWINDOW_H
