#include <QRadioButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QDebug>
#include <QString>
#include <QtWidgets>
#include <QStatusBar>
#include <QtSerialPort/QSerialPortInfo>
#include "portselectdialog.h"


PortSelectDialog::PortSelectDialog(QWidget *parent) :
QDialog(parent)
{
	radio_layout = new QVBoxLayout();
	telem_channels_group = new QButtonGroup();
	radio_group = new QButtonGroup();
	radio_group->setExclusive(this);
	//There is also a serialport defined in here, it is connected to the Open button. If the port is already open its closed first
	the_port = new SerialPort(this);
	the_bt = new BlueTooth(this);
	//QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Open, Qt::Horizontal, this);
	m_button = new QPushButton("Connect", this);//Text changes to Reconnect if the currently selected port is already open
	m_button->setGeometry(QRect(QPoint(100, 100),QSize(200, 50)));
	m_button->setEnabled(false);
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addLayout(radio_layout);
	layout->addWidget(m_button);
	QLabel *AuxLabel = new QLabel(tr("Auxillary data channels:"));//This is second to bottom of the tabbed page
	AuxLabel->setFixedHeight(30);
	QFrame *aux_channel_frame = new QFrame();
	aux_channel_frame->setFrameStyle(QFrame::Panel | QFrame::Sunken);//This sunken frame contains the 8 buttons and simgle radio button
	aux_channel_frame->setFixedHeight(50);
	telem_button_layout = new QHBoxLayout(aux_channel_frame);//This button group lies inside the sunken frame
	telem_type_btn = new QRadioButton(tr("Raw IMU"));
	telem_button_layout->addWidget(telem_type_btn);	
	QString style_auxbut="QPushButton:checked{background-color: rgba(100,200,120,1.0); border-style: outset; border-width:2px; border-radius: 6px; border-color: black} QPushButton{background-color: rgba(100,100,120,0.5); border-style: outset; border-width:1px; border-radius: 6px; border-color: beige; min-height: 27px;}";
	aux_telem_names << "Roll" << "Pitch" << "Yaw" << "Accel" << "Heading/Batt." << "Speed (km/h)" << "North (m)" << "East (m)";		
	aux_telem_names_raw << "Acc (x)" << "Acc (y)" << "Acc (z)" << "Gyro (x)" << "Gyro (y)" << "Gyro (z)" << "Mag (x)" << "Mag (y)";
	//Loop through, populating all the buttons. Use the standard titles rather than raw mode titles
	for(quint8 n=0; n<8; n++) {
		aux_buttons[n] = new QPushButton(aux_telem_names.at(n));
		telem_button_layout->addWidget(aux_buttons[n]);
		aux_buttons[n]->setCheckable(true);//the buttons can be turned on/off
		if(n==4)
			aux_buttons[n]->setChecked(true);
		aux_buttons[n]->setStyleSheet(style_auxbut);
		telem_channels_group->addButton(aux_buttons[n]);
		telem_channels_group->setId(aux_buttons[n], n);//Fill the two string lists with aux channel title
	}
	telem_channels_group->setExclusive(false);
	layout->addWidget(AuxLabel);
	layout->addWidget(aux_channel_frame);//the frame containing the buttons
	QLabel *DeviceLabel = new QLabel(tr("Device:"));//This is at the bottom of the tabbed page and holds the device type information
	DeviceLabel->setFixedHeight(30);
	DeviceValueLabel = new QLabel("");
	DeviceValueLabel->setFixedHeight(30);
	DeviceValueLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	layout->addWidget(DeviceLabel);
	layout->addWidget(DeviceValueLabel);
	Tx_waiting=0;
	numberofserialports=0;
	device_type=NO_DEVICE;	//Nothing connected device type is type 0
	connect(telem_channels_group, SIGNAL(buttonClicked(int)), this, SLOT(onButtonSelected(int)));
	connect(telem_type_btn, SIGNAL(clicked()), this, SLOT(switchTelemType()));
	connect(m_button, SIGNAL (released()), this, SLOT(onAccepted()));//When the button is pressed and released, this function is called
	connect(&portTimer, SIGNAL(timeout()), this, SLOT(drawRadioButtons()));
	connect(the_port, SIGNAL(error(unsigned)), this, SLOT(onSerialError(unsigned)));//This is the Serial error handling, Bluetooth error handler is built in
	connect(the_port, SIGNAL(written()), this, SLOT(onWritten()));
	connect(the_bt, SIGNAL(written()), this, SLOT(onWritten()));//Bt and Serial written signal handling is identical
	connect(the_port, SIGNAL(portClosed()), this, SLOT(closeHandler()));//No need to emit the connectionlost signal if we know there is a port closure
	connect(the_bt, SIGNAL(portClosed()), this, SLOT(closeHandler()));
	connect(radio_group, SIGNAL(buttonClicked(int)), this, SLOT(onRadioSelected(int)));
	connect(radio_group, SIGNAL(buttonReleased(int)), this, SLOT(onRadioSelected(int)));//For the connect/reconnect functionality
	//setLayout(layout);
	portTimer.start(1500); // Interval 0 means to refresh as fast as possible, set 1500ms to get approx 0.67hz. As well as ports, this also updates aux channels
}

