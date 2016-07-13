#include <QDebug>
#include <QString>
#include <QDateTime>
#include <QtGlobal>
#include <QObject>
#include <math.h>

#include "connectionmanager.h"
#include "portselectdialog.h"

connectionManager::connectionManager(QObject *parent ,PortSelectDialog* used_port_) :
QObject(parent)
{
	PortSelectDialog* used_port=used_port_;
	//Connect all the signals and slots to the PortSelectDialog interface
	connect(used_port, SIGNAL(newConnection(int)), this, SLOT(newConnection(int)));//A new connection, atm only 2 types supported, type 1 is an SP1ML dongle 
	connect(used_port, SIGNAL(connectionLost()), this, SLOT(lostConnection()));//A disconnection
	connect(this, SIGNAL(sendData(QByteArray*)), used_port, SLOT(txDataWrite(QByteArray*)));//A data write request
	connect(this, SIGNAL(setRTS(bool)), used_port, SLOT(setRTS(bool)));//Set the DTR. This is the only control signal that is currently used. 
	connect(this, SIGNAL(setDeviceDescriptor(QByteArray*)), used_port, SLOT(setDeviceDescriptor(QByteArray*)));//Used to set the device descriptor
	connect(this, SIGNAL(emptyRxBuffer(void)), used_port, SLOT(emptyRxBuffer(void)));
	connect(this, SIGNAL(readAsString(QByteArray*)), used_port, SLOT(readAsString(QByteArray*)));
	SP1MLnetworkaddress=LOWEST_ADDRESS-1;//First device connects at this address. The address is incremented with each subsequenct connection
	connection=false;
	operatingmode=0;		//Normal mode
    cable_capacitance=CABLE_CAPACITANCE;
	state=INIT_STATE_SP1ML;		//The initial "parking" state
	latestdatasample.channelmask=0x00FF;//Initialise with all the ECG channels enabled
	for(qint8 n=0; n<8; n++)
		latestdatasample.rawquality[n]=LEAD_OFF_MIN_QUALITY/2.0;//Init the quality filter with close to the lowest quality (this inits the filtered value) 
}		

