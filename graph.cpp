/***************************************************************************
**                                                                        **
**  QCustomPlot, an easy to use, modern plotting widget for Qt            **
**  Copyright (C) 2011-2015 Emanuel Eichhammer                            **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Emanuel Eichhammer                                   **
**  Website/Contact: http://www.qcustomplot.com/                          **
**             Date: 22.12.15                                             **
**          Version: 1.3.2                                                **
****************************************************************************/

/************************************************************************************************************
**                                                                                                         **
**  This is the example code for QCustomPlot.                                                              **
**                                                                                                         **
**  It demonstrates basic and some advanced capabilities of the widget. The interesting code is inside     **
**  the "setup(...)Demo" functions of MainWindow.                                                          **
**                                                                                                         **
**  In order to see a demo in action, call the respective "setup(...)Demo" function inside the             **
**  MainWindow constructor. Alternatively you may call setupDemo(i) where i is the index of the demo       **
**  you want (for those, see MainWindow constructor comments). All other functions here are merely a       **
**  way to easily create screenshots of all demos for the website. I.e. a timer is set to successively     **
**  setup all the demos and make a screenshot of the window area and save it in the ./screenshots          **
**  directory.                                                                                             **
**                                                                                                         **
*************************************************************************************************************/

#include "graph.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDesktopWidget>
#include <QScreen>
#include <QMessageBox>
#include <QMetaEnum>
#include <QTimer>
#include <QFont>

#include "connectionmanager.h"