void PortSelectDialog::drawRadioButtons()
{
	static int n=0;//Stores how many times we have run
	static QString portnames=QString("");//Used to detected changed config
	static QList<QSerialPortInfo> OldserialPortInfo;
	static QStringList OldBluetoothNames;
	if(the_port->isOpen() || the_bt->Connection)
		return;			//No need to do anything if already connected, also this seems to break the port
	//Find all the suitable serial ports and add appropriate buttons to the layout
	QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
	QString portstring=QString("");
	portname="";			//Wipe the port name, if we have gotten this far then there cannot be a connection
	foreach(const QSerialPortInfo &serialPortInfo, serialPortInfoList) {
		portstring.append(serialPortInfo.portName());	//All names go onto the string
		portstring.append(serialPortInfo.manufacturer());//Sometimes the additional info takes longer to update, so if this changes update
	}
	numberofserialports=serialPortInfoList.count();
	//Next look for Bluetooth devices, these are also listed
	the_bt->ParseDeviceList();	//Parses device list
	QStringList* BlueToothNames=the_bt->GetDeviceNames();
	foreach(const QString &Bt, *BlueToothNames) {
		portstring.append(Bt);	//Also stick Bluetooth device names onto the string
	}
	if(!n || !( portstring == portnames ))	{//Only update at boot or if the serial port config changes
		n=1;
        	QString name=QString("");//Save radio button position based upon the port name, and repopulate
		int checked_id = radio_group->checkedId();
		if(checked_id!=-1 && checked_id<OldserialPortInfo.count() ) {//A button selected that corresponds to a serial port on the old settings
			name.append(OldserialPortInfo.at(checked_id).portName());//Get name from the old list
		}
		OldserialPortInfo=serialPortInfoList;//Store the old info for reference
		if(checked_id!=-1 && (checked_id-OldserialPortInfo.count())<OldBluetoothNames.count()) {//A button is checked, and it could be a Bluetooth device
			name.append(OldBluetoothNames.at(checked_id));
		}
		OldBluetoothNames=*BlueToothNames;//Store the old bluetooth info for reference
		portnames = portstring;
		// Delete all existing widgets from the radio button layout, if any.
		if ( radio_layout != NULL )
		{
		    QLayoutItem* item;
		    while ( ( item = radio_layout->takeAt( 0 ) ) != NULL )
		    {
			delete item->widget();
			delete item;
		    }
		}
		// Remove all the buttons from the radio group
		QList<QAbstractButton*> ButtonList = radio_group->buttons();
		foreach(QAbstractButton* button_, ButtonList) {
			radio_group->removeButton(button_); 
		} 
		portnamelist.clear();
		int i = 0;
		qDebug() << QObject::tr("Port list: ");//First loop through the serial ports
		foreach(const QSerialPortInfo &serialPortInfo, serialPortInfoList) {
			qDebug() << endl
			<< QObject::tr("Port: ") << serialPortInfo.portName() << endl
			<< QObject::tr("Location: ") << serialPortInfo.systemLocation() << endl
			<< QObject::tr("Description: ") << serialPortInfo.description() << endl
			<< QObject::tr("Manufacturer: ") << serialPortInfo.manufacturer() << endl
			<< QObject::tr("Vendor Identifier: ") << (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : QByteArray()) << endl
			<< QObject::tr("Product Identifier: ") << (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : QByteArray()) << endl
			<< QObject::tr("Busy: ") << (serialPortInfo.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) << endl;
			QRadioButton *btn = new QRadioButton(
			QString("%1 : %2 (%3 at %4%5)")
			.arg(serialPortInfo.portName())
			.arg(serialPortInfo.description())
			.arg(serialPortInfo.manufacturer())
			.arg(serialPortInfo.systemLocation())
			.arg(serialPortInfo.isBusy() ? " (Busy)" : ""));
			if(checked_id!=-1 && !QString::compare(serialPortInfo.portName(),name,Qt::CaseInsensitive)) {
				btn->setChecked(true);//If the port configuration changes, the ticked button will still be the one we are connected to (if connected)
			}
			portnamelist << serialPortInfo.portName();//Load the port names into the list of ports
			radio_layout->addWidget(btn);
			radio_group->addButton(btn);
			radio_group->setId(btn, i++);
		}
		numberofserialports=i;			//Store number of serial ports for reference
		foreach(const QString &Bt, *BlueToothNames) {//Then loop through the Bluetooth devices
			QRadioButton *btn = new QRadioButton(Bt);
			if(checked_id!=-1 && !QString::compare(Bt,name,Qt::CaseInsensitive)) {
				btn->setChecked(true);//If the port configuration changes, the ticked button will still be the one we are connected to (if connected)
			}
			portnamelist << Bt;//Load the port names into the list of ports
			radio_layout->addWidget(btn);
			radio_group->addButton(btn);
			radio_group->setId(btn, i++);
		}
		//if(i) {
		//	QLabel *topLabel = new QLabel(tr("Select port and open"));
		//}
		if(!i) {		//If no ports are found, add text widget to the radio layout instead
			m_button->setEnabled(false);//Connect/Reconnect button is greyed out if there is nothing to connect to
			qDebug() << QObject::tr("No suitable ports found");
			QLabel *topLabel = new QLabel(tr("No suitable ports"));
			radio_layout->addWidget(topLabel);
		}
		else
			m_button->setEnabled(true);//Enable the button
	}
	onButtonSelected(n);//call this here to ensure we are always syncronised with button states (especially important at boot time !)
}