void connectionManager::connectionStateMachine() {
    static QByteArray transmitbuffer,receivebuffer;//Used to hold commands sent to the serial port
    static int internaltimeout;
	static double internaltimestamp;
	static double timelastpacket;		//The time we connected (is part of the class). This is the time of the last packet
	double secondsSinceEpoch=QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
	static quint8 bindex[8]={};		//Used to manage sample history buffer for I,Q filtering
	static qint32 qualityfilter_demod[8][10][2]={};//Used for I,Q filter history
	static quint32 qualityfilter[8]={};	//Used for low pass magnitude filtering of 62.5Hz bandpass output
	QByteArray readpacket;
    switch(state) {
	case INIT_STATE_SP1ML:			//Init state for SP1ML module configuration
		if(!connection)
			break;
		transmitbuffer.clear();
		transmitbuffer.append(PACKET_HEAD"10"); //This is the ping packet
		emit emptyRxBuffer();
		emit sendData(&transmitbuffer);	//Pass a pointer to the transmit data to the send function (goes via signals and slots)
		receivebuffer.clear();		//Will be used to receive the data later
		internaltimestamp=secondsSinceEpoch+0.5;//Timeout for a response from the remote SP1ML device (pings at 2hz) 
		//If we aren't connected we don't do anything. This allows the state machine to be "parked" here
		state++;
		break;
	case INIT_STATE_SP1ML+1:
		transmitbuffer.clear();
		emit readAsString(&receivebuffer);
		if(dataDepacket(&receivebuffer, 0, &readpacket)) {//If something was read (any following packets are left in the buffer)
			latestdatasample.device_scale_factor=ADC_SCALE_FACTOR/(float)readpacket[2];//The first argument byte (after address and sequence) is gain
			connectedDeviceName=readpacket.right(readpacket.size()-3);//Process the device name (first byte is the gain setting)
			connectedDeviceName.insert(0,QByteArray("ISM:"));//The name that is actually displayed in the GUI is proceeded by "ISM:"
			emit setDeviceDescriptor(&connectedDeviceName);
		}
		else {
			if(secondsSinceEpoch>internaltimestamp) {	//Abort on timeout
				receivebuffer.clear();//Wipe any data and start again
				state=INIT_STATE_SP1ML;
			}
			break;
		}
		SP1MLnetworkaddress++;		//This is the connection address, it increments with each new connection that is made
		if(SP1MLnetworkaddress>HIGHEST_ADDRESS)
			SP1MLnetworkaddress=LOWEST_ADDRESS;//Loop back around
		transmitbuffer.append(PACKET_HEAD"2");//This is the pingback with the address assignment
		transmitbuffer.append(QString::number(SP1MLnetworkaddress,16));//Add on the hex character argument
		emit sendData(&transmitbuffer);
		state++;
		break;
	case INIT_STATE_SP1ML+2:
		emit setRTS(true);		//Enter command mode before remapping the destination address on the usb dongle
		state++;
		internaltimeout=2;
		break;				//Need to wait for a single iteration for the setting to take effect
	case INIT_STATE_SP1ML+3:		//The device is now in command mode, send a config string to force it to send to the correct network address
		transmitbuffer.clear();		//Clear this before setting it to the config string
		receivebuffer.clear();
		emit emptyRxBuffer();		//Dischard all
		transmitbuffer.append("ATS16=0x0");//Hex format argument. This is the destination, i.e. the device we are connection to (base station is 0x00)
		transmitbuffer.append(QString::number(SP1MLnetworkaddress,16));
		transmitbuffer.append("\n\r");
		emit sendData(&transmitbuffer);	//Send the command to force an address change	
		state++;
		break;
	case INIT_STATE_SP1ML+4:
		emit readAsString(&receivebuffer);
		if(receivebuffer.indexOf("OK")!=-1) {//Got response
			receivebuffer.clear();
			transmitbuffer.clear();
			transmitbuffer.append("ATO\n\r");//Exit the command mode
			emit sendData(&transmitbuffer);
			emit setRTS(false);		//Exit command mode (also needs command to be sent)
			state++;
		}
        	else if(internaltimeout--) {
        		state=INIT_STATE_SP1ML+3;
        	}
		else {
			emit setRTS(false);	//Try cycling the RTS pin level
			state=INIT_STATE_SP1ML+2;
		}
		break;
	case INIT_STATE_SP1ML+5:
		emit readAsString(&receivebuffer);
		if(receivebuffer.indexOf("OK")!=-1) {//Got response (a response it set before leaving command mode)
			receivebuffer.clear();
			transmitbuffer.clear();
			internaltimeout=0;	//This is used for request management once the device enters normal mode
			goto ENTRY_STATE_;	//We continue into the data loop state (todo impliment support for multiple devices?)
		}
        	else if(internaltimeout--) {
        		state=INIT_STATE_SP1ML+3;
        	}
		else {
			emit setRTS(false);	//Try cycling the RTS pin level
			state=INIT_STATE_SP1ML+2;
		}
		break;
	case ENTRY_STATE:			//The RN-42 bluetooth enters here. Whilst there is no need for a name, the gain needs sending. Sequence used instead
		ENTRY_STATE_:
		connectiontime=secondsSinceEpoch;
		timelastpacket=secondsSinceEpoch;
		lastsequencenumber=0;
		if(connectiontype==0)		//This is reset to zero if we have a RN-42/bluetooth/raw serial device connected
			latestdatasample.device_scale_factor=0;//If the device type is zero and the scale factor also zero in the following routine, use sequence no.
		currentestimateddevicetime=secondsSinceEpoch;//All these are reset to defaults
		state=ENTRY_STATE+1;		//Continue straight into the next state once these static variables have been set
	case ENTRY_STATE+1:
		if(secondsSinceEpoch>internaltimestamp) {
			internaltimestamp=secondsSinceEpoch+INTERNAL_REQUEST_INTERVAL;
			int requests=(int)(DATA_RATE*INTERNAL_REQUEST_INTERVAL*DATA_OVER_REQUEST);//Number of samples to request
			transmitbuffer.clear();
			if(!operatingmode)	//Normal auxillary channel mode
				transmitbuffer.append(PACKET_HEAD"3");//Message type 3 is a request
			else
				transmitbuffer.append(PACKET_HEAD"4");//Message type 4 mode sends raw IMU data
			transmitbuffer.append(QString::asprintf("%02x",requests));//Zero padded hex
			transmitbuffer.append(QString::asprintf("%04x",latestdatasample.channelmask));//16 bit int as 4 hex characters
			emit sendData(&transmitbuffer);
		}
		internaltimeout--;
		emit readAsString(&receivebuffer);
		QByteArray readpacket;
		qint16 count=0;
		for(qint16 n=0;n<16;n++)
			count+=(latestdatasample.channelmask&(1<<n))?1:0;
		if(dataDepacket(&receivebuffer, count*2, &readpacket)) {//A packet was found, process it (count is number of channels, add two byte overhead)
			if(!connectiontype && !latestdatasample.device_scale_factor)
				latestdatasample.device_scale_factor=ADC_SCALE_FACTOR/(float)readpacket[1];//Populate this using the first sequence number
			double timegap=secondsSinceEpoch-timelastpacket;//This should be rounded to the nearest GUI refresh interval, also serial delay to consider
			int gap=((int)(readpacket[1])-lastsequencenumber)%256;//Normally this should be 1, but if packets are missed it could be greater
			timelastpacket=secondsSinceEpoch;
			lastsequencenumber=(int)(readpacket[1]);
			int gapt=(int)(timegap*DATA_RATE);	//This is how many samples should have been dropped according to the time gap
			gapt-=(gapt)%256; 			//This takes off the possible correction range that can be achieved using the sequence number
			gap+=gapt;	
			currentestimateddevicetime+=(float)gap/DATA_RATE;//Add to the current estimated device time
			//Load the data from the packet string into the sample history struct
			qint16 tempbuf;				//Used as working buffer for ECG samples
			qint16 indx=2;				//Starting index of the data (there is a network address and sequence number first)
			qint8 historybufferr=0;
			for(qint16 n=0; n<16; n++) {		//There are a maximum of 16 channels supported by the protocol
				if((indx+1)>=readpacket.size()) {
					historybufferr=1;	//Mark this as an error
					//qDebug() << endl << readpacket.size() << indx;
					break;			//This should not happen
				}
				if(latestdatasample.channelmask&(1<<n)) {//The current channel is enabled
					if(n<8) {		//If this is an ECG channel, run the filters to strip the ECG from the lead-off signal
						tempbuf=(readpacket[indx])||((qint16)readpacket[indx+1]<<8);//The current sample
						if((datasamplehist.indices[n][1]-(int)(readpacket[1]))%256==2) {//Normal packet spacing, everything is ok
							bindex[n]++;
							latestdatasample.samples[n]=(tempbuf+datasamplehist.samples[n][1])>>1;//comb block filt at 62.5hz
							if(!(bindex[n]&0x03)) {//Generate an I and a Q value from the last 4 samples (they might not be sequential)
								qint32 I=0,Q=0;
								for(quint8 ind=0; ind<4; ind++) {
									quint8 count=datasamplehist.indices[n][ind];
									qint8 I_=(qint8)(count&0x02)-1;
									qint8 Q_=1-(qint8)((count&0x02)^((count&0x01)<<1));//These are generated using indices
									I+=I_*datasamplehist.samples[n][ind];
									Q+=Q_*datasamplehist.samples[n][ind];//I and Q are accumulated, then put into history buffer
								}
								qualityfilter_demod[n][bindex[n]>>2][0]=I;
								qualityfilter_demod[n][bindex[n]>>2][1]=Q;//Load into the history buffer
							}
							if(bindex[n]>=36) {
								qint32 qI=0,qQ=0;
								for(quint8 m=0; m<10; m++) {
									qI+=qualityfilter_demod[n][m][0];
									qQ+=qualityfilter_demod[n][m][1];
								}
								qI/=10;	// Find the average amplitude
								qQ/=10;
								quint64 q_=(qI*qI)+(qQ*qQ);
								q_=sqrt(q_);//To prevent 32 bit integer overflow
								qualityfilter[n]+=((int32_t)q_-(int32_t)qualityfilter[n])>>5;//Secondary low pass filtering
							}
							if(bindex[n]>=36)
								bindex[n]=0xff;	//so it rolls over
							float diff=(float)qualityfilter[n];//Use float type to process the data using conversion factor
							diff*=6.10e-8*latestdatasample.device_scale_factor;// Convert to volts (peak to peak)
							diff=240.0*diff+exp(110000.0*diff*diff*diff*pow((cable_capacitance+200.0)/400.0,3));//Brute force match-> Mohm
							diff*=1000.0;// Now in units of k ohm, matching equation models board as ~200pF
							diff-=BOARD_IMPEDANCE;// This impedance is present already on the board
							latestdatasample.rawquality[n]=latestdatasample.rawquality[n]*(1.0-GAIN_COEFFICIENT)+diff*GAIN_COEFFICIENT;
							latestdatasample.quality[n]=CONVERT_GAIN*logf(latestdatasample.rawquality[n])+CONVERT_OFFSET;//display value
						}
						else
							historybufferr=1;//There was a problem filtering the data due to missing packets
						for(qint8 h=3; h; h++) {//Loop through the sample and index delay buffer
							datasamplehist.samples[n][h]=datasamplehist.samples[n][h-1];//Shift the history buffer
							datasamplehist.indices[n][h]=datasamplehist.indices[n][h-1];//Shift the indices buffer
						}
						datasamplehist.samples[n][0]=tempbuf;
						datasamplehist.indices[n][0]=(int)(readpacket[1]);
					}
					else			//Load the sample from the byte array little-endian
						latestdatasample.samples[n]=(readpacket[indx])||(readpacket[indx+1]<<8);
					indx+=2;
				}
			}				
			qDebug() << endl << "Received" << historybufferr;
			if(!historybufferr) {			//If we have good samples
				qDebug() << endl << "Received";
				emit setDataToGraph(&latestdatasample);	//This is connected to the plotter
			}
		}
		else if(secondsSinceEpoch>(timelastpacket+((secondsSinceEpoch<(connectiontime+TIMEOUT_TRANSITION))?TIMEOUT_ONE:TIMEOUT_TWO))) {
			if(connectiontype==1)			//causes the connection state to be reset so that nopefully a new connection can be made
				state=INIT_STATE_SP1ML;		//A long interval with nothing found (about 2 minutes during runtime, and 10 seconds at boot)
			else if(connectiontype==0)
				state=ENTRY_STATE;		//A simple re-entry, no need to establish a new connection
		}					
	}
}

