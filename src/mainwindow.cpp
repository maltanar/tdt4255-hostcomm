#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include "QHexEdit/qhexedit.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->lstInstructions->viewport()->installEventFilter(this);
    ui->lstInstructions->installEventFilter(this);

    QByteArray emptyMemData(256, 0);

    ui->dataMemDisplay->setData(emptyMemData);
    ui->instMemDisplay->setData(emptyMemData);

    m_board = TDT4255Board::getInstance();
    connect(m_board, SIGNAL(bufferOperationProgress(int,int)),this,SLOT(bufferOperationProgress(int,int)));
    connect(m_board, SIGNAL(bitfileOperationProgress(int,int)),this,SLOT(bitfileOperationProgress(int,int)));
    connect(m_board, SIGNAL(connStatusChange(bool)), this, SLOT(connStatusChanged(bool)));

    m_board->connectToBoard();

    // set options for the data memory display
    QHexEdit *hexEdit = ui->dataMemDisplay;
    connect(hexEdit, SIGNAL(currentAddressChanged(int)), this, SLOT(selDataAddrChanged(int)));

    hexEdit->setAddressArea(false);
    hexEdit->setAsciiArea(false);
    hexEdit->setHighlighting(true);
    hexEdit->setOverwriteMode(true);
    hexEdit->setReadOnly(false);

    hexEdit->setHighlightingColor(QColor(0xff, 0xff, 0x99, 0xff));
    hexEdit->setAddressAreaColor( QColor(0xd4, 0xd4, 0xd4, 0xff));
    hexEdit->setSelectionColor(QColor(0x6d, 0x9e, 0xff, 0xff));
    hexEdit->setFont(QFont("Courier", 10));

    hexEdit->setAddressWidth(4);

    // set options for the instruction memory display
    hexEdit = ui->instMemDisplay;
    connect(hexEdit, SIGNAL(currentAddressChanged(int)), this, SLOT(selInstAddrChanged(int)));

    hexEdit->setAddressArea(false);
    hexEdit->setAsciiArea(false);
    hexEdit->setHighlighting(true);
    hexEdit->setOverwriteMode(true);
    hexEdit->setReadOnly(false);

    hexEdit->setHighlightingColor(QColor(0xff, 0xff, 0x99, 0xff));
    hexEdit->setAddressAreaColor( QColor(0xd4, 0xd4, 0xd4, 0xff));
    hexEdit->setSelectionColor(QColor(0x6d, 0x9e, 0xff, 0xff));
    hexEdit->setFont(QFont("Courier", 10));

    hexEdit->setAddressWidth(4);

}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *o, QEvent *e)
{
    bool ret = false;

    // consume key events heading for the ex0 instruction list
    // to prevent changing the selected index
    // (we use it to highlight the currently exec'd instruction)

    if( o == this->ui->lstInstructions &&
            ( e->type() == QEvent::KeyPress
              || e->type() == QEvent::KeyRelease ) )
    {
        ret = true;
    }
    else if( o == this->ui->lstInstructions->viewport() &&
             ( e->type() == QEvent::MouseButtonPress
               || e->type() == QEvent::MouseButtonRelease
               || e->type() == QEvent::MouseMove || e->type() == QEvent::MouseButtonDblClick ) )
    {
        ret = true;
    }

    return ret;
}

void MainWindow::connStatusChanged(bool status)
{
    if(status)
        ui->lblBoardConnStatus->setText("Connected");
    else
        ui->lblBoardConnStatus->setText("Disconnected");
}

void MainWindow::updateAllRegisters()
{
    // call all register read handlers
    on_btnReadStackTop_clicked();
}

void MainWindow::bufferOperationProgress(int current, int max)
{
    ui->progressBar->setMaximum(max);
    ui->progressBar->setValue(current);

    ui->prgReadWriteBuf->setMaximum(max);
    ui->prgReadWriteBuf->setValue(current);
}

void MainWindow::bitfileOperationProgress(int current, int max)
{
    ui->prgBitfileProgress->setMaximum(max);
    ui->prgBitfileProgress->setValue(current);
}


void MainWindow::on_btnConvertInstrs_clicked()
{
    QString expr = ui->txtRPNExpression->text();

    QStringList elems = expr.split(" ");

    if(ui->lstInstructions->count() + elems.size() > 256)
    {
        QMessageBox::critical(this, "Error", "Too many instructions (max 256)");
        return;
    }

    // create instruction buffer while adding QListWidget entries

    foreach(QString elem, elems)
    {
        if(elem == "+")
        {
            ui->lstInstructions->addItem("ADD");
            m_programData.append(0x0100);
        }
        else if (elem == "-")
        {
            ui->lstInstructions->addItem("SUB");
            m_programData.append(0x0200);
        }
        else if(elem.toInt() != 0)
        {
            if(elem.toInt() < -128 || elem.toInt() > 127)
            {
                QMessageBox::critical(this, "Error", "Values must be in range [-128, 127]");
                return;
            }
            ui->lstInstructions->addItem("PUSH "+ elem);
            m_programData.append((quint16)(quint8)elem.toInt());
        }
        else
        {
            QMessageBox::critical(this, "Error", "Unrecognized symbol: " + elem);
            return;
        }
    }

    ui->lblInstrCount->setText("Instructions: (count = " + QString::number(ui->lstInstructions->count()) + ")");
}