void Graph::setupRealtimeDataDemo(QCustomPlot *customPlot, PortSelectDialog *connectionDialog, FileSelectDialog* fileDialog)
{
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
  QMessageBox::critical(this, "", "You're using Qt < 4.7, the realtime data demo needs functions that are available with Qt 4.7 to work properly");
#endif
  demoName = "Real Time Data Demo";
  
  connection = new connectionManager(this,connectionDialog);//This is used to pass data between the connection tab port and the graph
  connect(connection, SIGNAL(setDataToGraph(datasample_t*)), this, SLOT(addData(datasample_t*)));//This message is passed to write new data to the plot
  //This is used to change the hold time of the graph window (negative hold times give demo mode)
  connect(validatorLineEdit, SIGNAL(returnPressed()), this, SLOT(changeHoldTime()));
  //Used to change the channel mask
  connect(this, SIGNAL(setmask(quint16)), connection, SLOT(setChannelMask(quint16)));

  //Connect the aux channel control signals from the connectionDialog to the connectionmanager
  connect(connectionDialog, SIGNAL(setauxmask(quint8)), connection, SLOT(setauxmask(quint8)));//set the aux channel mask bits
  connect(connectionDialog, SIGNAL(setauxchanmode(bool)), connection, SLOT(setauxchanmode(bool)));//The operating mode

  holdtime=14;

  //Add a BPM estimator and connect it to the graph functions via signals and slots
  bpmestimator = new  BPMestimator((1.0/DATA_RATE), false/*true*/);//Note peakfinder disabled, at least for debugging, not clear if it helps overall
  connect(this, SIGNAL(addtobmp(datasample_t*)), bpmestimator, SLOT(runloopfilter(datasample_t*)));
  connect(bpmestimator, SIGNAL(foundbpm(double, double)), this, SLOT(updateBPMlabel(double, double)));

  // include this section to fully disable antialiasing for higher performance:
  /*
  customPlot->setNotAntialiasedElements(QCP::aeAll);
  QFont font;
  font.setStyleStrategy(QFont::NoAntialias);
  customPlot->xAxis->setTickLabelFont(font);
  customPlot->yAxis->setTickLabelFont(font);
  customPlot->legend->setFont(font);
  */
  connect(connectionDialog, SIGNAL(newConnection(int)), this, SLOT(reinitConnection(int)));//Used to reinit the buttons on a new connection
  connect(this, SIGNAL(addtofile(datasample_t*)), fileDialog, SLOT(adddatatofile(datasample_t*)));//Used to pass data to the logfile
  //Start by defining a pen, the pen will be used for the last four electrodes, setting a dotted linestyle
  QPen pen;
  pen.setWidth(3);
  pen.setColor(QColor(Qt::red));

  QFont font("Helvetica [Cronyx]", 12);//Font size is set here (for main plot)
  
  for(qint8 n=0; n<8; n++)
	customPlot->addGraph();//Create the first 8 plot lines (used for the solid traces)

  // red line (use the standard color coding for the ECG traces, and make it follow the standard)
  customPlot->graph(7)->setPen(pen);
  customPlot->graph(7)->setBrush(QBrush(QColor(240, 0, 0, 30)));//A dim red, with high transparency
  customPlot->graph(7)->setAntialiasedFill(false);
  // yellow/orange line (it's supposed to be yellow, but that doesn't show up very well on the screen)
  pen.setColor(QColor(QColor(240, 170, 20)));
  customPlot->graph(6)->setPen(pen);
  customPlot->graph(6)->setBrush(QBrush(QColor(200, 160, 0, 30)));
  customPlot->graph(6)->setAntialiasedFill(false);
  //customPlot->graph(0)->setChannelFillGraph(customPlot->graph(1));
  // green line
  pen.setColor(QColor(Qt::green));
  customPlot->graph(5)->setPen(pen);
  customPlot->graph(5)->setBrush(QBrush(QColor(0, 240, 0, 30)));
  customPlot->graph(5)->setAntialiasedFill(false);
  // red dotted line
  pen.setStyle(Qt::DotLine);
  pen.setColor(QColor(Qt::red));
  customPlot->graph(4)->setPen(pen);
  // dark yellow/orange dotted line
  pen.setColor(QColor(QColor(200, 150, 30)));
  customPlot->graph(3)->setPen(pen);
  // dark greenish dotted line
  pen.setColor(QColor(QColor(20, 150, 30)));
  customPlot->graph(2)->setPen(pen);
  // brown dotted line
  pen.setColor(QColor(QColor(139, 69, 13)));
  customPlot->graph(1)->setPen(pen);
  // dark dotted line
  pen.setColor(QColor(QColor(30, 30, 30)));
  customPlot->graph(0)->setPen(pen);
  
  customPlot->addGraph(); // red dot
  customPlot->graph(8)->setPen(QPen(Qt::red));
  customPlot->graph(8)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(8)->setScatterStyle(QCPScatterStyle::ssDisc);
  customPlot->addGraph(); // orange dot
  customPlot->graph(9)->setPen(QPen(QColor(240, 170, 20)));
  customPlot->graph(9)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(9)->setScatterStyle(QCPScatterStyle::ssDisc);

  customPlot->addGraph(); // green dot
  customPlot->graph(10)->setPen(QPen(Qt::green));
  customPlot->graph(10)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(10)->setScatterStyle(QCPScatterStyle::ssDisc);
  customPlot->addGraph(); // magenta dot (should be red, but allow it to be distinguished)
  customPlot->graph(11)->setPen(QPen(Qt::magenta));
  customPlot->graph(11)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(11)->setScatterStyle(QCPScatterStyle::ssDisc);
  customPlot->addGraph(); // dark orange/yellow dot
  customPlot->graph(12)->setPen(QPen(QColor(200, 150, 30)));
  customPlot->graph(12)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(12)->setScatterStyle(QCPScatterStyle::ssDisc);
  customPlot->addGraph(); // dark green dot
  customPlot->graph(13)->setPen(QPen(QColor(20, 150, 30)));
  customPlot->graph(13)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(13)->setScatterStyle(QCPScatterStyle::ssDisc);
  customPlot->addGraph(); // brown dot
  customPlot->graph(14)->setPen(QPen(QColor(139, 69, 13)));
  customPlot->graph(14)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(14)->setScatterStyle(QCPScatterStyle::ssDisc);
  customPlot->addGraph(); // dark dot
  customPlot->graph(15)->setPen(QPen(QColor(20, 20, 20)));
  customPlot->graph(15)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(15)->setScatterStyle(QCPScatterStyle::ssDisc);
  
  customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
  customPlot->xAxis->setDateTimeFormat("hh:mm:ss");
  customPlot->xAxis->setAutoTickStep(false);
  customPlot->xAxis->setTickStep(2);
  customPlot->axisRect()->setupFullAxesBox();

  customPlot->yAxis->setAutoTickStep(false);
  customPlot->yAxis->setTickStep(0.5);//This is the convention for ECG plots, 0.5 millivolts per devision 
  customPlot->yAxis->setLabel("ECG (mv)");//Label
  customPlot->yAxis->setLabelPadding(-3);//Tight spacing

  // make left and bottom axes transfer their ranges to right and top axes:
  QObject::connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
  QObject::connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));
  
  //There are two text labels, at top and bottom
  textLabeltop = new QCPItemText(customPlot);
  customPlot->addItem(textLabeltop);
  textLabeltop->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
  textLabeltop->position->setType(QCPItemPosition::ptAxisRectRatio);
  textLabeltop->position->setCoords(0.5, 0); // place position at center/top of axis rect
  textLabeltop->setText("");//Blank text when initialised, if we get any electrode failures, the top text label is used to indicate them
  textLabeltop->setFont(font); // make font a bit larger
  textLabeltop->setPen(QPen(Qt::NoPen)); // show black border around text

  textLabelbottom = new QCPItemText(customPlot);
  customPlot->addItem(textLabelbottom);
  textLabelbottom->setPositionAlignment(Qt::AlignBottom|Qt::AlignHCenter);
  textLabelbottom->position->setType(QCPItemPosition::ptAxisRectRatio);
  textLabelbottom->position->setCoords(0.5, 1.0); // place position at center/bottom of axis rect
  textLabelbottom->setText("");//Blank text when initialised, if we get any RLD remaps, the bottom text label is used to indicate them
  textLabelbottom->setFont(font); // make font a bit larger
  textLabelbottom->setPen(QPen(Qt::NoPen)); // show no border around text

  //there is also a BPM indicator using larger font
  font.setPixelSize(21);
  textLabelbpm = new QCPItemText(customPlot);
  customPlot->addItem(textLabelbpm);
  textLabelbpm->setPositionAlignment(Qt::AlignBottom|Qt::AlignLeft);
  textLabelbpm->position->setType(QCPItemPosition::ptAxisRectRatio);
  textLabelbpm->position->setCoords(0.10, 0.2); // place position close to left/top of axis rect
  textLabelbpm->setText("");//Blank text when initialised, if we get any RLD remaps, the bottom text label is used to indicate them
  textLabelbpm->setFont(font); // make font a bit larger
  textLabelbpm->setPen(QPen(Qt::NoPen)); // show no border around text

  customPlot_=customPlot;
  // setup a timer that repeatedly calls MainWindow::realtimeDataSlot:
  QObject::connect(&dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
  dataTimer.start(40); // Interval 0 means to refresh as fast as possible, set 40ms to get approx 25hz
  //Timer to wipe the BPM label
  QTimer* LabelTimer= new QTimer;
  QObject::connect(LabelTimer, SIGNAL(timeout()), this, SLOT(BPMlabelcaller()));
  LabelTimer->start(3000);//3 second timeout
}

