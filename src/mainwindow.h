#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include "tdt4255board.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool eventFilter( QObject* o, QEvent* e );

public slots:
    void connStatusChanged(bool status);
    void updateAllRegisters();
    void bufferOperationProgress(int current, int max);
    void bitfileOperationProgress(int current, int max);

private slots:
    void on_btnConvertInstrs_clicked();
    void on_btnClrInstrs_clicked();
    void on_btnReadStackTop_clicked();
    void on_btnWriteProgram_clicked();
    void on_btnExecOne_clicked();
    void on_btnExecAll_clicked();
    void on_btnReset_clicked();
    void on_btnSelBitfile_clicked();
    void on_btnUpload_clicked();
    void on_btnCheckConnEx0_clicked();
    void on_btnCheckConnEx1_clicked();
    void on_btnLoadDataFromFile_clicked();
    void on_btnLoadInstFromFile_clicked();
    void on_btnEx1ProcReset_clicked();
    void on_btnEx1ProcStart_clicked();
    void on_btnEx1ProcStop_clicked();
    void on_btnReadInst_clicked();
    void on_btnReadData_clicked();
    void on_btnWriteInst_clicked();
    void on_btnWriteData_clicked();
    void on_btnSaveDataToFile_clicked();
    void on_btnSaveInstToFile_clicked();

    void selInstAddrChanged(int addr);
    void selDataAddrChanged(int addr);

private:
    Ui::MainWindow *ui;
    TDT4255Board * m_board;
    QList<quint16> m_programData;

};

#endif // MAINWINDOW_H