void MainWindow::on_btnClrInstrs_clicked()
{
    ui->lstInstructions->clear();
    m_programData.clear();
    ui->lblInstrCount->setText("Instructions: (count = 0)");
}

void MainWindow::on_btnReadStackTop_clicked()
{
    quint8 val = 0;
    if(m_board->readRegister(TDT4255_EX0_REGADR_STACKTOP, val))
        ui->txtStackTop->setText(QString::number((qint8) val));
    else
        ui->txtStackTop->setText("error!");
}


void MainWindow::on_btnWriteProgram_clicked()
{
    // TODO
    QByteArray programBytes;
    QDataStream programStream(&programBytes, QIODevice::ReadWrite);
    programStream.setByteOrder(QDataStream::LittleEndian);

    for(int i = 0; i < m_programData.count(); i++)
        programStream << m_programData.at(i);

    //qDebug() << programBytes.toHex();

    if(!m_board->writeBuffer(TDT4255_EX0_PRGDAT_BASEADDR, programBytes))
    {
        QMessageBox::critical(this, "Error", "Could not write program data");
        return;
    }

    // read and verify that program has been correctly written
    QByteArray verifyBytes(programBytes.size(), 0);
    if(!m_board->readBuffer(TDT4255_EX0_PRGDAT_BASEADDR, verifyBytes))
    {
        QMessageBox::critical(this, "Error", "Could not read to verify program data");
        return;
    }

    if(verifyBytes == programBytes)
        QMessageBox::information(this, "Message", QString::number(m_programData.size())+ " instructions successfully programmed");
    else
        QMessageBox::critical(this, "Error", "Program data written but not verified");
}

void MainWindow::on_btnExecOne_clicked()
{
    ui->lstInstructions->setCurrentRow(ui->lstInstructions->currentRow()+1);
    m_board->writeRegister(TDT4255_EX0_REGADR_INSTLEFT, 0x01);
    updateAllRegisters();
}

void MainWindow::on_btnExecAll_clicked()
{
    m_board->writeRegister(TDT4255_EX0_REGADR_INSTLEFT, (quint8) m_programData.size());
    updateAllRegisters();

    ui->lstInstructions->setCurrentRow(ui->lstInstructions->count()-1);
}

void MainWindow::on_btnReset_clicked()
{
    // reset on
    m_board->writeRegister(TDT4255_EX0_REGADR_RESTPROC, 1);
    // set remaining instructions to zero
    m_board->writeRegister(TDT4255_EX0_REGADR_INSTLEFT, 0);
    // write FF to instruction pointer (workaround for not losing the first instruction)
    m_board->writeRegister(TDT4255_EX0_REGADR_INSTPNTR, 0xFF);
    // reset off
    m_board->writeRegister(TDT4255_EX0_REGADR_RESTPROC, 0);
    updateAllRegisters();

    // clear selection from UI list
    ui->lstInstructions->clearSelection();
    ui->lstInstructions->setCurrentRow(-1);
}

void MainWindow::on_btnSelBitfile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(0, "Select bitfile", "", "*.bit");

    if(!fileName.isEmpty())
        ui->txtBitfile->setText(fileName);
}

void MainWindow::on_btnUpload_clicked()
{
    if(ui->txtBitfile->text().isEmpty())
    {
        QMessageBox::critical(this, "Error", "Select bitfile to upload");
        return;
    }

    if(!m_board->flashBitfile(ui->txtBitfile->text()))
        QMessageBox::critical(this, "Error", "Failed to upload bitfile to FPGA");
    else
        QMessageBox::information(this, "Success", "Bitfile successfully uploaded to FPGA");

    on_btnCheckConnEx0_clicked();
    on_btnCheckConnEx1_clicked();
}

void MainWindow::on_btnCheckConnEx0_clicked()
{
    if(!m_board->verifyConnection(TDT4255_EX0_REGADR_MAGIC_ID, TDT4255_EX0_REGVAL_MAGIC_ID))
    {
        ui->lblConnectionStatusEx0->setText("Exercise framework status: Disconnected");
        ui->grpProcControl->setEnabled(false);
    }
    else
    {
        ui->lblConnectionStatusEx0->setText("Exercise framework status: Connected");
        ui->grpProcControl->setEnabled(true);
    }
}