void Graph::realtimeDataSlot()
{
    //int i=customPlot_->graphCount();//Number of graph elements on the plot (not currently used)
    double key;
    // calculate two new data points:
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
    double key = 0;
#else
    if(demomode)
	  key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
#endif
    if(!demomode) {//Run the decode 
	connection->connectionStateMachine();//Runs the state machine
    }	
    else {//Fake data demonstration
      double value0 = qSin(key); //qSin(key*1.6+qCos(key*1.7)*2)*10 + qSin(key*1.2+0.56)*20 + 26;
      double value1 = qCos(key); //qSin(key*1.3+qCos(key*1.2)*1.2)*7 + qSin(key*0.9+0.26)*24 + 26;
      double plotme[8] = {value0, value1, value1*value0, value1-value0, value0+value1, -value0, -value1,value0-value1};//This should eventually be using buffer	
      for(qint8 n=0; n<8; n++) {
	if(((int)floor(key)%60)>15.0&&((int)floor(key)%8)!=n) {//Most of the time just draw the lines as normal, otherwise for one of the traces we treat it differently
    		(connection->latestdatasample).samples[n]=(qint16)(plotme[n]*(double)(1<<14));
	}
	else if(((int)floor(key)%40)>20.0) {
		(connection->latestdatasample).samples[n]=(qint16)((1<<15)-1);//One away from upper limit, this is the RLD replacement signal
	}
	else if(((int)floor(key)%25)>2.0)
		(connection->latestdatasample).samples[n]=-(qint16)(1<<15);//The lead off signal
	/*else*/ if(n==7)
		(connection->latestdatasample).samples[n]=-(qint16)(1<<15)+1;//Disabled (this will remain on)
	if(n && n<7)
        	(connection->latestdatasample).quality[n]=0.9;
	else
        	(connection->latestdatasample).quality[n]=0.6+0.35*qSin(key/2);
      }
      (connection->latestdatasample).sampletime=key;
      //(connection->latestdatasample).channelmask=0x00FF;//The 8 ECG channels are active (little endian on the device) (for the fake data this is disabled)
      (connection->latestdatasample).device_scale_factor=1.0/(float)(1<<15);//Around a millivolt of simulated range
      addData(&(connection->latestdatasample));//Add the plot data directly
    }
}

