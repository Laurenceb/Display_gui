#include <QDebug>
#include "bluetooth.h"
#include "portselectdialog.h"

//The BlueTooth class runs device discovery and manages connections to rfcomm devices.
BlueTooth::BlueTooth(QObject *parent) :
QObject(parent)
{
	Attempt_Reconnection=false;
	DiscoveryInProgress=false;
	Connection=false;
	CurrentDeviceAddress.clear();
	DeviceNameList.clear();
	QBluetoothLocalDevice* BlueToothLocalDevice = new QBluetoothLocalDevice();
	BlueToothAdapterAddress = BlueToothLocalDevice->address();
	BlueToothSocket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);//The Socket is of the rfcomm type
	BlueToothDiscovery = new QBluetoothDeviceDiscoveryAgent(BlueToothAdapterAddress);
	//Connections for the discovery agent
	connect(BlueToothDiscovery, SIGNAL(finished()), this, SLOT(StartDeviceDiscovery()));//This will cause device discovery to loop forever unless interrupted
	connect(BlueToothDiscovery, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(DeviceDiscoveryErrorHandler(QBluetoothDeviceDiscoveryAgent::Error)) );
	//Connections for the socket
	connect(BlueToothSocket, SIGNAL(connected()), this, SLOT(HandleDeviceConnected()));
	connect(BlueToothSocket, SIGNAL(disconnected()), this, SLOT(HandleDeviceDisconnect()));
	connect(BlueToothSocket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(HandleDeviceError(QBluetoothSocket::SocketError)));
	connect(BlueToothSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(processWritten(qint64)));
	//Run discovery
	StartDeviceDiscovery();//Runs by default at the beginning
}

BlueTooth::~BlueTooth() {
	if(DiscoveryInProgress)
		BlueToothDiscovery->stop();
	if(Connection)
		Disconnect();
}

//Called to start a device discovery
void BlueTooth::StartDeviceDiscovery(){
	DiscoveryInProgress=true;
	BlueToothDiscovery->start();
}

//To stop device discovery
void BlueTooth::StopDeviceDiscovery(){
	DiscoveryInProgress=false;
	BlueToothDiscovery->stop();
}

//Handle an error during discovery, this will restart the discovery if possible, otherwise quits the discovery
void BlueTooth::DeviceDiscoveryErrorHandler(QBluetoothDeviceDiscoveryAgent::Error err){
	bool cancelthis=false;//If we have to cancel ongoing device discovery due to the error
	if(err) {//If there really is an error... Otherwise do nothing
		if(QBluetoothDeviceDiscoveryAgent::InvalidBluetoothAdapterError==err) {
			qDebug() << endl << "Error: invalid Bluetooth Adaptor" << endl;
			cancelthis=true;
		}
		else if(QBluetoothDeviceDiscoveryAgent::UnsupportedPlatformError==err) {
			qDebug() << endl << "Error: platform is unsupported" << endl;
			cancelthis=true;
		}
		else if(QBluetoothDeviceDiscoveryAgent::UnknownError==err) {
			qDebug() << endl << "Error: unknown error" << endl;
			cancelthis=true;
		}
		if(cancelthis)//We have to cancle device discovery
			StopDeviceDiscovery();
		else {	//We attempt to continue with the device discovery
			BlueToothDiscovery->stop();//Turn it off and back on again!
			BlueToothDiscovery->start();//Documentation appears to imply that this will restart the discovery. Stop takes time before emitting cancelled
		}
	}
}

//Retrieve the list of discovered device, note that this will consist only of devices found during the current discovery session
void BlueTooth::ParseDeviceList() {
	if(!DiscoveryInProgress)
		return;			//Do nothing if there is no ongoing device discovery
	BlueToothInfoList=BlueToothDiscovery->discoveredDevices();
	QString Namefilterone=PLAUSIBLE_NAME_1;
	QString Namefiltertwo=PLAUSIBLE_NAME_2;
	QString devname;
	bool anydevicesfound=false;
	foreach(const QBluetoothDeviceInfo &devinfo, BlueToothInfoList) {
		devname=devinfo.name();//If the device name is plausible
		if(devname.contains(Namefilterone)||devname.contains(Namefiltertwo)) {
			if(!DeviceNameList.contains(devname)) {//The name was not previously in the list
				qDebug() << endl << "Found Bluetooth device:" << devname << endl;//Print debug output for each new device as it is found
			}
			DeviceNameList << devname;//Put the name (as a string) into the list
			anydevicesfound=true;
		}
	}
	if(anydevicesfound) {
		DeviceNameList.removeDuplicates();//This removes any duplicate names from the name list. Note that name list consists of all uniques since gui started
		QList<QBluetoothDeviceInfo> BlueToothInfoList_=BlueToothInfoList;//Copy the list to a new temporary copy before we repopulate it
		BlueToothInfoList.clear();
		int n;
		QStringList q;
		foreach(const QBluetoothDeviceInfo &devinfo, BlueToothInfoList_) {
			q << devinfo.name();//names of the current devices in the order that they appear
		}
		foreach(const QString &dev_name, DeviceNameList) {//Now loop through the sorted device name list and populate the list of descriptors
			n=q.indexOf(QRegExp(dev_name));//Index of this name in the old name list
			BlueToothInfoList << BlueToothInfoList_.at(n);//BlueToothInfoList now has the same order as DeviceNameList
		}
	}
}

