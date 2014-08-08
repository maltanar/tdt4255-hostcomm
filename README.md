TDT4255 hostcomm utility
by Yaman Umuroglu (yamanu@idi.ntnu.no)
-----------------------------------------

This folder contains the source code for the hostcomm utility, which provides a graphical user interface for the TDT4255 lab exercises, as well as a means to configure the Spartan-6 LX16 Evaluation Kit (used in the laboratory) with new bitfiles.

The entire utility is written in in Qt and tested on Ubuntu 14.04. It should also work on any platform (Linux, Windows, Mac) that provides Qt5 (specifically the QtSerialPort add-on module). To build, open the hostcomm.pro with Qt Creator, or issue the commands:

qmake hostcomm.pro
make

Note that the FPGA board (Avnet Spartan-6 Evaluation Kit) is programmed over a serial port connection, which may need additional permissions (i.e read/write access to /dev/ttyACM0). udev rules for granting the necessary permissions are provided in the udev-rules folder.

