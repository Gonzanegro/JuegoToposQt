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
    ui->textBrowser->append("seleccione un comando y presione el boton enviar.\n *PRIMERO ENVIE OPENPORT*");
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
                        ui->textBrowser->append("**************PUERTO CERRADO***************");
                        miPaintBox->getCanvas()->fill(Qt::black);
                        miPaintBox->update();
                    }
                    else{
                        if(QSerialPort1->open(QSerialPort::ReadWrite)){
                            ui->comboBox->setItemText(1,"close Port");//si se pudo cerrar//si se pudo abrir
                            miPaintBox->getCanvas()->fill(Qt::transparent);
                            paint();//para empezar a dibujar
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
        if(ui->checkBox->isChecked())
            ledState=1;
        else
            ledState=0;
        bufTX.payLoad[bufTX.index++]=SET_LEDS;
        bufTX.payLoad[bufTX.index++]=ui->spinBox->value();//numLed;
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
    uint16_t mask=0;
    ui->textBrowser->setTextColor(Qt::red);

        switch (bufRX.payLoad[1]){
            case ALIVE:
                  str="MBED -> PC ID VALIDO ACK";
                break;
            case GET_LEDS:
                str="MBED->PC:GETLEDS ACK :)";
                myWord.ui8[0]=bufRX.payLoad[2];
                myWord.ui8[1]=bufRX.payLoad[3];
                arrayLeds=myWord.ui16[0];
                for(int i=0;i<4;i++){
                    mask |= 1<<i;
                    refreshLeds(mask,i+1);
                }
            break;
            case SET_LEDS:
                str="MBED->PC:SETLEDS ACK :)";
                myWord.ui8[0]=bufRX.payLoad[2];
                myWord.ui8[1]=bufRX.payLoad[3];
                arrayLeds=myWord.ui16[0];
                for(int i=0;i<4;i++){
                    mask |= 1<<i;
                    refreshLeds(mask,i+1);
                }
            break;
            case GET_BUTTONS:
                myWord.ui8[0]=bufRX.payLoad[2];
                myWord.ui8[1]=bufRX.payLoad[3];
                buttonArray=myWord.ui16[0];
                refreshButtons();
            break;
            case BUTTONEVENT:
                numButton=bufRX.payLoad[2];
                flanco=bufRX.payLoad[3];
                myWord.ui8[0]=bufRX.payLoad[4];
                myWord.ui8[1]=bufRX.payLoad[5];
                myWord.ui8[2]=bufRX.payLoad[6];
                myWord.ui8[3]=bufRX.payLoad[7];
                timerRead=myWord.ui32;
                str=" MBED -> PC : se presiono un boton";

                refreshButtons();
                SetBufTX(GET_LEDS);
            break;
            default:
            str=((char *)bufRX.payLoad);
            str= ("MBED-->PC *ID Invalido * (" + str + ")");
            break;
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
void MainWindow::paint(){
    uint16_t ledSize=60,ledPositiony=100,ledPositionx=150,buttonPositiony=200,buttonSize=74,ratio=15;
    QPainter painter(miPaintBox->getCanvas());
    QPen pen;
    QBrush brush;
    painter.setBrush(brush);
    painter.setPen(pen);
    pen.setColor(Qt::black);
    brush.setColor(Qt::black);
    brush.setStyle(Qt::SolidPattern);

//    painter.drawLine(150,0,150,300);
//    painter.drawLine(300,0,300,300);
//    painter.drawLine(450,0,450,300);
//    painter.drawLine(600,0,600,300);

//    painter.drawLine(0,100,750,100);
//    painter.drawLine(0,200,750,200); //guias de simetria
////centros en 100 y 200 eje y  150,300,450,600 eje x
    pen.setColor(Qt::black);
    pen.setWidth(3);
    brush.setColor(Qt::white);
    painter.setPen(pen);

    painter.setBrush(brush);

    for(int j=1;j<5;j++) //dibuja los leds
        painter.drawEllipse((ledPositionx*j)-(ledSize/2),ledPositiony-(ledSize/2),ledSize,ledSize);// led2


    pen.setWidth(5);
    brush.setColor(Qt::gray);
    painter.setBrush(brush);
    painter.setPen(pen);
    for(int i=1;i<5;i++) //dibuja los botones
        painter.drawRoundedRect(((ledPositionx*i)-(buttonSize/2)),(buttonPositiony-(buttonSize/2)),buttonSize,buttonSize,ratio,ratio);

    ledSize=22;
    brush.setColor(Qt::black);
    painter.setBrush(brush);
    for(int a=1;a<5;a++) //dibuja los centros de los botones
        painter.drawEllipse((ledPositionx*a)-(ledSize/2),buttonPositiony-(ledSize/2),ledSize,ledSize);// led2
    painter.drawText(140,50,"LED 1");
    painter.drawText(290,50,"LED 2");
    painter.drawText(440,50,"LED 3");
    painter.drawText(590,50,"LED 4");
    miPaintBox->update();
}
void MainWindow::refreshLeds(uint16_t mask,uint8_t index){
    uint16_t ledSize=60,ledPositiony=100,ledPositionx=150;
    QPainter painter(miPaintBox->getCanvas());
    QPen pen;
    QBrush brush;



    pen.setColor(Qt::black);
    pen.setWidth(3);
    brush.setColor(Qt::black);
    brush.setStyle(Qt::SolidPattern);
    painter.setPen(pen);
    painter.setBrush(brush);


       if(arrayLeds & mask){ //primer led
        brush.setColor(Qt::green);
        painter.setBrush(brush);
        painter.drawEllipse((ledPositionx*index)-(ledSize/2),ledPositiony-(ledSize/2),ledSize,ledSize);// led2
       }else{
        brush.setColor(Qt::white);
        painter.setBrush(brush);
        painter.drawEllipse((ledPositionx*index)-(ledSize/2),ledPositiony-(ledSize/2),ledSize,ledSize);// led2
        }

    miPaintBox->update();
}
void MainWindow::refreshButtons(){

//    QPainter painter(miPaintBox->getCanvas());
//    QPen pen;
//    QBrush brush;
//    SetBufTX(GET_BUTTONS);
//    uint16_t mask=0xFFFF;


//    miPaintBox->update();
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    SetBufTX(SET_LEDS);
}

