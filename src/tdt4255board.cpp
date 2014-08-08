#include <QDebug>
#include "tdt4255board.h"

TDT4255Board* TDT4255Board::m_instance = 0;

TDT4255Board::TDT4255Board() :
    QObject(0)
{
    m_serialPort = new QSerialPort(this);

    connectToBoard();
}

bool TDT4255Board::executeProgrammingCommand(QString cmdString, QString expectedReply, bool stripACK)
{
    if(cmdString.length() > 0)
    {
        QByteArray cmdData = cmdString.toLocal8Bit();
        cmdData.append('\0');
        m_serialPort->write(cmdData);
    }

    // wait for up to 0.5s for the reply
    m_serialPort->waitForReadyRead(1000);
    QByteArray returnData = m_serialPort->readAll();

    // strip the ack\0 sequence from reply before comparison, if desired
    if(stripACK)
    {
        QByteArray ackSeq = QString("ack").toLocal8Bit().append('\0');
        returnData.replace(ackSeq, QByteArray());
    }

    if(expectedReply != QString::fromLocal8Bit(returnData))
    {
        qDebug() << "command" << cmdString << "expected reply" << expectedReply << "but got" << QString::fromLocal8Bit(returnData);
        qDebug() << "hex:" << returnData.toHex() << "length" << returnData.length();
        return false;
    }

    return true;
}

bool TDT4255Board::sendBitfile(QString fileName)
{
    QFile f(fileName);
    if(!(f.open(QIODevice::ReadOnly)))
    {
        qDebug() << "could not open bitfile";
        return false;
    }

    qDebug() << "sending binary file of size " << f.size();

    QByteArray dataToSend = f.readAll();

    f.close();

    const unsigned int segmentSize = 64;

    if(dataToSend.size() > segmentSize)
    {
        unsigned int segments = dataToSend.size() / segmentSize;
        unsigned int remainder = dataToSend.size() - segments*segmentSize;

        for(unsigned int i = 0; i < segments; i++)
        {
            QByteArray buffer = dataToSend.mid(segmentSize*i, segmentSize);
            m_serialPort->write(buffer);
            emit bitfileOperationProgress(i+1,segments);
            m_serialPort->waitForBytesWritten(10);
        }

        if(remainder > 0)
        {
            QByteArray buffer = dataToSend.mid(segmentSize*(segments),remainder);
            m_serialPort->write(buffer);
            emit bitfileOperationProgress(segments,segments);
        }
    }
    else
    {
        m_serialPort->write(dataToSend);
        emit bitfileOperationProgress(1,1);
    }

    m_serialPort->waitForReadyRead(500);
    QByteArray resp = m_serialPort->readAll();

    if(QString::fromLocal8Bit(resp) != "ack")
    {
        qDebug() << "sendBitfile did not receive ack:";
        qDebug() << "hex:" << resp.toHex() << "length=" << resp.size();
        return false;
    }

    return true;
}

void TDT4255Board::readPortData()
{
    qDebug() << "received: " << QString::fromLocal8Bit(m_serialPort->readAll());
}

TDT4255Board::~TDT4255Board()
{
    disconnectFromBoard();
}

TDT4255Board* TDT4255Board::getInstance()
{
    static QMutex mutex;
    if (!m_instance)
    {
        mutex.lock();

        if (!m_instance)
            m_instance = new TDT4255Board();

        mutex.unlock();
    }

    return m_instance;
}

void TDT4255Board::destroyInstance()
{
    static QMutex mutex;
    mutex.lock();
    delete m_instance;
    m_instance = 0;
    mutex.unlock();
}

bool TDT4255Board::connectToBoard()
{
    if(m_serialPort->isOpen())
        return true;

    // TODO make the comport configurable
#ifdef Q_OS_LINUX
    m_serialPort->setPortName("/dev/ttyACM0");
#else
    m_serialPort->setPortName("COM5");
#endif

    if(!m_serialPort->open(QIODevice::ReadWrite))
    {
        qDebug() << "Error opening serial port: " << m_serialPort->errorString();
        qDebug() << "Port: " << m_serialPort->portName();
        return false;
    }

    qDebug() << "Port successfully opened";

    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    return true;
}

void TDT4255Board::disconnectFromBoard()
{
    m_serialPort->close();
}

bool TDT4255Board::verifyConnection(quint16 magicRegAddr, QString magicRegExpectedVal)
{
    if(!m_serialPort->isOpen())
    {
        qDebug() << "verifyConnection: port not open, attempting to open..";
        if(!connectToBoard())
            return false;
    }

    QByteArray magicBuf(4,0);
    if(!readBuffer(magicRegAddr, magicBuf))
    {
        qDebug() << "verifyConnection: could not read magic word";
        return false;
    }

    if(QString::fromLocal8Bit(magicBuf.toHex()).toLower() != magicRegExpectedVal)
    {
        qDebug() << "verifyConnection: unexpected magic word" << magicBuf.toHex();
        return false;
    }

    qDebug() << "verifyConnection successful!";

    return true;
}

