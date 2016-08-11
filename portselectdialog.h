#ifndef SELECTDEVICEDIALOG_H
#define SELECTDEVICEDIALOG_H
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QLabel>
#include <QRadioButton>
#include "serialport.h"
#include "bluetooth.h"

//This is the default CP2102 config, but the devices can be reconfigured using windows tool from silicon labs
#define CP2102_DEVICE_DESCRIPTOR "CP2102 USB to UART Bridge"
#define SP1ML_DEVICE_DESCRIPTOR "ECG dongle"

//This is the FTDI based RN42 dongle
#define RN_42_DONGLE_DESCRIPTOR "FT232R USB"

enum devicetypes {NO_DEVICE=0,SP1ML,RN42,BT};

class QTextEdit;
class QButtonGroup;
class PortSelectDialog : public QDialog
{
	Q_OBJECT
	public:
		explicit PortSelectDialog(QWidget *parent = 0);
		SerialPort* the_port;	//Can access the buffer in here to get at the incoming data, check its connected first
		BlueTooth* the_bt;	//Bluetooth class
		bool Tx_waiting;	//This is used to check that written data has actually been sent. Note that whilst using QBluetooth it will always be false
		quint8 device_type;	//The type of device that is connected at present
		signals:
		 //void openDevice(QString port_name);
		 void newConnection(int devicetype);//Emitted when a new connection is made and we need to restart the connection manager state machine (type 1==SP1)
		 void connectionLost();	//Emitted when we lose a serial connection
		 void setauxmask_(quint8 mask);//Emitted to set the mask for enabled aux channels
		 void setauxchanmode_(bool buttonselected);//Emitted to force the connection manager to switch between the two possible packet types
	public slots:
		 void setRTS(bool RTS);	//This is set to logic1 to enter command mode.
		 void setDTR(bool DTR);
		 void txDataWrite(QByteArray * byteArray);//Used to write to the port (non blocking, check that data has been written using the Tx_waiting variable)
		 void setDeviceDescriptor(QByteArray* device);//used to label the connected device
		 void readAsString(QByteArray* data);
		 void emptyRxBuffer(void);
	private slots:
		void onRadioSelected(int index);
		void onAccepted();
		void onRejected();
		void onWritten();
		void drawRadioButtons();
		void onSerialError(unsigned error_code);
		void closeHandler();
		void switchTelemType();
		void onButtonSelected(int);
	private:
		QButtonGroup* radio_group;//This is used for the port selectors
		QVBoxLayout* radio_layout;
		QButtonGroup* telem_channels_group;//Used to switch aux telem streams on/off
		QRadioButton* telem_type_btn;
		QHBoxLayout* telem_button_layout;
		QStringList aux_telem_names;//A list of channel names
		QStringList aux_telem_names_raw;//Similar list of channel names, but this time used for the raw telemetry
		QPushButton* aux_buttons[8];
		QStringList portnamelist;//List of port names, used for button control
		QString portname;	//This stores the name of the current port, used to switch the connect button text from "Connect" to "Reconnect"
		QLabel* DeviceValueLabel;//Used for displaying the name of the currently connected device
		QTimer portTimer;
		QPushButton *m_button;	//The connect button, text changes to "Reconnect" when there is a connection (as long as current radio button selected)
		int numberofserialports;//Used to determine if the selected button corresponds to a serial port or a bluetooth device
};
#endif // SELECTDEVICEDIALOG_H
