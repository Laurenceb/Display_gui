#ifndef CONNECTIONMANAGE_H
#define CONNECTIONMANAGE_H
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QtGlobal>
#include <QObject>

#include "portselectdialog.h"

#define PACKET_HEAD "MJ" /*This is the two byte header used to start all command bytes sent to devices over both SP1ML and bluetooth*/
#define HEAD 0x0A	/*This is the packet header used for packets sent back from the device over both networks*/

#define LOWEST_ADDRESS 2
#define HIGHEST_ADDRESS 15 /*Address 0 is taken by any potential misconfigured devices, device 1 is for unconnected, then the range from 2 to f in hex is active*/

#define INIT_STATE_SP1ML 0 /*This is used to the SP1ML network manager state machine, but not for the bluetooth which is initialised at a higher state level*/
#define ENTRY_STATE 6
#define INIT_STATE_RN42 ENTRY_STATE;

#define DATA_RATE 250.0
#define INTERNAL_REQUEST_INTERVAL 0.2
#define DATA_OVER_REQUEST 3 /*We always request three times as much data as we need. This allows two transmitted packets in a row to be lost*/
//^ note that the product of these three numbers should be less than or equal to 255, so it can fit into a single unsigned byte argument

#define TIMEOUT_ONE 3.0
#define TIMEOUT_TWO 60.0
#define TIMEOUT_TRANSITION 20.0 /*20 seconds after first connection, the connection timeout interval is changed from first period to second period*/

//Device specific config
#define ADC_SCALE_FACTOR 9.7656e-4 /*Converts the data received into millivolt units (the PGA gain setting also needs to be applied)*/

//Lead-off signal to electrode impedance conversion coefficients (impedance uses kohm units)
#define LEAD_OFF_R 10000.0	/*10M resistors used on the device*/
#define BOARD_IMPEDANCE 30.0	/*30k ohm impedance from the internal filtering onboard the device*/
#define FILTER_GAIN_LEAD (0.3*0.7298*0.125)/*Sync filter at spr/4 is 0.7298, then the IIR filter onboard the device approx 0.3 (3s.f.) gain at 62.5Hz, then 1/8 comb*/
#define CABLE_CAPACITANCE 550;	/*this is the cable capacitance for 10 lead ECG cables*/

#define LEAD_OFF_MAX_QUALITY 10.0	/*10k ohms or lower is best quality, over 1Mohm gives lowest quality. See Ti app note */
#define LEAD_OFF_MIN_QUALITY 1000.0

#define LEAD_QUALITY_TIME_COEFFICIENT 2.5/*This is measured in seconds, and is the time constant used to rolling average filter the lead-off signal*/

//macros derived from the above
#define GAIN_COEFFICIENT 1.0/(DATA_RATE*LEAD_QUALITY_TIME_COEFFICIENT) /*for filtering, note below assumes 3.3v VCC*/
#define CONVERSION_FACTOR (5000.0/(1650.0*FILTER_GAIN_LEAD*latestdatasample.device_scale_factor))/*converts comb filter output to est elect. resistance in k ohms*/ 
#define CONVERT_GAIN (1.0/(logf(LEAD_OFF_MAX_QUALITY)-logf(LEAD_OFF_MIN_QUALITY))) /* out=CONVERT_GAIN*logf(in)+CONVERT_OFFSET*/
#define CONVERT_OFFSET (-CONVERT_GAIN*logf(LEAD_OFF_MIN_QUALITY))	/*The scaling from estimated contact impedance to contact integrity metric uses*/

typedef struct {
	double sampletime;
	quint16 channelmask;//Mask holding the currently active channels. Only active channels are transmitted, so this is useful for unpacking correctly
	qint16 samples[16];//Up to 16 channels of data are supported by the protocol
	float quality[8];//This is a variable in the 0 to 1.0 range, with 1.0 being max quality. This is what is displayed in the GUI 
	float rawquality[8];//This is the raw quality index, the estmated electrode contact inpedance in units of ohms
	float device_scale_factor;//This is the scale factor used to convert the signal into millivolts
} datasample_t;		//This type is used to pass back data from the connectionManager to the graph

typedef struct {
	qint16 samples[8][4];
	qint16 indices[8][4];//4 samples used for I,Q extraction
} datasample_buff_t;	//This type is used to buffer the received data from GUI side filtering (at present it is used to split off the lead-off signal)

class connectionManager: public QObject 
{
	Q_OBJECT
	public:
		explicit connectionManager(QObject *parent = 0,PortSelectDialog* used_port_=0);
		~connectionManager();
		void connectionStateMachine(void);
		QByteArray connectedDeviceName;
		datasample_t latestdatasample;
		datasample_buff_t datasamplehist;		//Historical sample buffer, used for filtering
		signals:
		 void setDTR(bool);
		 void setRTS(bool);
		 void setDataToGraph(datasample_t*);		//This is used to append a new data sample to the graph. Note magic numbers are not processed here
		 void setDeviceDescriptor(QByteArray*);		//Used to set the device name to the GUI
		 void sendData(QByteArray*);
		 void readAsString(QByteArray* data);
		 void emptyRxBuffer(void);
	public slots:
		 void setChannelMask(quint16 newmask);		//This is used to set the enabled channels, logic 1 enables a channel
		 void newConnection(int type);
		 void lostConnection(void);
		 void setauxmask(quint8 mask);
		 void setauxchanmode(bool mode);
	private:
		bool dataDepacket(QByteArray* data, int n, QByteArray* data_output);
		PortSelectDialog* used_port;
		bool connection;
		int state;
		int SP1MLnetworkaddress;
		double connectiontime;//Time at which the current connection was first made, this is used for managing timestamps on the graph
		double currentestimateddevicetime;//This is an estimated time syncronised to the device clock, used to accomodate clock drift between device and PC
		int connectiontype;//The type of connection, type 0 is a rfcomm or raw serial connection, type 1 is an SP1ML, higher numbers could be used in future
		int lastsequencenumber;//Used to track any missed packets and attempt to correct the time accordingly
		quint8 operatingmode;
		quint16 cable_capacitance=CABLE_CAPACITANCE;//Used to pass the capacitance value of the connected cable to the impedance estimator
};
#endif