void Graph::addData(datasample_t* datasamp) {
    static double lastPointKey = 0;
    static qint16 inhibitmask = 0;//Initialised so that it allows auto channel disabling
    double key=datasamp->sampletime;
    //Each time data is added to the plot, the file add slot is also called
    emit addtofile(datasamp);
    //Also send data to the BPM processor
    emit addtobmp(datasamp);
    quint16 tmpmask=datasamp->channelmask;
    for(int n=0;n<8;n++) {//Set the mask according to the checked buttons across the bottom of the graph
	if(channelenablebuttons[n]->isChecked())//Button needs to be checked
		tmpmask|=(1<<n);
	else if(!(inhibitmask&(1<<n)))//Only disable the channel if the inhibit mask is not set
		tmpmask&=~(1<<n);
	//The first 5 lines use a dotted line style, it is important to offset the pattern appropriatly. Do this before any new data added, as it will distort axes
	if(n<5) {
		QPen thepen=customPlot_->graph(n)->pen();
		double offsetvalue=thepen.dashOffset();//retrieve current offset
		QVector<qreal> pattern=thepen.dashPattern();
		double dashlength=0;
		for (int i = 0; i < pattern.size(); i++)
			dashlength+=pattern.at(i);//The length of the dashes
		dashlength*=thepen.widthF();	//dashes are in units of pen width, convert to pixels
		offsetvalue*=thepen.widthF();	//also in pixel units
		//Add the plot length of the removed data to the offsetvalue, looping through all points that are due to be removed
		const QCPDataMap *dataMap = customPlot_->graph(n)->data();//Read the raw data from the graph
		QMap<double, QCPData>::const_iterator i = dataMap->constBegin();
		double x,y,x_old,y_old;
		while (i != dataMap->constEnd()) {//break once we reach the limit, need to include one more datapt
			x=customPlot_->xAxis->coordToPixel(i.value().key);
			y=customPlot_->yAxis->coordToPixel(i.value().value);
			if(i != dataMap->constBegin())
				offsetvalue+=sqrt(((x-x_old)*(x-x_old))+((y-y_old)*(y-y_old)));//Distance in pixel units
			x_old=x;
			y_old=y;
			if(i.value().key>=(key-holdtime))//We have passed the trim limit and reached the first not chopped data point
				break;
			i++;
		}
		offsetvalue=fmod(offsetvalue,dashlength);//modulo the offset with the dash length
		thepen.setDashOffset(offsetvalue/thepen.widthF());//convert back to dash units
		customPlot_->graph(n)->setPen(thepen);//The pattern offset is continually adjusted to match the data
	}
    }
    emit setmask(tmpmask);
    QString txt_bot=QString("RLD remap:");
    QString txt_top=QString("Lead off:");
    QStringList electrodes;
    electrodes << "RA" << "LA" << "LL" << "C1" << "C2" << "C3" << "C4" << "C5";
    for(int n=0; n<8; n++) {		//A maximum of 8 channels on the plot (TODO, add a second plot with accel, and possibly an orientation visualisation)
	if((datasamp->channelmask)&(1<<n)) {
		qint16 dat=datasamp->samples[n];//error status signals (limits are used to signal for electrode off and RLD remap in operation)
		float qal=(float)datasamp->quality[n];//this is float in the 0 to 1.0 range, with 1.0 representing max quality
		if(abs(dat)>((1<<15)-2)) {//Add some text annotation with info on connected lead numbers
			if(dat>0) {	// update text label bottom (RLD replacement)
				//txt_bot.append(QString::number(n,10));//Use the numerical channel
				txt_bot.append(electrodes.at(n));//Use the electrode name as a string
				txt_bot.append(",");
				channelenablebuttons[n]->setEnabled(true);
				channelenablebuttons[n]->setChecked(false);
				inhibitmask|=(1<<n);
			}
			else if(dat==-(1<<15)) {//Missing electrode at the top, but only if its not a disabled channel (which has code of lower limit +1)
				//txt_top.append(QString::number(n,10));
				txt_top.append(electrodes.at(n));
				txt_top.append(",");
				channelenablebuttons[n]->setEnabled(true);
				channelenablebuttons[n]->setChecked(false);
				inhibitmask|=(1<<n);
			}
			else if(dat==-(1<<15)+1) {//A channel that is disabled at the device end
				channelenablebuttons[n]->setChecked(false);//Disable the channel to free up capacity
				channelenablebuttons[n]->setEnabled(false);//Set the buttons of disabled channels as greyed out. It is assumed config doesnt change
				inhibitmask&=~(1<<n);
			}//but these setting are reset when a new connection is made to a device or a port
		}
        	else {	//Normal buttons
			channelenablebuttons[n]->setEnabled(true);
			if(datasamp->channelmask&(1<<n))//Channel is enabled
				channelenablebuttons[n]->setChecked(true);//Reset the button as appropriate
			inhibitmask&=~(1<<n);
			//customPlot_->graph(n)->addData(key, (float)dat/(float)(1<<15));//Data is in the +-1 range
			customPlot_->graph(7-n)->addData(key, (float)dat*datasamp->device_scale_factor);//Data is in millivolts (note reverse order for first 8)
        		customPlot_->graph(n+8)->clearData();    // set data of dots: 
        		//customPlot_->graph(n+8)->addData(key, (float)dat/(float)(1<<15));//The indicator
			customPlot_->graph(n+8)->addData(key, (float)dat*datasamp->device_scale_factor);
			//The is a progressbar style shading across the button
			if(qal>0.99)
				qal=0.99;
			if(qal<0.15)
				qal=0.15;
//qDebug() << style_populator[n].arg(QString::asprintf("%.2f",qal),QString::asprintf("%.2f",qal+0.01));
			channelenablebuttons[n]->setStyleSheet(style_populator[n].arg(QString::asprintf("%.2f",qal),QString::asprintf("%.2f",qal+0.01)));
		}
	}
	else {//This plot line and indicator point needs to be removed from the graph
		customPlot_->graph(7-n)->clearData();//Remove all the plotline data for disabled plots
		customPlot_->graph(n+8)->clearData();//Remove the plot point as well
	}
    }
    if(txt_top.size()>9) {
	textLabeltop->setText(txt_top.left(txt_top.size()-1));
	textLabeltop->setPen(QPen(Qt::black));//Box around the text
    }
    else {
	textLabeltop->setText("");
	textLabeltop->setPen(QPen(Qt::NoPen));
    }
    if(txt_bot.size()>10) {
	textLabelbottom->setText(txt_bot.left(txt_bot.size()-1));//Trim off the trailing commas on these two text lines before setting them
    }
    else {
	textLabelbottom->setText("");
    }
    // remove data of lines that's outside visible range (channels that are marked as bad get removed):
    for(int n=0; n<8; n++) {
        customPlot_->graph(n)->removeDataBefore(key-holdtime);
    }
    // rescale value (vertical) axis to fit the current data:
    for(int n=0, l=0; n<8; n++) {
	if((datasamp->channelmask)&(1<<n)) {
        	customPlot_->graph(7-n)->rescaleValueAxis(l>0);//Only enlarge the extra axes
		l++;
	}
    }
    //Grab the upper and lower limits of the range, and expand out to the nearest 0.5mv points, to avoid excessive distracting y axis rescaling
    float low=floor(customPlot_->yAxis->range().lower/0.5)*0.5;
    float up=ceil(customPlot_->yAxis->range().upper/0.5)*0.5;
    customPlot_->yAxis->setRange(up, up-low, Qt:: AlignRight);

    lastPointKey = key;
    // make key axis range scroll with the data (at a constant range size of 14):
    customPlot_->xAxis->setRange(key+0.25, holdtime+0.25, Qt::AlignRight);
    customPlot_->replot();

    // check to see if the GPS heading/battery voltage telemetry stream is enabled. If it is, look for a negative value and strip off percentage charge
    static qint16 percent_charged=-1;// The -1 code indicates that there is no valid data
    qint16 percent_charged_=-1;
    if((datasamp->channelmask)&(1<<11)) {//the 11th channel contains battery/heading info
	qint16 dat=datasamp->samples[11];
	if(dat<0) {			//This is a battery value (the heading values are always greater or equal to zero)
		percent_charged_=(-dat)&0x7F;//the lower 7 bits are the charge state
		if(percent_charged_>100 || percent_charged_<0)//the data is invalid
			percent_charged_=-1;
		percent_charged=percent_charged_;//copy into the static
	}
    }

    // calculate frames per second:
    static double lastFpsKey;
    static int frameCount;
    ++frameCount;
    if (key-lastFpsKey > 2) // average fps over 2 seconds
    {
      if(percent_charged<0) {
      bar_->showMessage(
          QString("%1 FPS, Total Data points: %2")
          .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
          .arg(customPlot_->graph(0)->data()->count()+customPlot_->graph(1)->data()->count())
          , 0); }
      else {
      bar_->showMessage(
          QString("%1 FPS, Total Data points: %2, Battery: %3%")
          .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
          .arg(customPlot_->graph(0)->data()->count()+customPlot_->graph(1)->data()->count())
          .arg(percent_charged)
          , 0); }
      lastFpsKey = key;
      frameCount = 0;
    }
}

