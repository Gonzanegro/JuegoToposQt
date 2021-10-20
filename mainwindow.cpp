#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    miTimer = new QTimer(this);
    ui->setupUi(this);
    miPaintBox = new QPaintBox(0,0,ui->widget);
    QSerialPort1= new QSerialPort(this);
    QSerialPort1->setPortName("COM6");
    QSerialPort1->setBaudRate(115200);
    QSerialPort1->setDataBits(QSerialPort::Data8);
    QSerialPort1->setParity(QSerialPort::NoParity);
    QSerialPort1->setFlowControl(QSerialPort::NoFlowControl);
    connect(QSerialPort1, &QSerialPort::readyRead, this, &MainWindow::onQSerialPort1Rx);
    connect(miTimer,&QTimer::timeout,this,&MainWindow::miTimerOnTime);

    ui->textBrowser->setTextColor(255);
    ui->textBrowser->append("seleccione un comando y presione el boton enviar");
    ui->pushButton->setEnabled(false);
    miTimer->start(10);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    switch (index) {
        case CBWAITING:
            ui->pushButton->setEnabled(false);
        break;
        case CBOPENP:
            ui->pushButton->setEnabled(true);
            comandos=OPENPORT;
        break;
        case CBALIVE:
           if(QSerialPort1->isOpen()){
            ui->pushButton->setEnabled(true);
            comandos=ALIVE;
           }else{
             ui->pushButton->setEnabled(false);
           }
        break;
        case CBGETLED:
            if(QSerialPort1->isOpen()){
                ui->pushButton->setEnabled(true);
                comandos=GET_LEDS;
            }else{
                ui->pushButton->setEnabled(false);
            }
        break;;
        case CBSETLED:
            if(QSerialPort1->isOpen()){
                ui->pushButton->setEnabled(true);
                comandos=SET_LEDS;
            }else{
                ui->pushButton->setEnabled(false);
            }
        break;
    }
}
void MainWindow::on_pushButton_clicked(){
    switch(comandos){
        case OPENPORT:
                if(QSerialPort1->isOpen()){ //si el puerto esta abierto
                        QSerialPort1->close(); //lo cierra
                        ui->comboBox->setItemText(1,"Open Port");//si se pudo cerrar
                        ui->textBrowser->clear();
                    }
                    else{
                        if(QSerialPort1->open(QSerialPort::ReadWrite)){
                            ui->comboBox->setItemText(1,"close Port");//si se pudo cerrar//si se pudo abrir

                        }else{
                            QMessageBox::information(this, "PORT", "NO se pudo abrir el PUERTO");
                        }
                    }

        break;
        case ALIVE:
            SetBufTX(ALIVE);
        break;
        case SET_LEDS:
            //SetBufTX(SET_LEDS);
            setLeds();
        break;
        case GET_LEDS:
            SetBufTX(GET_LEDS);
        break;
        case GET_BUTTONS:

        break;
        case BUTTONEVENT:

        break;

    }
}
void MainWindow::SetBufTX(uint8_t ID){
    QString buf;
    bufTX.index=0;
    bufTX.payLoad[bufTX.index++]='U';
    bufTX.payLoad[bufTX.index++]='N';
    bufTX.payLoad[bufTX.index++]='E';
    bufTX.payLoad[bufTX.index++]='R';
    bufTX.payLoad[bufTX.index++]= 0 ;
    bufTX.payLoad[bufTX.index++]=':';
    switch (ID) {
        case ALIVE:
        bufTX.payLoad[bufTX.index++]=ALIVE;
        bufTX.payLoad[NBYTES]=0x02;
        break;
        case GET_LEDS:
        bufTX.payLoad[bufTX.index++]=GET_LEDS;
        bufTX.payLoad[NBYTES]=0x02;
        break;
        case SET_LEDS:
        bufTX.payLoad[bufTX.index++]=SET_LEDS;
        bufTX.payLoad[bufTX.index++]=numLed;
        bufTX.payLoad[bufTX.index++]=ledState;
        bufTX.payLoad[NBYTES]=0x04;
        break;
    }
    bufTX.cheksum=0;
    for(int i=0;i<bufTX.index;i++){
        bufTX.cheksum ^=bufTX.payLoad[i];
    }
    bufTX.payLoad[bufTX.index++]=bufTX.cheksum;

    if(QSerialPort1->isWritable()){
       QSerialPort1->write((char *)bufTX.payLoad,bufTX.payLoad[NBYTES]+6);
   }
   QString str;
   for(int i=0;i<bufTX.index;i++){
    //str = str + QString("%1").arg(bufTX.payLoad[i],2,16,QChar('0'));
    if(isalnum(bufTX.payLoad[i]))
        str = str + QString("%1").arg((char )bufTX.payLoad[i]);
    else
        str = str + "/" + QString("%1").arg(bufTX.payLoad[i],2,16,QChar('0')) +"/";
   }
   ui->textBrowser->setTextColor(Qt::black);
   ui->textBrowser->append("pc->mbed :"+ str);

}
void MainWindow::miTimerOnTime(){
    if(bufRX.timeOut!=0){
        bufRX.timeOut--;
    }else{
        estado=START;
    }
}
void MainWindow::decodeData(){
    QString str ;
   ui->textBrowser->setTextColor(Qt::red);
    for(int i=1;i<bufRX.index;i++){
        switch (bufRX.payLoad[1]){
            case ALIVE:
                  str="MBED -> PC ID VALIDO ACK";
                break;
            case GET_LEDS:
                str="MBED->PC:GETLEDS ACK :)";

                break;
            case SET_LEDS:
                str="MBED->PC:SETLEDS ACK :)";
                break;
            case GET_BUTTONS:

            break;
            case BUTTONEVENT:
               str=" MBED -> PC : se presiono un boton");
            break;
            default:
            str=((char *)bufRX.payLoad);
            str= ("MBED-->PC *ID Invalido * (" + str + ")");
            break;
            }
   }
   ui->textBrowser->append(str);
}
void MainWindow::onQSerialPort1Rx(){
    unsigned char *reception;
    int count;
    QString str;
    count=QSerialPort1->bytesAvailable();

    if(count <= 0)
        return;

    reception= new unsigned char [count];

    QSerialPort1->read((char *)reception ,count);

    for(int i=0;i<count;i++){
     if(isalnum(reception[i]))
         str = str + QString("%1").arg((char )reception[i]);
     else
         str = str + "/" + QString("%1").arg(reception[i],2,16,QChar('0')) +"/";
    }
    ui->textBrowser->setTextColor(Qt::green);
    ui->textBrowser->append("MBED->PC: " +str );
    bufRX.timeOut =5;
    for(int i=0;i<count;i++){
        switch (estado){
            case START:
                    if(reception[i]== 'U'){ //recibio la U
                        estado=HEADER_1;
                        bufRX.cheksum=0;
                    }
            break;
            case HEADER_1:
                if(reception[i]== 'N') //recibio la N
                    estado=HEADER_2;
                else{
                    i--;
                    estado=START;
                }
            break;
            case HEADER_2:
                if(reception[i]== 'E') //recibio la E
                    estado=HEADER_3;
                else{
                    i--;
                    estado=START;
                }
            break;
            case HEADER_3:
                if(reception[i]== 'R') //recibio la U
                    estado=NBYTES;
                else{
                    i--;
                    estado=START;
                }
            break;
            case NBYTES:
                bufRX.nBytes=reception[i];
                estado=TOKEN;
            break;
            case TOKEN:
                if(reception[i]==':'){
                    estado=PAYLOAD;
                    bufRX.cheksum= 'U' ^ 'N' ^ 'E' ^ 'R' ^ bufRX.nBytes ^ ':';
                    bufRX.payLoad[0]=bufRX.nBytes;
                    bufRX.index=1;
                }else{
                    i--;//token
                    estado=START;
                }
            break;
            case PAYLOAD:

                if(bufRX.nBytes>1){
                    bufRX.payLoad[bufRX.index++]=reception[i];
                    bufRX.cheksum^=reception[i];
                }
                bufRX.nBytes--;
                if(bufRX.nBytes==0){
                    estado=START;
                    if(bufRX.cheksum==reception[i]){
                        decodeData();
                    }
                }
            break;
        default:
            estado=START;
            break;
        }
    }
    delete [] reception;
}

