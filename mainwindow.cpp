#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    miTimer = new QTimer(this);
    timerJuego = new QTimer(this);
    ui->setupUi(this);
    miPaintBox = new QPaintBox(0,0,ui->widget);
    QSerialPort1= new QSerialPort(this);
    QSerialPort1->setPortName("COM6");
    QSerialPort1->setBaudRate(115200);
    QSerialPort1->setDataBits(QSerialPort::Data8);
    QSerialPort1->setParity(QSerialPort::NoParity);
    QSerialPort1->setFlowControl(QSerialPort::NoFlowControl); //parametros del puerto serie
    connect(QSerialPort1, &QSerialPort::readyRead, this, &MainWindow::onQSerialPort1Rx);//conecta para hacer la interrupcion de recepción de datos
    connect(miTimer,&QTimer::timeout,this,&MainWindow::miTimerOnTime); //timeout para bufer de datos
    connect(timerJuego,&QTimer::timeout,this,&MainWindow::juegoOnTime); //timeout para funcion de juego
    ui->textBrowser->setTextColor(255);
    ui->textBrowser->append("seleccione un comando y presione el boton enviar.\n *PRIMERO ENVIE OPENPORT*");
    ui->pushButton->setEnabled(false); //para no permitir enviar datos
    ui->state->setFrameStyle(0);
    ui->state->setText("    ESPERANDO CONEXION...    ");

    miTimer->start(10);
    ui->textBrowser->close();
    timerJuego->start(ONTIME); //inicializa el timer con el intevalo

    QDateTime dt;
    srand(dt.currentDateTime().time().msec()); //semilla
    myFlags.gameStarted=0;
}
MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    switch (index) { //para los comandos de la combobox
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
                        ui->state->setText("    ESPERANDO CONEXION...    ");
                        miPaintBox->getCanvas()->fill(Qt::black);
                        miPaintBox->update();
                    }
                    else{
                        if(QSerialPort1->open(QSerialPort::ReadWrite)){
                            ui->comboBox->setItemText(1,"close Port");//si se pudo abrir
                            ui->state->setText("CONEXION LISTA: ESPERANDO JUGADOR");
                            miPaintBox->getCanvas()->fill(Qt::transparent);
                            paint();//para empezar a dibujar
                            SetBufTX(GET_LEDS);
                        }else{
                            QMessageBox::information(this, "PORT", "NO se pudo abrir el PUERTO");
                        }
                    }

        break;
        case ALIVE:
            SetBufTX(ALIVE);
        break;
        case SET_LEDS:
            //EMPTY;
        break;
        case GET_LEDS:
            SetBufTX(GET_LEDS);
        break;
        case GET_BUTTONS:
            SetBufTX(GET_BUTTONS);
        break;
        case BUTTONEVENT:
        //EMPTY
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
    bufTX.payLoad[bufTX.index++]=':';//termina de armar el header
    switch (ID) {
        case ALIVE: //manda alive
        bufTX.payLoad[bufTX.index++]=ALIVE;
        bufTX.payLoad[NBYTES]=0x02;
        break;
        case GET_LEDS: //pide leds
        bufTX.payLoad[bufTX.index++]=GET_LEDS;
        bufTX.payLoad[NBYTES]=0x02;
        break;
        case SET_LEDS: //manda los leds
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
void MainWindow::juegoOnTime(){
   if(gameTime!=0){ //Si hay tiempo de juego
        gameTime--;
   }else{ //Si no hay
        if(myFlags.gameStarted==1) //En caso de que se haya jugado
            gameState=WAIT;
   }

   doGame(); //funcion encargada de ejecutar el juego
}
void MainWindow::decodeData(){
    QString str ;
    uint16_t mask=0,maxTimeOutside=0,timeOutside=0,userTimeReaction=0;
    ui->textBrowser->setTextColor(Qt::red);

        switch (bufRX.payLoad[1]){ //ID
            case ALIVE:
                  str="MBED -> PC ID VALIDO ACK";
                break;
            case GET_LEDS:
                str="MBED->PC:GETLEDS ACK :)";
                myWord.ui8[0]=bufRX.payLoad[2];
                myWord.ui8[1]=bufRX.payLoad[3];
                arrayLeds=myWord.ui16[0];
                for(int i=0;i<4;i++){ //para dibujar los leds en pantalla
                    mask=0;
                    mask |= 1<<i;
                    refreshLeds(mask,i+1);
                }
            break;
            case SET_LEDS: //para prender y refrescarlos en pantalla
                str="MBED->PC:SETLEDS ACK :)";
                myWord.ui8[0]=bufRX.payLoad[2];
                myWord.ui8[1]=bufRX.payLoad[3];
                arrayLeds=myWord.ui16[0];
                for(int i=0;i<4;i++){
                    mask=0;
                    mask |= 1<<i;
                    refreshLeds(mask,i+1);
                }
            break;
            case GET_BUTTONS: //Pide el array de botones
                myWord.ui8[0]=bufRX.payLoad[2];
                myWord.ui8[1]=bufRX.payLoad[3];
                buttonArray=myWord.ui16[0];

//                for(int i=0;i<4;i++){
//                    buttonMask=0xFFFF;
//                    buttonMask=^buttonArray;
//                    refreshButtons(buttonMask,i+1);
//                }

            break;
            case BUTTONEVENT: //Si se presiono un boton
                numButton=bufRX.payLoad[2];
                flanco=bufRX.payLoad[3];
                myWord.ui8[0]=bufRX.payLoad[4];
                myWord.ui8[1]=bufRX.payLoad[5];
                myWord.ui8[2]=bufRX.payLoad[6];
                myWord.ui8[3]=bufRX.payLoad[7];
                timerRead=myWord.ui32;
                str=" MBED -> PC : se presiono un boton";
                refreshButtons(numButton);
                SetBufTX(GET_LEDS); //actualiza el array de LEDS
                if(gameState==WAIT){ //Si no esta jugando
                    if(flanco==FALLING)//falling
                        timeFalling=timerRead;
                    if(flanco==RISING) //rising
                        timeRising=timerRead;
                    if(timeRising !=0 && timeRising-timeFalling>1000){ //compara los tiempos de flancos para iniciar el juego
                        gameState=BEGIN;
                        gameTime=3000/ONTIME;//tiempo que se va a ejecutar el begining
                        ui->LcdAciertos->display(aciertos);
                        ui->lcdErrores->display(errores);
                        ui->lcdFallos->display(fallos);
                        ui->lcdPuntaje->display(puntos);
                    }
                 }
                if(gameState==PLAYING){ //si esta jugando
                    if(!ledsGame[numButton].isOutside && flanco==FALLING){ //si el led no esta encendido y el flanco es falling
                          errores++;
                          puntos-=20;
                          ui->lcdPuntaje->display(puntos);
                          ui->lcdErrores->display(errores);
                    }
                    if(ledsGame[numButton].isOutside && flanco==FALLING){ //Si el led salio y el flanco es falling
                        numLed=numButton+1;
                        ledState=0;
                        SetBufTX(SET_LEDS);

                        ledsGame[numButton].isOutside=0; //si se apago apago la flag de salida
                        ledsGame[numButton].gotTime=0; //indico que quiero volver a asignarle tiempo

                        aciertos++; //Aumenta un acierto

                        maxTimeOutside=ledsGame[numButton].timeToGo*ledsGame[numButton].timeToGo;//calculo numerador

                        timeOutside=ledsGame[numButton].timeOutside;//calculo denominador 1

                        userTimeReaction=ledsGame[numButton].timeActual-gameTime;//calculo denominador 2

                        puntos=puntos+((maxTimeOutside)/(timeOutside*userTimeReaction));//calculo de la nueva puntuacion
                        ui->LcdAciertos->display(aciertos);
                        ui->lcdPuntaje->display(puntos);
                    }
                }
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
    ui->textBrowser->append("MBED->PC: " +str ); //imprime el crudo
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
    pen.setWidth(1);
    brush.setColor(Qt::black);
    painter.setPen(pen);
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

    switch (index){ //para seleccionar los colores
        case 1:
        brush.setColor(Qt::green);
        break;
        case 2:
        brush.setColor(Qt::red);
        break;
        case 3:
        brush.setColor(Qt::yellow);
        break;
        case 4:
        brush.setColor(Qt::blue);
        break;
    }

    pen.setColor(Qt::black);
    pen.setWidth(3);
    brush.setStyle(Qt::SolidPattern);
    painter.setPen(pen);
    painter.setBrush(brush); //Setea los parametros


       if(arrayLeds & mask){ //dibuja los leds segun su indice *1 *2 *3 *4
        painter.setBrush(brush);
        painter.drawEllipse((ledPositionx*index)-(ledSize/2),ledPositiony-(ledSize/2),ledSize,ledSize);// led2
       }else{//si estan apagados
        brush.setColor(Qt::white);
        painter.setBrush(brush);
        painter.drawEllipse((ledPositionx*index)-(ledSize/2),ledPositiony-(ledSize/2),ledSize,ledSize);// led2
        }

    miPaintBox->update();
}
void MainWindow::refreshButtons(uint index){
    uint16_t buttonSize=22,buttonPositiony=200,buttonPositionx=150;
    QPainter painter(miPaintBox->getCanvas());
    QPen pen;
    QBrush brush;

    brush.setStyle(Qt::SolidPattern);
    pen.setWidth(3);

            if(flanco==2){
                brush.setColor(Qt::white);
                brush.setStyle(Qt::SolidPattern);
                pen.setStyle(Qt::DotLine);
            }else{
              brush.setColor(Qt::black);
              brush.setStyle(Qt::SolidPattern);
              pen.setStyle(Qt::SolidLine);
            }
            painter.setBrush(brush);
            painter.setPen(pen);
            painter.drawEllipse((buttonPositionx*(index+1) )-(buttonSize/2),buttonPositiony-(buttonSize/2),buttonSize,buttonSize);// led2


//    for(int a=1;a<5;a++) //dibuja los centros de los botones
//        painter.drawEllipse((buttonPositionx*a )-(buttonSize/2),buttonPositiony-(buttonSize/2),buttonSize,buttonSize);// led2



   miPaintBox->update();
}
void MainWindow::doGame(){
    uint16_t timeOn=0,timeOff=0;
    switch(gameState){
        case WAIT:
                if(myFlags.gameStarted==1){//si finalizo
                    errores=0;
                    puntos=0;
                    fallos=0;
                    aciertos=0; //termina de resetear
                    gameTime=TIMEBEFORE/ONTIME;//tiempo que se va a ejecutar el begining
                    ui->lcdCrono->display("-----");
                }
                if(gameTime!=0){//si se termino de jugar
                    if(gameTime==60 || gameTime==45 || gameTime==15){//festejo de los leds
                        for(int i=0;i<4;i++){
                            numLed=i+1;
                            ledState=1;
                            SetBufTX(SET_LEDS);
                        }
                    }
                    if(gameTime==50 || gameTime==30 || gameTime==1){
                        for(int i=0;i<4;i++){
                            numLed=i+1;
                            ledState=0;
                            SetBufTX(SET_LEDS);
                        }
                    }//termina la señalizacion de los leds
                }
                myFlags.gameStarted=0; //reseteo de flag de juego
                if(QSerialPort1->isOpen())
                ui->state->setTextColor(Qt::black);
                if(QSerialPort1->isOpen())
                    ui->state->setText("  ESPERANDO JUGADOR... ");
                else
                    ui->state->setText("DESCONECTADO... SELECCIONE OPEN PORT");
                timeRising=0;

        break;
        case BEGIN:
            myFlags.gameStarted=1;
            for(int i=0;i<4;i++){//inicializa los valores antes de jugar
                ledsGame[i].timeToGo = 0;
                ledsGame[i].timeOutside = 0;
                ledsGame[i].timeActual=0;
                ledsGame[i].gotTime=0;
                ledsGame[i].isOutside=0;
            }
            ui->state->setTextColor(Qt::green);
            ui->state->setText(" comenzando ");
            if(gameTime==59 || gameTime==45 || gameTime==15){//indica el incio del juego destellando 3 veces los leds
                for(int i=0;i<4;i++){
                    numLed=i+1;
                    ledState=1;
                    SetBufTX(SET_LEDS);
                }
            }
            if(gameTime==53 || gameTime==30 || gameTime==1){//apaga los leds
                for(int i=0;i<4;i++){
                    numLed=i+1;
                    ledState=0;
                    SetBufTX(SET_LEDS);
                }
            }
            if(gameTime==0){//si termino la inicializacion
                gameTime=30000/ONTIME; //pone el tiempo de juego
                gameState=PLAYING; //cambia de estado
                }
        break;
        case PLAYING: //hace el juego
            ui->state->setText("PLAYING");
            ui->lcdCrono->display(QString().number((gameTime*ONTIME)/1000,10));//para el cronometro

            for(int i=0;i<4;i++){//genera los tiempos aleatorios
                 if(!(ledsGame[i].gotTime)){  //Si no se le asigno ningun tiempo a ese led
                        ledsGame[i].timeToGo = ((rand()%(MAXTGO-MINTGO))+MINTGO)/ONTIME ;
                        ledsGame[i].timeOutside = ((rand()%(MAXTOUTSIDE-MINTOUTSIDE))+MINTOUTSIDE)/ONTIME;
                        ledsGame[i].timeActual=gameTime;
                        ledsGame[i].gotTime=1;
                        ledsGame[i].isOutside=0;
                 }
//            }

//            for(int i=0;i<4;i++){ //para encender los leds en el tiempo debido
                if(!ledsGame[i].isOutside){//si el topo no salio
                    timeOn=ledsGame[i].timeActual-ledsGame[i].timeToGo;
                    if(ledsGame[i].gotTime==1 && timeOn==gameTime){//si el topo tiene tiempos y ya se cumplio el tiempo de salida
                        numLed=i+1;
                        ledState=1;
                        SetBufTX(SET_LEDS);//prende los leds
                        ledsGame[i].timeActual=gameTime;
                        ledsGame[i].isOutside=1;//pone la flag que salio el topo
                    }
                }
                 if(ledsGame[i].isOutside){//si el topo esta afuera
                        timeOff=ledsGame[i].timeActual-ledsGame[i].timeOutside;//lee el tiempo que tiene que estar encendido
                        if(timeOff==gameTime){//si se agoto el tiempo (fallo )
                            numLed=i+1;
                            ledState=0;
                            SetBufTX(SET_LEDS);//apaga los leds
                            ledsGame[i].isOutside=0; //si se apago apago la flag de salida
                            ledsGame[i].gotTime=0; //indico que quiero volver a asignarle tiempo
                            fallos+=1;
                            puntos-=10;
                            ui->lcdFallos->display(fallos);
                            ui->lcdPuntaje->display(puntos);
                        }
                    }
             }


        break;
        default:
            gameState=WAIT;
        break;
    }
}
