#include <QDebug>
#include "serialport.h"
#include "portselectdialog.h"

SerialPort::SerialPort(QObject *parent) :
QObject(parent)
{
	port = new QSerialPort(this);
	connect(port, &QSerialPort::readyRead, this, &SerialPort::onDataAvailable);//There is some received data in the serial port buffer
	connect(port, (void (QSerialPort::*)(QSerialPort::SerialPortError))&QSerialPort::error, this, &SerialPort::onError);
	connect(port, &QSerialPort::bytesWritten, this, &SerialPort::processWritten);//The transmit buffer is now empty
}

// connect(sender, &QObject::destroyed, [=](){ this->m_objects.remove(sender); });
SerialPort::~SerialPort()
{
}

void SerialPort::setName(QString const& name)
{
	port->setPortName(name);
}

QString const SerialPort::name()
{
	return port->portName();
}

bool SerialPort::open()
{
	if(!port->open(QIODevice::ReadWrite))
	return false;
	if (!(port->setBaudRate(QSerialPort::Baud115200)
	&& port->setDataBits(QSerialPort::Data8)
	&& port->setParity(QSerialPort::NoParity)
	&& port->setStopBits(QSerialPort::OneStop)
	&& port->setFlowControl(QSerialPort::NoFlowControl)))
	return false;
	emit portOpened();
	return true;
}

bool SerialPort::isOpen() const
{
	return port->isOpen();
}

void SerialPort::close()
{
	port->close();
	emit portClosed();
}

void SerialPort::sendMessage(QString const& message)
{
	qDebug() << "SerialPort::sendMessage:" << QString(":") + message + "\\n";
	port->write((QString(":") + message + "\n").toLatin1());
}

void SerialPort::onDataAvailable()
{
	// append new data to buffer
	QByteArray arr=port->readAll();
	for(int i=0; i<arr.count(); i++) {//Loop through the character array, placing all received characters into the queue
		queue_.enqueue(arr.at(i));
	}
	//qDebug() << "on_ready_read(): buffer: [" << buffer.data() << "]";//, orig_size:" << orig_buffer_size << ", new size:" << buffer.size() << ", bytes read:" << bytes_read << ", available:" << available;
	// find messages in buffer
	// TODO see boost::asio for framer interfaces
/*	char const *DELIM = "\n"; // TODO \r ?
	char const *MSG_START = ":";
	// TODO large bogus strings w/o DELIM or MSG_START will fill the buffer
	// TODO redo algorithm: discard everything before :
	int index;
	while((index = buffer.indexOf(DELIM, 0)) != -1) {
		QByteArray sub = buffer.left(index);
		buffer.remove(0, index + 1);
		if(sub.size() > 0 && sub.startsWith(MSG_START)) {
			//qDebug() << "found message:" << sub.data() << "buffer: " << buffer.data();
			emit messageReceived(QString(sub.mid(1))); // without : or \n\r
		} else qDebug() << "string discarded";
	}*/
}

void SerialPort::writeBytes(QByteArray * byteArray) {
	port->write(*byteArray);//this is a non blocking write
}

void SerialPort::processWritten() {
	emit written();	//this signal is used by the portselectdialog
}

void SerialPort::setDTR(bool DTR) {
	port->setDataTerminalReady(DTR);
}

void SerialPort::setRTS(bool RTS) {
	port->setRequestToSend(RTS);
}

void SerialPort::onError(QSerialPort::SerialPortError err)
{
	if(err != QSerialPort::NoError) {
		qDebug() << "serial port error:" << err; // when cable is disconnected, emits QSerialPort::ReadError, maybe QSerialPort::WriteError
		emit error((unsigned)err);
		port->clearError();
	}
}