void PortSelectDialog::onRadioSelected(int index)
{
	qDebug() << index;
	if(portname==portnamelist.at(index))//The button that is selected corresponds to the name of the current connection
		m_button->setText("Reconnect");	
	else
		m_button->setText("Connect");
}

void PortSelectDialog::onSerialError(unsigned error_code)
{
	if(the_port->isOpen() && (error_code==11 || error_code==12) ) {//Close the port if it is open and serious error, before reopening after setting the name
		the_port->close();//emit signal on error9 or close. This can be used to reset connection manager state machine. Also signal on connection
		//emit connectionLost();//No need for this as it is emitted by the serial port closure
	}
	else if(error_code==9) {
		emit connectionLost();//Code 9 also means lost connection
	}
	int checked_id = radio_group->checkedId();
	if(checked_id == -1) return;
	if(!(error_code>=4 && error_code<=6))	//simple framing, parity or break errors do not cause any action to be taken here
		radio_group->button(checked_id)->setDown(0);//Unset the button for the port
	qDebug() << checked_id;
}

void PortSelectDialog::onAccepted()
{
	bool connectedok=false;
	int checked_id = radio_group->checkedId();
	if(checked_id == -1) return;
	QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();//Use this to check the description of the selected port
	//qDebug() << "openDevice" << ports.at(checked_id).portName();
	//emit openDevice(ports.at(checked_id).portName());
	if(the_port->isOpen())//Close the port if it is open, before reopening after setting the name
		the_port->close();
	else if(the_bt->Connection)
		the_bt->Disconnect();
	if(checked_id<numberofserialports) {//We have to open a Serial device
		portname=portnamelist.at(checked_id);//Store this for future reference, name of the current port
		the_port->setName(portname);
		the_port->open();
		QString sp1ml=QString(SP1ML_DEVICE_DESCRIPTOR);//Use this to test if the connected port is a SP1ML device (using CP2102)
		QString cp2102=QString(CP2102_DEVICE_DESCRIPTOR);
		QString rn42=QString(RN_42_DONGLE_DESCRIPTOR);//FTDI dongle with RN-42 on it
		//The sp1ml device descriptor (defined in the header), is part of this device descriptor
		if(ports.at(checked_id).description().contains(cp2102) || ports.at(checked_id).description().contains(sp1ml)) {
			device_type=SP1ML;	//Type 1 is the SP1ML, currently three types, basic serial (0), SP1ML (1), and RN-42 dongle (2)
		}
		if(ports.at(checked_id).description().contains(rn42)) {
			device_type=RN42;	//Type 2 is the RN42 dongle (Currently the extended functionality such as Inquiry scan is unsupported)
			//the_port->setKeepOpen(false);//This should be set false to avoid io errors on rfcomm devices 
		}
		connectedok=true;
	}
	else {		//This is a bluetooth connection
		device_type=BT;
		checked_id-=numberofserialports;//Number of the bluetooth device according to the bluetooth numbering scheme
		connectedok=the_bt->ConnectToDevice(checked_id);
	}
	if(connectedok) {
		m_button->setText("Connected (click to reconnect)");	
		emit newConnection(device_type);
	}
}