bool TDT4255Board::flashBitfile(QString fileName)
{
    QFileInfo f(fileName);

    if(!f.exists())
    {
        qDebug() << "file " << fileName << " does not exist!";
        return false;
    }

    if(!m_serialPort->isOpen())
    {
        if(!connectToBoard())
        {
            qDebug() << "port is not open, cannot upload bitfile";
            return false;
        }
    }

    // clear read buffer in case there is something left there
    m_serialPort->readAll();

    // step 1: check firmware version
    if(!executeProgrammingCommand("get_ver", "3.0.2"))
        return false;

    // step 2: preludium
    if(!executeProgrammingCommand("load_config 1", "ack", false))
        return false;
    if(!executeProgrammingCommand("drive_prog 0", "ack", false))
        return false;
    if(!executeProgrammingCommand("drive_mode 7", "ack", false))
        return false;
    if(!executeProgrammingCommand("spi_mode 1", "ack", false))
        return false;
    if(!executeProgrammingCommand("drive_prog 1", "ack", false))
        return false;
    // read_init seems to work a bit on-and-off, so removing it for now
    // this should be investigated in some detail
    //if(!executeProgrammingCommand("read_init", "\x1", true))
    //    return false;
    if(!executeProgrammingCommand("drive_mode 8", "ack", false))
        return false;
    if(!executeProgrammingCommand("fpga_rst 1", "ack", false))
        return false;

    // step 3: send the bitfile info

    if(!executeProgrammingCommand("ss_program " + QString::number(f.size()), "ack", false))
        return false;

    // step 4: send the bitfileOperationProgress(
    if(!sendBitfile(fileName))
        return false;

    // step 5: postludium

    // these next two commands seem to give unpredictable results,
    // so we skip them for now (they are just for checking that
    // the FPGA is in the correct state)
    /*
    if(!executeProgrammingCommand("read_init", "\0", true))
        return false;
    if(!executeProgrammingCommand("read_done", "", false))
        return false;
    */

    if(!executeProgrammingCommand("spi_mode 0", "ack", false))
        return false;
    if(!executeProgrammingCommand("load_config 0", "ack", false))
        return false;
    if(!executeProgrammingCommand("fpga_rst 0", "ack", false))
        return false;


    return true;
}

bool TDT4255Board::readRegister(quint16 address, quint8 &value)
{
    if(!m_serialPort->isOpen())
        return false;
    // example command string for reading register at address 0x0003:
    // r 0003
    QString commandString = QString("r %1\n").arg((ushort) address, 4, 16, QLatin1Char('0'));
    m_serialPort->write(commandString.toLocal8Bit());
    // wait for response, timeout after 1 sec
    m_serialPort->waitForReadyRead(1000);
    QByteArray receivedData = m_serialPort->readAll();

    if(receivedData.size() != 4)
    {
        qDebug() << "readRegister got invalid response: " << receivedData.toHex();
        return false;
    }

    // convert hex string to number
    bool conversionOK = false;
    quint8 val = (quint8) QString::fromLocal8Bit(receivedData).toUInt(&conversionOK,16);

    if(!conversionOK)
    {
        qDebug() << "readRegister got invalid data: " << receivedData;
        return false;
    }

    value = val;

    return true;
}

bool TDT4255Board::writeRegister(quint16 address, quint8 value)
{
    if(!m_serialPort->isOpen())
        return false;

    // example of writing 0xfe at address 0x8000:
    // w fe 8000
    QString commandString = QString("w %1 %2\n")
            .arg(value, 2, 16, QLatin1Char('0'))
            .arg(address, 4, 16, QLatin1Char('0'));

    m_serialPort->write(commandString.toLocal8Bit());

    return true;
}

bool TDT4255Board::readBuffer(quint16 baseAddress, QByteArray &buffer)
{
    bool ok = true;
    for(int i = 0; i < buffer.size(); i++)
    {
        ok &= readRegister(baseAddress, (quint8&) buffer.data()[i]);
        if(!ok)
            break;
        baseAddress++;
        emit bufferOperationProgress(i+1, buffer.size());
    }

    if(!ok)
    {
        qDebug() << "readBuffer failed at address " << baseAddress;
        return false;
    }

    return true;
}

bool TDT4255Board::writeBuffer(quint16 baseAddress, QByteArray buffer)
{
    bool ok = true;
    for(int i = 0; i < buffer.size(); i++)
    {
        ok &= writeRegister(baseAddress, buffer.at(i));
        if(!ok)
            break;
        baseAddress++;
        emit bufferOperationProgress(i+1, buffer.size());
    }

    if(!ok)
    {
        qDebug() << "writeBuffer failed at address " << baseAddress;
        return false;
    }

    return true;
}
