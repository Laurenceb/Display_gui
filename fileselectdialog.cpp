#include <QDialogButtonBox>
#include <QDebug>
#include <QString>
#include <QtWidgets>
#include <QStatusBar>
#include <QDateTime>
#include <QtSerialPort/QSerialPortInfo>
#include "fileselectdialog.h"
#include "connectionmanager.h"


FileSelectDialog::FileSelectDialog(const QFileInfo &fileInfo, QWidget *parent) :
QDialog(parent)
{
	QVBoxLayout *mainlayout = new QVBoxLayout(this);
	QString rootpath=fileInfo.absoluteFilePath();
	QString rootname="";
	bool path_is_directory=true;		//Path is a directory
	if(rootpath.indexOf('.')>0)
		path_is_directory=false;	//Found a . in the path, so it cannot be a directory
	qint8 ind=rootpath.lastIndexOf('/');	//extract the root path
	filedelimiter='/';
	if(ind<0) {
		ind=rootpath.lastIndexOf('\\');	//dependant on system type
		filedelimiter='\\';
	}
	if(!path_is_directory) {		//remove any filename
		rootname=rootpath.right(rootpath.size()-ind-1);
		rootpath=rootpath.left(ind);
	}
	//First the explanatory label (at the top), followed by text entry and two horizontal buttons
	QLabel *toplabel = new QLabel(tr("Logging path:"));//Text at the top of the tab panel
	file_path = new QLineEdit(rootpath);	//The file name entry box (this is editable, but can also be set via the directory browser)
	QLabel *toplabel_ = new QLabel(tr("Logfile name:"));
	if(path_is_directory) {
		QString nstr=QDateTime::currentDateTime().toString("dd-MM-yy-hh-mm-ss");
		file_name = new QLineEdit(nstr.append(".csv"));
	}
	else
		file_name = new QLineEdit(rootname);
	QHBoxLayout *hLayout = new QHBoxLayout();//Goes below the entry box and contains two buttons, the directory browse, and the Start/Stop button
	QPushButton* directorybutton = new QPushButton("Select directory");
	startstop = new QPushButton("Start");
	hLayout->addWidget(directorybutton);
	hLayout->addWidget(startstop);
	//Now the file information inside a sunken frame
	QFrame *status_frame = new QFrame();
	status_frame->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	QVBoxLayout *slayout = new QVBoxLayout(status_frame);//layout for the status lables, a vertical series of labels and line edit widgets
	slayout->setContentsMargins(4, 4, 4, 4);//Small margins
	QLabel *tstat = new QLabel(tr("File last modified:"),status_frame);//First is the file modification timestamp
	QString mod="";
	if(!path_is_directory)			//Only populate if the path is not a directory
		mod=fileInfo.lastModified().toString();
	filetimelabel = new QLineEdit(mod);	//Initially fill with whatever was passed as the file
	filetimelabel->setDisabled(1);		//Grey this out, it is only an indicator
	QLabel *tsize = new QLabel(tr("File size:"),status_frame);//Second: the file size
	qlonglong size = fileInfo.size()/1024;
	QString siz="";
	if(mod.size())
		siz=tr("%1 K").arg(size);
	filesizelabel = new QLineEdit(siz);	//Size is in k
	filesizelabel->setDisabled(1);		//Grey this out, it is only an indicator
	slayout->addWidget(tstat);
	slayout->addWidget(filetimelabel);
	slayout->addWidget(tsize);
	slayout->addWidget(filesizelabel);	//Add all the widgets inside the frame
	mainlayout->addWidget(toplabel);	//Now add all the stuff to the main widget
	mainlayout->addWidget(file_path);
	mainlayout->addWidget(toplabel_);
	mainlayout->addWidget(file_name);
	mainlayout->addLayout(hLayout);
	mainlayout->addWidget(status_frame);
	connect(directorybutton, SIGNAL (released()), this, SLOT(directorysearcher()));
	connect(startstop, SIGNAL (released()), this, SLOT(filecontroller()));
	connect(&filetimer, SIGNAL(timeout()), this, SLOT(fileinfoupdate()));//The file info update timer
	fileopen=0;
	filetimer.start(2000); // Interval 0 means to refresh as fast as possible, set 2000ms to get 0.5hz update rate
}

void FileSelectDialog::directorysearcher(void) {
	QString root=file_path->text();//grab the path string from the entry box
	QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),root, QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
	file_path->setText(dir);		//Set to the entry box
}

void FileSelectDialog::filecontroller(void) {
	if(fileopen) {
		outStream << "Datalogging stopped";
		fil.close();
		fileopen=0;
		startstop->setText("Start");
	}
	else {
		QString filnam=file_name->text();
		quint8 in=filnam.lastIndexOf('.');
		QPalette p = file_name->palette();
		errorcode=1;
		if(in>0) {//There is a file extension (it's not essential to enter one)
			if(filnam.right(filnam.size()-(in+1))!="csv") {
				file_name->setText(filnam.left(in+1));//Wipe off the extension
				p.setColor(QPalette::Base, Qt::red);
				file_name->setPalette(p);//Background to red
				return;//This isnt possible, wipe and return
			}
		}
		else 	//Need to add a .csv file extention
			filnam.append(".csv");
		QString p_=file_path->text();
		p_.append(filedelimiter);
		filnam=p_.append(file_name->text());//The name to be opened
		//Open the file here
		fil.setFileName(filnam);
		if(!fil.open(QIODevice::ReadWrite|QIODevice::Append|QIODevice::Text)) {
			p.setColor(QPalette::Base, Qt::red);
			file_name->setPalette(p);//Background to red
			return;
		}
		p.setColor(QPalette::Base, Qt::green);//green is code for opened ok
		file_name->setPalette(p);
		QString header("Datalogging started: ");
		header.append(QDateTime::currentDateTime().toString(Qt::ISODate));//Add a timestamp to the top of the file
		outStream.setDevice(&fil);
		outStream << header << endl;
		fileopen=1;//Only mark this if we are all ok
		startstop->setText("Stop");
	}
}

void FileSelectDialog::fileinfoupdate(void) {
	QString mod="";
	QString fullpath=file_path->text();
	fullpath.append(filedelimiter);
	fullpath.append(file_name->text());
	QFileInfo fileInfo(fullpath);
	if(file_name->text().lastIndexOf('.')>0)//Only populate if the name is set
		mod=fileInfo.lastModified().toString();
	filetimelabel->setText(mod);//Initially fill with whatever was passed as the file
	qlonglong size = fileInfo.size()/1024;
	QString siz="";
	if(mod.size())
		siz=tr("%1 K").arg(size);
	filesizelabel->setText(siz);//Size is in k
	//Handle the background colour
	if(!(errorcode--)) {
		QPalette p = file_name->palette();
		p.setColor(QPalette::Base, Qt::white);
		file_name->setPalette(p);//Background to white
	}
}

void FileSelectDialog::adddatatofile(datasample_t* data) {
	if(fileopen) {			//Only do anything if there is a file open at the time
		QDateTime a;
		a.setMSecsSinceEpoch(qint64(data->sampletime*1000.0));
		outStream << a.toString(Qt::ISODate)<< (QString::number(fmod(data->sampletime,1.0), 'f', 3 )).right(4) << ',';//Time to nearest millisecond
		for(quint8 n=0; n<16; n++) {//Loop through all 16 channels
			if(data->channelmask & (1<<n))
				outStream << (data->samples)[n];
			if(n<15)
				outStream << ',';
		}
		outStream << endl;
	}
}

FileSelectDialog::~FileSelectDialog()
{
}