void Graph::bracketDataSlot()
{
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
  double secs = 0;
#else
  double secs = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
#endif
  
  // update data to make phase move:
  int n = 500;
  double phase = secs*5;
  double k = 3;
  QVector<double> x(n), y(n);
  for (int i=0; i<n; ++i)
  {
    x[i] = i/(double)(n-1)*34 - 17;
    y[i] = qExp(-x[i]*x[i]/20.0)*qSin(k*x[i]+phase);
  }
  customPlot_->graph()->setData(x, y);
  
  itemDemoPhaseTracer->setGraphKey((8*M_PI+fmod(M_PI*1.5-phase, 6*M_PI))/k);
  
  customPlot_->replot();
  
  // calculate frames per second:
  double key = secs;
  static double lastFpsKey;
  static int frameCount;
  ++frameCount;
  if (key-lastFpsKey > 2) // average fps over 2 seconds
  {
    //ui->statusBar->showMessage(
    //      QString("%1 FPS, Total Data points: %2")
    //      .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
    //      .arg(customPlot_->graph(0)->data()->count())
    //      , 0);
    lastFpsKey = key;
    frameCount = 0;
  }
}

void Graph::setupPlayground(QCustomPlot *customPlot)
{
  Q_UNUSED(customPlot)
}

void Graph::screenShot()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
//  QPixmap pm = QPixmap::grabWindow(qApp->desktop()->winId(), this->x()+2, this->y()+2, this->frameGeometry().width()-4, this->frameGeometry().height()-4);
#else
//  QPixmap pm = qApp->primaryScreen()->grabWindow(qApp->desktop()->winId(), this->x()+2, this->y()+2, this->frameGeometry().width()-4, this->frameGeometry().height()-4);
#endif
//  QString fileName = demoName.toLower()+".png";
//  fileName.replace(" ", "");
//  pm.save("./screenshots/"+fileName);
//  qApp->quit();
}

