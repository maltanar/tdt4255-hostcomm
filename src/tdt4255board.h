#ifndef TDT4255BOARD_H
#define TDT4255BOARD_H

#include <QObject>
#include <QMutex>
#include <QtSerialPort/QtSerialPort>

#define TDT4255_EX0_REGADR_MAGIC_ID     0x4000
#define TDT4255_EX0_REGVAL_MAGIC_ID     "c0decafe"
#define TDT4255_EX0_REGADR_STACKTOP     0x0000
#define TDT4255_EX0_REGADR_INSTLEFT     0x0001
#define TDT4255_EX0_REGADR_INSTPNTR     0x0002
#define TDT4255_EX0_REGADR_RESTPROC     0x0003
#define TDT4255_EX0_PRGDAT_BASEADDR     0x8000

#define TDT4255_EX1_REGADR_MAGIC_ID     0x4000
#define TDT4255_EX1_REGVAL_MAGIC_ID     "cafec0de"
#define TDT4255_EX1_REGADR_ENABPROC     0x0000
#define TDT4255_EX1_REGADR_RESTPROC     0x0001
#define TDT4255_EX1_DATMEM_BASEADDR     0x8000
#define TDT4255_EX1_INSMEM_BASEADDR     0xC000

class TDT4255Board : public QObject
{
    Q_OBJECT
public:
    static TDT4255Board* getInstance();
    static void destroyInstance();

    bool connectToBoard();
    void disconnectFromBoard();


    bool verifyConnection(quint16 magicRegAddr, QString magicRegExpectedVal);

    bool flashBitfile(QString fileName);

    bool readRegister(quint16 address, quint8 &value);
    bool writeRegister(quint16 address, quint8 value);

    bool readBuffer(quint16 baseAddress, QByteArray & buffer);
    bool writeBuffer(quint16 baseAddress, QByteArray buffer);

private:
    TDT4255Board();
    ~TDT4255Board();

    TDT4255Board(const TDT4255Board &); // hide copy constructor
    TDT4255Board& operator=(const TDT4255Board &); // hide assign operator


    static TDT4255Board* m_instance;

protected:
    QSerialPort * m_serialPort;

    bool executeProgrammingCommand(QString cmdString, QString expectedReply, bool stripACK = true);
    bool sendBitfile(QString fileName);

signals:
    void bufferOperationProgress(int current, int max);
    void bitfileOperationProgress(int current, int max);

public slots:
    void readPortData();

};

#endif // TDT4255BOARD_H