// connect(sender, &QObject::destroyed, [=](){ this->m_objects.remove(sender); });
connectionManager::~connectionManager()
{
}

//This returns true if the a valid packet is found. The argument n is a helper to give an idea of how big the payload should be (excludes net & sync), set 0 to ignore
//Extracts a single payload starting from the left
bool connectionManager::dataDepacket(QByteArray* data, int n, QByteArray* data_output) {//Correctly packet format HEAD, network id, sequence number, followed by data
	int s=0;
	s=data->indexOf(HEAD,0);//Find the first packet header
	if(s>=0)
		*data=data->right(data->size()-s);//Trim off the start of the string, i.e. everything up to the packet header. This should avoid sync issues
	else
		return false;	//no header found
	if(n && ( data->size()<(n+4) || ( (data->size()>(n+4))&&(data->at(n+4)!=HEAD) ) )) {//Three bytes of header plus a byte for the COBS initialisation
		if(data->indexOf(HEAD,1)>40)//Only abort if there are no other packets within 40 bytes. Allows packet length to change at runtime (between requests)
			return false;	//There cannot be a matching packet contained within. 
	}
	if(data->size()<4)
		return false;	//Nothing useful in the buffer (yet)
	int skip;		//The initial skip value (candidate, as we don't know if we have a complete packet yet - service name reply is of unknown length)
	int index=data->indexOf(HEAD,1)-1;//The next index
	int siz;
	if(index<0)		//Nothing found
		index=data->size()-1;//Go to the end of the string
	siz=index;
	*data_output=data->mid(0,siz);//Copy to the output, only the output is manipulated here
	//qDebug() << endl << (((*data_output))).toHex();
	for(skip=1;skip<siz;skip++) {//Go through XORING, exclude the head byte, include the tail
		((*data_output)[skip])=((*data_output)[skip])^HEAD;//Payload is XOR with head
	}
	//qDebug() << endl << "Received";
	//qDebug() << endl << (((*data_output))).toHex();
	index--;
	while(index) {		//Start at the end of the packet and work in reverse, then remove the last byte, as its redundant, and return the payload
		skip=(*data_output)[index];//Load the skip value
		//qDebug() << endl << "Skip" << skip << index;
		if(!skip) {
			data_output->clear();
			qDebug() << endl << "Skip error" << index << (*data_output)[0] << (*data_output)[1] <<  (*data_output)[2] <<  (*data_output)[3];
			return false;
		}
		if(index>0)
			(*data_output)[index]=0x00;//Work backwards replacing bytes
		index-=skip;
		if( (index==0 && data->at(index)!=HEAD) || index<0 ) { //Neither of these two things should happen, if they do we aren't in a valid packet, so abort
			data_output->clear();
			qDebug() << endl << "Head error" << index << data_output->at(index);
			return false;
		}
	}//Only one packet is extracted from the start
	*data_output=data_output->mid(1,siz-1);//Trim to get only the packet contents
	*data=data->right(data->size()-siz);//Remove the processed packet from the input (allows more packets to be grabbed from later on)
	return true;
}