//A high level function to accept signal from graph draw to write data
void PortSelectDialog::txDataWrite(QByteArray * byteArray) {
	if(the_port->isOpen()) {
		the_port->writeBytes(byteArray);
		Tx_waiting=1;
	}
	else if(the_bt->Connection) {
		the_bt->writeBytes(byteArray);
		Tx_waiting=1;
	}
}

void PortSelectDialog::onWritten() {
	Tx_waiting=0;	//Set this to indicate no data waiting in buffer
}

void PortSelectDialog::onRejected()//not currently implimented, closes the widget
{
	close();
}

void PortSelectDialog::setRTS(bool RTS)//This sets the RTS level, used for entering command mode rapidly on the SP1ML dongle 
{
	the_port->setRTS(RTS);//(remeber to tie CP2102 side of JP4 to MODE0 pin of SP1ML, and connect 3k3 resistor in series with 10nF cap to RESET pin)
}

void PortSelectDialog::setDTR(bool DTR)//Currently unused, as pin is not broken out on the SP1ML STEVAL usb board
{
	the_port->setDTR(DTR);
}

void PortSelectDialog::emptyRxBuffer(void) {//Empties the Rx buffer, only use if really essential, so as to 
	if(the_port->isOpen()) {
		QByteArray arr=the_port->port->readAll();
		//qDebug() << "," << arr.count() << ",";
		for(int i=0; i<arr.count(); i++) {//Loop through the character array, placing all received characters into the queue
			the_port->queue_.enqueue(arr.at(i));
		}
		while(!(the_port->queue_.isEmpty())) {
			the_port->queue_.dequeue();//Dump the data and do nothing with it
		}
	}
	else if(the_bt->Connection) {
		the_bt->ReceiveAvailableData();
		while(!(the_bt->queue_.isEmpty())) {
			the_bt->queue_.dequeue();//Dump the data and do nothing with it
		}	
	}
}

void PortSelectDialog::readAsString(QByteArray* data) {
	//while(!(the_port->queue_.isEmpty())) {
	//	data->append(the_port->queue_.dequeue());//Dump the data and put it into the string
	//}
	if(the_port->isOpen())
		data->append(the_port->port->readAll());
	else if(the_bt->Connection) {
		the_bt->ReceiveAvailableData();
		while(!(the_bt->queue_.isEmpty()))
			data->append(the_bt->queue_.dequeue());
	}
}

/*void PortSelectDialog::emptyRxBuffer(void) {
	while(!(queue->isEmpty())) {
		queue->dequeue();//Dump the data and do nothing with it
	}
}*/

void PortSelectDialog::closeHandler(void) {
	emit connectionLost();
}

void PortSelectDialog::setDeviceDescriptor(QByteArray* device) {//Used to set the displayed device label
	DeviceValueLabel->setText(*device);
}

void PortSelectDialog::switchTelemType() {//A slot called when the telemetry type button is checked or unchecked. First thing to do it to relabel the buttons
	static bool buttonselected=false;//by default the button is not selected
	buttonselected=!buttonselected;//flip true/false
	telem_type_btn->setChecked((bool)buttonselected);//Alternate button
	for(quint8 n=0; n<8; n++) {
		if(buttonselected)
			aux_buttons[n]->setText(aux_telem_names_raw.at(n));//Alternate the set of button names
		else
			aux_buttons[n]->setText(aux_telem_names.at(n));
	}
	emit setauxchanmode_(buttonselected);
}

void PortSelectDialog::onButtonSelected(int b) {//Handles aux telem channel reconfiguration
	quint8 mask=0;
	for(quint8 n=0; n<8; n++) {
		if(aux_buttons[n]->isChecked())
			mask|=(1<<n);
	}
	emit setauxmask_(mask);
}