//Points to the devices stringlist
QStringList* BlueTooth::GetDeviceNames(void){
	return &DeviceNameList;
}

//Connects to a device, the DeviceIndex is the number of the device as an index to the list of devices
void BlueTooth::ConnectToDevice(int DeviceIndex){
	if(DiscoveryInProgress)
		StopDeviceDiscovery();//Don't try to discover whilst also trying to connect
	if(DeviceIndex<BlueToothInfoList.count()) {
		BlueToothSocket->connectToService(BlueToothInfoList.at(DeviceIndex).address(),1,QIODevice::ReadWrite);//Open the device as read/write, port is hardcoded as 1?
		CurrentDeviceAddress=BlueToothInfoList.at(DeviceIndex).address();
	}
	else
		qDebug() << endl << "Error, device number" << DeviceIndex+1 << " Requested out of" << BlueToothInfoList.count() << "devices";
}

//Simple device disconnect, used as a slot to disconnect
void BlueTooth::Disconnect(void) {
	DisconnectFromDevice(false);
}

//This just sets a global so that we know we are connected ok, handles the device connection signal
void BlueTooth::HandleDeviceConnected(void){
	Connection=true;
	emit portOpened();
}

//This will mark the device as disconnected, disconnect, then request a reconnection, which will actually occur at a later time and cause a connection event
void BlueTooth::HandleDeviceError(QBluetoothSocket::SocketError err){
	bool attemptreconnect=false;
	if(err!=-2) {//If there actually is an error
		if(err==QBluetoothSocket::UnknownSocketError) {
			qDebug() << endl << "Error: unknown Bluetooth socket (did you forget to pair the device?)" << endl;
		}
		else if(err==QBluetoothSocket::HostNotFoundError) {
			qDebug() << endl << "Error: Bluetooth host not found" << endl;
			attemptreconnect=true;
		}
		else if(err==QBluetoothSocket::ServiceNotFoundError) {
			qDebug() << endl << "Error: Bluetooth service not found on remote host" << endl;
		}
		else if(err==QBluetoothSocket::NetworkError) {
			qDebug() << endl << "Error: Bluetooth read or write returned an error" << endl;
			attemptreconnect=true;
		}
		else if(err==QBluetoothSocket::UnsupportedProtocolError) {
			qDebug() << endl << "Error: this Bluetooth protocol is unsupported by the platform" << endl;
		}
		else if(err==QBluetoothSocket::OperationError) {
			qDebug() << endl << "Error: Bluetooth socket state error" << endl;
			attemptreconnect=true;
		}
		DisconnectFromDevice(attemptreconnect);//If it is likely to work, try to reconnect to the device
	}
}

//This disconnects from a Bluetooth device, disconnection signal will be emitted later at the actual point of disconnection
void BlueTooth::DisconnectFromDevice(bool attemptreconnect){
	Connection=false;
	Attempt_Reconnection=attemptreconnect;
	BlueToothSocket->disconnectFromService();
}

//Handles the device disconnection signal
void BlueTooth::HandleDeviceDisconnect(void) {
	Connection=false;
	emit portClosed();
	if(Attempt_Reconnection && !(CurrentDeviceAddress.isNull()))
		BlueToothSocket->connectToService(CurrentDeviceAddress,1,QIODevice::ReadWrite);
	else
		StartDeviceDiscovery();//Turn the device discovery back on
}

//Writes to the bluetooth device. Note that unlike with a serial device
void BlueTooth::writeBytes(QByteArray * byteArray) {
	if(Connection && (BlueToothSocket->state()==QBluetoothSocket::ConnectedState)) {//Only write if the socket is open
		BlueToothSocket->write(*byteArray);//this is a non blocking write
		//BlueToothSocket->flush();	//non blocking, bypasses internal buffer
	}
}

//This handles the BytesWritten signal from QIODevice
void BlueTooth::processWritten(qint64 numbytes) {
	emit written();	//this signal is used by the portselectdialog
}

//This function retreives all received data
void BlueTooth::ReceiveAvailableData(void) {
	if(Connection && (BlueToothSocket->state()==QBluetoothSocket::ConnectedState)) {//Only read if the socket is open
		// append new data to buffer
		QByteArray arr=BlueToothSocket->readAll();
		//qDebug() << "," << arr.count() << ",";
		for(int i=0; i<arr.count(); i++) {//Loop through the character array, placing all received characters into the queue
			queue_.enqueue(arr.at(i));
		}
	}
}

