#include <QtGui>
#include <QtNetwork>
#include <QTime>

#include <stdlib.h>
#include "sender.h"

 Sender::Sender(QWidget *parent)
     : QDialog(parent)
     , imageLbl(new QLabel)
 {


//-------------------camera setup------------------------
// the file : "/home/pi/haarcascade_frontalface_alt.xml" is a file that defined a face.
     if(!faceClassifier.load("/home/pi/haarcascade_frontalface_alt.xml")){exit(-1);}
// use frameTimer to controle frame frequency
     QTimer *frameTimer =  new QTimer(this);
     connect(frameTimer,&QTimer::timeout,this,&Sender::sendPacket);
     frameTimer->setInterval(1);
     frameTimer->start();

     Camera.set( CV_CAP_PROP_FRAME_WIDTH, 256 );
     Camera.set( CV_CAP_PROP_FRAME_HEIGHT, 144 );

// setup the image type
// CV_8UC3 stand for colorfull, CV_8UC1 for black and white
     Camera.set( CV_CAP_PROP_FORMAT, CV_8UC3 );
     Camera.open();

// IP address setup
     groupAddress = QHostAddress("127.0.0.1");

     statusLabel = new QLabel(tr("Ready to send a datagram on the address %1 on port 45454").arg(groupAddress.toString()));

     ttlLabel = new QLabel(tr("Datagram TTL:"));
     ttlSpinBox = new QSpinBox;
     ttlSpinBox->setRange(0, 255);

     QHBoxLayout *ttlLayout = new QHBoxLayout;
     ttlLayout->addWidget(ttlLabel);
     ttlLayout->addWidget(ttlSpinBox);

     startButton = new QPushButton(tr("&Start"));
     quitButton = new QPushButton(tr("&Quit"));

     buttonBox = new QDialogButtonBox;
     buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
     buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

     timer = new QTimer(this);
     udpSocket = new QUdpSocket(this);
     messageNo = 1;

     connect(ttlSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ttlChanged(int)));
     connect(startButton, SIGNAL(clicked()), this, SLOT(startSending()));
     connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
// Permit to call sendDatagram() function to send a datagram to a client
     connect(timer, SIGNAL(timeout()), this, SLOT(sendDatagram()));

     QVBoxLayout *mainLayout = new QVBoxLayout;
     mainLayout->addWidget(statusLabel);
     mainLayout->addLayout(ttlLayout);
     mainLayout->addWidget(imageLbl);
     mainLayout->addWidget(buttonBox);
     setLayout(mainLayout);

     setWindowTitle(tr("UDP Sender"));
     ttlSpinBox->setValue(1);
 }

 void Sender::ttlChanged(int newTtl)
 {
     udpSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, newTtl);
 }

 void Sender::startSending()
 {
     startButton->setEnabled(false);
     timer->start(1000);
 }

 void Sender::sendDatagram()
 {
     statusLabel->setText(tr("Datagram send %1").arg(messageNo));

    QByteArray datagram;
// here we send the time of the server when it send a datagram to see the difference between transmission and reception
    QTime time = QTime::currentTime();
    QString format= "hh:mm:ss.zzz";
    QString timeString = time.toString(format);

    qDebug() << "ServerTime : " << timeString;

    datagram.append(timeString);


    imageCompression();
    for(int i=0;i < compressed_data.size();i++){
        datagram.append(compressed_data[i]);
    }

     udpSocket->writeDatagram(datagram.data(), datagram.size(),
                              groupAddress, 45454);
     ++messageNo;
     qDebug() << "sizeDatagram : " << datagram.size();

 }




 void Sender::sendPacket()
 {
 takePicture();
 faceDetection();
 imageCompression();
 affichageVideo();
 packetGeneration();
 }

// get video
 void Sender::takePicture()
 {
     cv::Mat image;

// Start capture
     Camera.grab();
     Camera.retrieve ( image);

     //std::cout<<"Stop camera..."<<std::endl;

     //save image
// The camera is roll on the raspberry, so we have to flip the image
     cv::flip(image,flip_image,0);
 }

// face detection function, draw red square around face
 void Sender::faceDetection()
 {
     cv::Mat grey;
     std::vector<cv::Rect> faces;
// convert image in gray, easier to detect a face on a grey image
     cv::cvtColor(flip_image,grey,CV_BGR2GRAY);
     faceClassifier.detectMultiScale(grey,faces);

     for(int i=0; i < faces.size();i++){
         cv::rectangle(flip_image,faces[i],cv::Scalar(255,0,0));
     }
 }


 void Sender::imageCompression()
 {
     std::vector<int> params;
     params.push_back(CV_IMWRITE_JPEG_QUALITY);
     params.push_back(20);
     cv::imencode(".jpg",flip_image,compressed_data,params);
 }
 void Sender::affichageVideo()
 {
     imageLbl->setPixmap(QPixmap::fromImage(MatToQimage(flip_image)));
 }

 QImage Sender::MatToQimage(cv::Mat inMat){
     QImage image( inMat.data,
                               inMat.cols, inMat.rows,
                               static_cast<int>(inMat.step),
                               QImage::Format_RGB888 );

     return image;

 }


 QByteArray Sender::packetGeneration(){
     QByteArray block;
         QDataStream out(&block,QIODevice::WriteOnly);
         out.setVersion(QDataStream::Qt_4_0);

         out << (quint32) (compressed_data.size() + 4);

         out << (quint32) compressed_data.size();

         for(int i=0;i < compressed_data.size();i++){
             out << compressed_data[i];
 }
     return block;
 }
