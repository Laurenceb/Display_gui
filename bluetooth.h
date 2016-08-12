#ifndef BLUETOOTH_H
#define BLUETOOTH_H
#include <QObject>
#include <QByteArray>
#include <QString>
#include <QQueue>
#include <QBluetoothSocket>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>

//These are used to select the potential datalogger devices from the list of found bluetooth devices - filtering by name
#define PLAUSIBLE_NAME_1 "Equine"/*This is the name format given to configured devices*/
#define PLAUSIBLE_NAME_2 "RNBT"/*The factory default name for the RN-42 modules*/

class BlueTooth : public QObject
{
	Q_OBJECT
	public:
		explicit BlueTooth(QObject *parent = 0);
		~BlueTooth();
		QBluetoothSocket* BlueToothSocket;
		QBluetoothDeviceDiscoveryAgent* BlueToothDiscovery;
		QBluetoothAddress BlueToothAdapterAddress;
		QStringList DeviceNameList;	//A list of device names
		bool Connection;		//True when a device is connected
		QQueue<unsigned char> queue_;	//Used for data output (i.e. passing back of data that is received over the bluetooth link)
		void ParseDeviceList(void);	//After the device list has been parsed, the DeviceNameList can be used to populate connection buttons
		void writeBytes(QByteArray * byteArray);//Writes to connected device
		void ReceiveAvailableData(void);
		QStringList* GetDeviceNames(); //Retreive the list of device names, called by the portselectdialog to update the list of icons
		bool ConnectToDevice(int n);	//Connect to a device, use the index into the list of device to identify the device to connect to
		void Disconnect(void);		//Disconnect from a connected device
		signals:
		 void error(unsigned error_code);//When an error occurs _during a connection_
		 void portOpened();		//When a device is opened and closed
		 void portClosed();
		 void written();
	private slots:
		 void DeviceDiscoveryErrorHandler(QBluetoothDeviceDiscoveryAgent::Error err);//There was an error during the discovery phase
		 void StartDeviceDiscovery(void);//(Re)starts the device discovery agent
		 void HandleDeviceConnected(void);
		 void HandleDeviceError(QBluetoothSocket::SocketError err);
		 void HandleDeviceDisconnect(void);
		 void processWritten(qint64 numbytes);
	private:
		void DisconnectFromDevice(bool attemptreconnect);
		void StopDeviceDiscovery(void);
		bool DiscoveryInProgress;	//True when a discovery process is ongoing
		bool Attempt_Reconnection;	//Try to reconnect to the device if it has failed
		QBluetoothAddress CurrentDeviceAddress;	//Address of the current connection target
		QList<QBluetoothDeviceInfo> BlueToothInfoList;//List of full device descriptors, note that this should be in the same order to allow simpler indexing
		QList<quint64> BlueToothAddresses;
};
#endif // BLUETOOTH_H