/*
void connectionManager::setDeviceDescriptor(QByteArray* name) {
	emit setDeviceDescriptor(name); 
}*/

void connectionManager::newConnection(int type) {
	connection=true;
	connectiontype=type;
	emit setRTS(false);	//RTS to logic 1==normal mode. Init as normal mode (SP1ML will init from its EEPROM)
	if(type==1)//^ It is not clear why the levels need to be as above (seems to be inverted?)
		state=INIT_STATE_SP1ML;//Reset the state as appropriate
	else {
		state=INIT_STATE_RN42;
		connectedDeviceName=QByteArray("Bluetooth or serial device");//Process the device name (this is used to update the GUI)
		emit setDeviceDescriptor(&connectedDeviceName);
	}
}

void connectionManager::lostConnection(void) {
	connection=false;	//This variable lets us know if we are connected
	state=INIT_STATE_SP1ML;	//Go to the initial parking state
	connectedDeviceName=QByteArray("");//This is used to update the GUI, set as no device
	emit setDeviceDescriptor(&connectedDeviceName);
}

void connectionManager::setChannelMask(quint16 newmask) {//This is a slot used to configure which channels will be plotted
	latestdatasample.channelmask=newmask;
}

void connectionManager::setauxmask(quint8 mask) {
	latestdatasample.channelmask&=0x00FF;//wipe upper 8 bits
	latestdatasample.channelmask|=(mask<<8);//set them as appropriate
}

void connectionManager::setauxchanmode(bool mode) {
	operatingmode=mode?1:0;	//Normal mode is mode 0 (secondary mode sends raw IMU data)
	//TODO add an emitted signal to the secondary graph(s) (once/if they are implimented) here to prevent plotting of inappropriate data
}

