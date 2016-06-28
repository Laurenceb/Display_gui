#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <QObject>
#include <QByteArray>
#include <QString>
#include <QQueue>
#include <QtSerialPort/QSerialPort>

class SerialPort : public QObject
{
	Q_OBJECT
	public:
		explicit SerialPort(QObject *parent = 0);
		~SerialPort();
		void setName(QString const& name);
		QString const name();
		bool open();
		bool isOpen() const;
		void close();
		void sendMessage(QString const& message);
		void writeBytes(QByteArray * byteArray);
		void processWritten();
		void setRTS(bool RTS);
		void setDTR(bool DTR);
		QQueue<unsigned char> queue_;//Used for data output
		signals:
		 void portOpened();
		 void portClosed();
		 void messageReceived(QString);
		 void error(unsigned error_code);
		 void written();
	public slots:
	private slots:
		void onDataAvailable();
		void onError(QSerialPort::SerialPortError serialPortError);
	private:
		QSerialPort *port;
};
#endif // SERIALPORT_H