void MainWindow::on_btnCheckConnEx1_clicked()
{
    bool en = false;
    if(!m_board->verifyConnection(TDT4255_EX1_REGADR_MAGIC_ID, TDT4255_EX1_REGVAL_MAGIC_ID))
    {
        ui->lblConnectionStatusEx1->setText("Exercise framework status: Disconnected");
        en = false;
    }
    else
    {
        ui->lblConnectionStatusEx1->setText("Exercise framework status: Connected");
        en = true;
    }

    if(en)
    {
        // stop and reset processor upon connected check
        on_btnEx1ProcStop_clicked();
        on_btnEx1ProcReset_clicked();
    }

    // enable/disable UI elements based on conn status
    ui->grpEx1DataMem->setEnabled(en);
    ui->grpEx1InstMem->setEnabled(en);
    ui->grpEx1ProcCtrl->setEnabled(en);


}

void MainWindow::on_btnLoadDataFromFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(0, "Fill Data Memory", "", "");

    if(!fileName.isEmpty())
    {
        QFile f(fileName);
        f.open(QIODevice::ReadOnly);
        QByteArray dat = f.readAll();
        f.close();
        dat.resize(256);
        ui->dataMemDisplay->setData(dat);
    }
}

void MainWindow::on_btnLoadInstFromFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(0, "Fill Instruction Memory", "", "");

    if(!fileName.isEmpty())
    {
        QFile f(fileName);
        f.open(QIODevice::ReadOnly);
        QByteArray dat = f.readAll();
        f.close();
        dat.resize(256);
        ui->instMemDisplay->setData(dat);
    }
}

void MainWindow::on_btnEx1ProcReset_clicked()
{
    m_board->writeRegister(TDT4255_EX1_REGADR_RESTPROC, 1);
    m_board->writeRegister(TDT4255_EX1_REGADR_RESTPROC, 0);
}

void MainWindow::on_btnEx1ProcStart_clicked()
{
    m_board->writeRegister(TDT4255_EX1_REGADR_ENABPROC, 1);

    // i/d memory should not be touched while processor is running
    ui->grpEx1DataMem->setEnabled(false);
    ui->grpEx1InstMem->setEnabled(false);
}

void MainWindow::on_btnEx1ProcStop_clicked()
{
    m_board->writeRegister(TDT4255_EX1_REGADR_ENABPROC, 0);

    // i/d memory can now be modified again
    ui->grpEx1DataMem->setEnabled(true);
    ui->grpEx1InstMem->setEnabled(true);
}

void MainWindow::on_btnReadInst_clicked()
{
    QByteArray buf(256, 0);
    m_board->readBuffer(TDT4255_EX1_INSMEM_BASEADDR, buf);
    ui->instMemDisplay->setData(buf);
}

void MainWindow::on_btnReadData_clicked()
{
    QByteArray buf(256, 0);
    m_board->readBuffer(TDT4255_EX1_DATMEM_BASEADDR, buf);
    ui->dataMemDisplay->setData(buf);
}

void MainWindow::on_btnWriteInst_clicked()
{
    m_board->writeBuffer(TDT4255_EX1_INSMEM_BASEADDR, ui->instMemDisplay->data());
}

void MainWindow::on_btnWriteData_clicked()
{
    m_board->writeBuffer(TDT4255_EX1_DATMEM_BASEADDR, ui->dataMemDisplay->data());
}

void MainWindow::on_btnSaveDataToFile_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(0, "Save Data Memory", "", "");

    if(!fileName.isEmpty())
    {
        QFile f(fileName);
        f.open(QIODevice::ReadWrite);
        f.write(ui->dataMemDisplay->data());
        f.close();
    }
}

void MainWindow::on_btnSaveInstToFile_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(0, "Save Instruction Memory", "", "");

    if(!fileName.isEmpty())
    {
        QFile f(fileName);
        f.open(QIODevice::ReadWrite);
        f.write(ui->instMemDisplay->data());
        f.close();
    }
}

void MainWindow::selInstAddrChanged(int addr)
{
    ui->lblSelInstAddr->setText("addr = " + QString::number(addr/4));

    QByteArray selWord = ui->instMemDisplay->data().mid(4*(addr/4),4);
    int data = (*(int *) selWord.data());

    ui->lblSelInstValSigned->setText("signed = " + QString::number((data)));
    data = qToBigEndian(data);
    ui->lblSelInstValHex->setText("hex =" + QByteArray((char *)&data, 4).toHex());
}

void MainWindow::selDataAddrChanged(int addr)
{
    ui->lblSelDataAddr->setText("addr = " + QString::number(addr/4));

    QByteArray selWord = ui->dataMemDisplay->data().mid(4*(addr/4),4);
    int data = (*(int *) selWord.data());

    ui->lblSelDataValSigned->setText("signed = " + QString::number((data)));
    data = qToBigEndian(data);
    ui->lblSelDataValHex->setText("hex =" + QByteArray((char *)&data, 4).toHex());

}
