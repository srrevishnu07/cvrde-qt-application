#include "mmws.h"
#include <QPainter>
#include <QtMath>
#include <QSerialPort>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QtSql>
#include<QLabel>
#include<QVBoxLayout>

unsigned char m_activityCounter = 0;

RadarWidget::RadarWidget(QWidget *parent)
    : QWidget(parent), m_degree(0), m_sliderDegree(0), m_fovValue(0.1),m_zoomLevel(1),m_isDTVMode(true){
    m_serialPort = new QSerialPo`rt(this);
    m_serialPort->setPortName("COM14");
    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Initialize the status labels with default text
    DTVStatusLabel = new QLabel("", this);
    TIStatusLabel = new QLabel("", this);
    FOVStatusLabel = new QLabel("", this);



    // Open the serial port for writing
    if (!m_serialPort->open(QIODevice::WriteOnly)) {
        qDebug()<<"Azimuth value"<<m_degree;
        qDebug() << "Error opening serial port:" << m_serialPort->errorString();

    }
    else{
        qDebug()<<"Serial Port Opened Successfully";

    }

    m_timer = new QTimer(this);

    connect(m_timer, &QTimer::timeout, this, &RadarWidget::updateRadar);
    m_timer->start(1000);


}

//setting the degree for the radar
void RadarWidget::setDegree(int degree) {
    if (m_degree != degree) {
        m_degree = degree; // Update the radar degree
        emit degreeChanged(degree); // Emit the signal with the new degree
        prepareAndSendData();
        update();
        qDebug() << "Azimuth value" << m_degree;
    }
}


//for drawing the radar
void RadarWidget::paintEvent(QPaintEvent */*event*/) {
    QPainter painter(this);
    painter.setBrush(Qt::green);
    painter.drawEllipse(rect());
    QPen pen(Qt::red);
    pen.setWidth(2);
    painter.setPen(pen);
    float radians = qDegreesToRadians(static_cast<float>(m_degree));
    int x = width() / 2 + cos(radians) * (width() / 2);
    int y = height() / 2 + sin(radians) * (height() / 2);
    painter.drawLine(width() / 2, height() / 2, x, y);


}

//setting the slider degree and to send the degree to update the value
void RadarWidget::setSliderDegree(int degree) {
    if (m_sliderDegree != degree) {
        m_sliderDegree = degree;
        prepareAndSendData();
        update();
        qDebug() << "Elevation value" << m_sliderDegree;
        emit sliderDegreeChanged(degree); // Emit the signal with the new degree
    }
}


void RadarWidget::onDTVButtonClick() {
    m_dtvButtonState = !m_dtvButtonState; // Toggle the state
    prepareAndSendData();

    m_isDTVMode = !m_isDTVMode; // Toggle between DTV and TI mode
    m_zoomInButtonState = 0;
    m_zoomOutButtonState = 0;
    QString dtvStatus;
    QString tiStatus;
    QString fovStatus;
    if (m_isDTVMode) {
        dtvStatus = "Daycam               Ready";
        tiStatus =  "TI                         Not Ready";
        fovStatus =  "                 0.1";

        m_fovValue = 0.1;
        m_zoomLevel = 1;
    } else {
        dtvStatus = "Daycam                Not Ready";
        tiStatus=   "TI                              Normal";
        fovStatus  = "                  Wide 1x";
         m_fovValue = 1.0;
         m_zoomLevel = 1;
    }
    emit dtvStatusChanged(dtvStatus);

    emit tiStatusChanged(tiStatus);
    emit fovStatusChanged(fovStatus);
}

void RadarWidget::onZoomInButtonClick() {
    if (m_zoomInButtonState < 15) {
        m_zoomInButtonState++;
        m_zoomOutButtonState = 0; // Reset the zoom out button state
        if (m_isDTVMode && m_fovValue < 1.5) {
            m_fovValue += 0.1;
        }
        else if (!m_isDTVMode && m_zoomLevel < 15) {
            m_zoomLevel++;
        }
    } else {
        qDebug() << "Max zoom";
    }
    QString fovStatus = m_isDTVMode ? QString("                    %1").arg(m_fovValue, 0, 'f', 1) :
                            QString("                 Wide %1x").arg(m_zoomLevel);
    emit fovStatusChanged(fovStatus);
    prepareAndSendData();
}

void RadarWidget::onZoomOutButtonClick() {
    if (m_zoomOutButtonState < 15) {
        m_zoomOutButtonState++;
        m_zoomInButtonState = 0; // Reset the zoom in button state
        if (m_isDTVMode && m_fovValue > 0.1) {
            m_fovValue -= 0.1;
        }
        else if (!m_isDTVMode && m_zoomLevel > 1) {
            m_zoomLevel--;
        }
    } else {
        qDebug() << "Minimum zoom level reached";
    }
    QString fovStatus = m_isDTVMode ? QString("                    %1").arg(m_fovValue, 0, 'f', 1) :
                            QString("                 Wide %1x").arg(m_zoomLevel);
    emit fovStatusChanged(fovStatus);
    prepareAndSendData();
}



//F+ has 4bits so max focus is 15
void RadarWidget::onFPlusButtonClick() {
    if (m_fPlusButtonState < 15) {
        m_fPlusButtonState++;
    } else {
        qDebug() << "Maximum Focus";
    }
    prepareAndSendData();
}

//F- has 4 bits so max focus out
void RadarWidget::onFMinusButtonClick() {
    if (m_fMinusButtonState < 15) {
        m_fMinusButtonState++;
    } else {
        qDebug() << "Max focus reduced";
    }
    prepareAndSendData();
}



//sending the data to serial communication
void RadarWidget::prepareAndSendData() {
    qint16 radarDegreeValue = static_cast<qint16>(m_degree);//making it to 16 bits
    qint16 sliderDegreeValue = static_cast<qint16>(m_sliderDegree);
    QByteArray data;//data to be sent through serial communication
    unsigned char eighthByte = 0;//data from the Dtv,zoom+,zoom- buttons
    unsigned char ninthByte = 0;//data from F+,F- buttons
    unsigned char checksum=0;

    eighthByte |= m_dtvButtonState << 7;
    eighthByte |= (m_zoomInButtonState & 0x0F) << 3;
    eighthByte |= (m_zoomOutButtonState & 0x07);
    ninthByte |= (m_fPlusButtonState & 0x0F) << 4;
    ninthByte |= (m_fMinusButtonState & 0x0F);

    data.append(static_cast<char>(0xAA)); // sending to serial communication Header value
    data.append(++m_activityCounter); // Activity counter
    data.append(static_cast<char>(radarDegreeValue & 0xFF)); // Radar degree LSB
    data.append(static_cast<char>((radarDegreeValue >> 8) & 0xFF)); // Radar degree MSB
    data.append(static_cast<char>(sliderDegreeValue & 0xFF)); // Slider degree LSB
    data.append(static_cast<char>((sliderDegreeValue >> 8) & 0xFF));
    data.append(ninthByte);
    data.append(eighthByte);

    for(int i=1;i< data.size();++i)
    {
        checksum += static_cast<unsigned char>(data[i]);//calculating checksum by adding all the data except header and footer
    }
    data.append(checksum);
    data.append(static_cast<char>(0xEE)); // Footer


    if (data.size() == 10) {


        QSqlQuery query;

        QString dateTime = QDateTime::currentDateTime().toString("hh:mm:ss dd/MM/yyyy");//datetime for the timestamping


        // Create a QSqlDatabase object
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

        // Set the database file
        db.setDatabaseName("H:/radarwidgetn/test.db");

        // Open the database
        if (!db.open()) {
            qDebug() << "Error: Unable to open database";
        }
        else
        {
            qDebug()<<"Database Opened Succesfully";
        }


        QString header = QString::number(0xAA, 16).rightJustified(2, '0');//making the header as hexadecimal
        QString footer = QString::number(0xEE, 16).rightJustified(2, '0');//making the footer as hexadecimal

        QString queryString = QString("INSERT INTO serial_data_log "
                                      "(timestamp, header, activity_counter, sight_traverse_lsb, sight_traverse_msb, "
                                      " sight_elevation_lsb, sight_elevation_msb, focus_button_state,zoom_button_state, "
                                      "checksum, footer) VALUES ('%1', '%2', '%3', '%4', '%5', '%6', '%7', '%8', '%9', '%10', '%11')")//inserting the data into the table
                                  .arg(dateTime)
                                  .arg(header)
                                  .arg(QString::number(m_activityCounter, 16).rightJustified(2, '0'))
                                  .arg(QString::number(radarDegreeValue & 0xFF, 16).rightJustified(2, '0'))
                                  .arg(QString::number((radarDegreeValue >> 8) & 0xFF, 16).rightJustified(2, '0'))
                                  .arg(QString::number(sliderDegreeValue & 0xFF, 16).rightJustified(2, '0'))
                                  .arg(QString::number((sliderDegreeValue >> 8) & 0xFF, 16).rightJustified(2, '0'))
                                  .arg(QString::number(ninthByte, 16).rightJustified(2, '0'))
                                  .arg(QString::number(eighthByte, 16).rightJustified(2, '0'))
                                  .arg(QString::number(checksum, 16).rightJustified(2, '0'))
                                  .arg(footer);


        QSqlQuery qry;



        qDebug() << "Executing query:" << query.lastQuery();

        if (qry.exec(queryString)) {
            qDebug() << "Insertion was successful";
        } else {
            qDebug() << "Error:" << qry.lastError().text();
        }

        sendData(data); //sending the data to serial com
    } else {
        qDebug() << "Data packet is not the correct size";
    }
}


//verifying the sent data is of correct size
void RadarWidget::sendData(const QByteArray &data) {
    qint64 bytesWritten = m_serialPort->write(data);
    if(bytesWritten == -1)
    {
        qDebug()<<"Failed to write the data to serial port"<<data.toHex();
    }
    else if(bytesWritten !=data.size())
    {
        qDebug()<<"Failed to write all data to serial port";
    }
    else
    {
        qDebug()<<"Data writted to serial port successfully"<<data.toHex();
        emit dataSent(data);
    }

}

void RadarWidget::updateRadar() {
    prepareAndSendData();
}

//destructor
RadarWidget::~RadarWidget() {
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    delete m_serialPort;

     if (m_db.isOpen()) {
         m_db.close();
     }
}


