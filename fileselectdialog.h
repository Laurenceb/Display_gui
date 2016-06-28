#ifndef FILESELECT_H
#define FILESELECT_H
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QtGlobal>
#include <QLineEdit>
#include <QObject>
#include <QFileInfo>
#include <QTextStream>
#include "connectionmanager.h"

class FileSelectDialog : public QDialog 
{
	Q_OBJECT
	public:
		explicit FileSelectDialog(const QFileInfo &fileInfo, QWidget *parent = 0);
		~FileSelectDialog();
//		signals:

	public slots:
		void directorysearcher(void);
		void filecontroller(void);
		void fileinfoupdate(void);
		void adddatatofile(datasample_t* data);//Called via the connection manager when file storage is enabled. It saves data to file, key is the timestamp
	private:
		QLineEdit* file_path;
		QLineEdit* file_name;
		QTimer filetimer;	//Used to update the file information text labels
		QLineEdit* filetimelabel;
		QLineEdit* filesizelabel;
		QPushButton* startstop;
		char filedelimiter;	/*System filepath delimited, either / or \   */
		quint8 fileopen;	//True if the file is open 
		QFile fil;		//The log file
		quint8 errorcode;	//Can be used for error handling, at the moment it makes the file entry background colour go red for a few seconds on error
		QTextStream outStream;
};

#endif