void Graph::reinitConnection(int connectiontype) {
	for(int n=0; n<8; n++) {
		channelenablebuttons[n]->setChecked(true);//All channels default to enabled
		channelenablebuttons[n]->setEnabled(false);//But they are all greyed out
	}
}

void Graph::changeHoldTime(void) {
	QString text_=validatorLineEdit->text();
	double h=text_.toDouble();//Set this using the entered text
	if(h<=2) {//Low holdtime is not allowed, and a negative holdtime is used to control enable the demo mode
		if(h<0) {
			demomode=true;
			h=-(int)h;
			for(int n=0; n<8; n++) {
				channelenablebuttons[n]->setChecked(true);//All channels default to enabled - do this each time the hold time is changed
				channelenablebuttons[n]->setEnabled(false);//But greyed out
			}
		}
		else
			demomode=false;
		if(h<4)
			h=14;
	}
	else
		demomode=false;
	holdtime=h;
	validatorLineEdit->setText(QString::number(holdtime));//Show the value that is actually being used
	customPlot_->xAxis->setTickStep(floor((holdtime+0.25)/6));//Approx 7 ticks
}

void Graph::updateBPMlabel(double value, double timestamp) {
	static quint8 localbpm;
	static double timeout;
	if(qIsNaN(value)) {
		if(timestamp>timeout)
			textLabelbpm->setText("BPM:--");//No valid heartbeat
		return;			//return as there is nothing to do
	}
	timeout=timestamp+1.5;//1.5 seconds of invalid signal gives a timeout
	float err=value-localbpm;
	if(fabs(err)>0.6)
		localbpm=(quint8)round(value);
	QString lab="BPM:";
	lab.append(QString::number( localbpm, 10));
	textLabelbpm->setText(lab);
}

void Graph::BPMlabelcaller() {
	double key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
	updateBPMlabel(qSNaN(), key);//Call this every three seconds to wipe label if there is no data arriving 
}








