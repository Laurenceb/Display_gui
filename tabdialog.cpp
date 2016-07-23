/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QStatusBar>

#include "tabdialog.h"
#include "graph.h"
#include "portselectdialog.h"
#include "fileselectdialog.h"

TabDialog::TabDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent)
{
    QFileInfo fileInfo(fileName);

    //buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
    //                                 | QDialogButtonBox::Cancel);

    //connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    //connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    statusBar_ = new QStatusBar;

    tabWidget = new QTabWidget;
    PortSelectDialog *connectionDialog=new PortSelectDialog();
    FileSelectDialog *fileDialog=new FileSelectDialog(fileInfo);
    tabWidget->addTab(new GraphTab(statusBar_,connectionDialog, fileDialog), tr("Graphs"));//graph passed a pointer to the PortSelectDialog, to handle data in/out
    tabWidget->addTab(connectionDialog, tr("Connection"));
    tabWidget->addTab(fileDialog, tr("Local datalogging"));

    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(statusBar_);
    statusBar_->showMessage("Equine ECG, no device connected");

    connect(enablebuttonsgroup, SIGNAL(buttonClicked(int)), this, SLOT(onButSelected(int)));
    connect(enablebuttonsgroup, SIGNAL(buttonReleased(int)), this, SLOT(onButSelected(int)));//Connect the ECG enable/disable functions to the button click signals
    connect(this, SIGNAL(setecgmask_(quint8), connectionDialog->, SLOT(quint8));

    resize(700,400);
    setMinimumSize(500, 350);
    setLayout(mainLayout);
    setWindowTitle(tr("Equine ECG"));
}

TabDialog::onButSelected(int unused){
	quint8 mask=0;
	for(quint8 m=0; m<8; m++) {
		if(enablebuttons[n]->isChecked())
			mask|=(1<<m);	
	}
	emit setecgmask_(mask);	//The mask is set in connectionmanager 
}


//This is passed a pointer to the statusbar and the port selection dialog
GraphTab::GraphTab(QStatusBar* statusBar__, PortSelectDialog *connectionDialog, FileSelectDialog *fileDialog, QWidget *parent)
    : QWidget(parent)
{
    statusBar_=statusBar__;
    Graph *G= new Graph;
    QVBoxLayout *vLayout = new QVBoxLayout;
    QHBoxLayout *hLayout = new QHBoxLayout;//Goes below the plot, and is used for the 8 channel enable buttons
    QCustomPlot * plot_temp = new QCustomPlot;
    vLayout->addWidget(plot_temp);
    plot_temp->setObjectName(QStringLiteral("plot_temp"));
    //plot_temp->setGeometry(400, 250, 700, 500);//x,y location of window, and the window size at initialisation? TODO check functionality of this
    QFont newFont("Courier", 8, QFont::Bold, true);
    plot_temp->setFont(newFont);
    //Add eight buttons to enable the 8 plot lines. The item in this horizontal set is used to set the graph display time in seconds (-1 sets demo mode) 
    enablebuttonsgroup= new QButtonGroup();
    QStringList ecg_telem_names;
    ecg_telem_names << "RA" << "LA" << "LL" << "C1" << "C2" << "C3" << "C4" << "C5";
    //Configure the stylesheet
    QString stylestring_base;		//Dynamic stylesheets for the buttons
    stylestring_base="QPushButton{font-weight: bold; color: rgba(100,105,120,1.0); background-color: rgba(%1, 0.25); border-style: outset; border-width:2px; border-radius: 6px; border-color: beige} QPushButton:checked{background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 rgba(%1, 1.0), stop: %2 rgba(%1, 1.0), stop: %3 white, stop: 1.0 white); border-color: black}";
    QStringList colorlist;
    QStringList styletemplate;//Colour code buttons according to their channel
    colorlist << "240, 0, 0" << "240, 170, 20" << "0, 240, 0" << "240, 0, 0" << "200, 150, 30" << "20, 150, 30" << "139, 69, 13" << "45, 42, 50";
    foreach(const QString &str, colorlist) {
	QString tm=stylestring_base.arg(str);//A list of strings
	tm.replace("%2","%1");		//Swap %3 for %1 and %4 for %2
	tm.replace("%3","%2");
	styletemplate << tm;		//A string list for the styles
    }
    G->style_populator=styletemplate;
    //Loop through, populating all the buttons. Use the standard titles rather than raw mode titles
    for(quint8 n=0; n<8; n++) {
	enablebuttons[n] = new QPushButton(ecg_telem_names.at(n));
	enablebuttons[n]->setCheckable(true);//the buttons can be turned on/off
	enablebuttons[n]->setStyleSheet(styletemplate[n].arg("0.99","1.00"));
	enablebuttons[n]->setChecked(true);//All channels default to enabled
	enablebuttons[n]->setEnabled(false);//But they are all greyed out
	hLayout->addWidget(enablebuttons[n]);//Add the button to the horizontal layout
	enablebuttonsgroup->addButton(enablebuttons[n]);
	enablebuttonsgroup->setId(enablebuttons[n], n);
    }
    enablebuttonsgroup->setExclusive(false);
    G->channelenablebuttons=enablebuttons;
    //add some read only explanatory text
    QLabel *echoLabel = new QLabel(tr("Hold time (s):"));
    hLayout->addWidget(echoLabel);
    //Add a validated integer input
    G->validatorLineEdit = new QLineEdit;
    G->validatorLineEdit->setMaximumWidth(100);
    G->validatorLineEdit->setValidator(new QDoubleValidator(-60.0,60.0, 2, G->validatorLineEdit));//Enforce a range of +-1minute
    hLayout->addWidget(G->validatorLineEdit);//The line edit goes at the end of the buttons
    //Create the plot
    G->bar_=statusBar_;
    G->setupRealtimeDataDemo(plot_temp,connectionDialog,fileDialog);//There is a horizontal set of buttons across the bottom
    plot_temp->replot();
    vLayout->addLayout(hLayout);
    setLayout(vLayout);
}


/*LoggingTab::LoggingTab(const QFileInfo &fileInfo, QWidget *parent)//TODO: remove filename argument? Or have this as the current path?
    : QWidget(parent)
{
    QLabel *fileNameLabel = new QLabel(tr("Current logfile:"));//Text at the top of the tab panel
    QLineEdit *fileNameEdit = new QLineEdit(fileInfo.fileName());

    QLabel *pathLabel = new QLabel(tr("Path:"));
    QLabel *pathValueLabel = new QLabel(fileInfo.absoluteFilePath());
    pathValueLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    QLabel *sizeLabel = new QLabel(tr("Size:"));
    qlonglong size = fileInfo.size()/1024;
    QLabel *sizeValueLabel = new QLabel(tr("%1 K").arg(size));
    sizeValueLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    QLabel *lastReadLabel = new QLabel(tr("Last Read:"));
    QLabel *lastReadValueLabel = new QLabel(fileInfo.lastRead().toString());
    lastReadValueLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    QLabel *lastModLabel = new QLabel(tr("Last Modified:"));
    QLabel *lastModValueLabel = new QLabel(fileInfo.lastModified().toString());
    lastModValueLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    QVBoxLayout *mainLayout = new QVBoxLayout;	//Vertical layout
    mainLayout->addWidget(fileNameLabel);
    mainLayout->addWidget(fileNameEdit);
    mainLayout->addWidget(pathLabel);
    mainLayout->addWidget(pathValueLabel);
    mainLayout->addWidget(sizeLabel);
    mainLayout->addWidget(sizeValueLabel);
    mainLayout->addWidget(lastReadLabel);
    mainLayout->addWidget(lastReadValueLabel);
    mainLayout->addWidget(lastModLabel);
    mainLayout->addWidget(lastModValueLabel);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
	//TODO setup a timer with callback to update the file info, or have another set of files and classes?
}*/

