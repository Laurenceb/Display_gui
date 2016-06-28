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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include "qcustomplot.h" // the header file of QCustomPlot. Don't forget to add it to your project, if you use an IDE, so it gets compiled.
#include "connectionmanager.h"
#include "fileselectdialog.h"
#include "BPMestimator.h"

class Graph : public QObject {
 Q_OBJECT
  
public:  
  void setupRealtimeDataDemo(QCustomPlot *customPlot, PortSelectDialog *connectionDialog, FileSelectDialog* fileDialog);
  void setupPlayground(QCustomPlot *customPlot);
  QLineEdit* validatorLineEdit;
  QStatusBar* bar_;
  QPushButton** channelenablebuttons;
  QStringList style_populator; 
  signals:
   void addtofile(datasample_t* data);
   void addtobmp(datasample_t* data);
   void setmask(quint16 mask);
private slots:
  void realtimeDataSlot();
  void bracketDataSlot();
  void screenShot();
  void addData(datasample_t* datasamp);
  void reinitConnection(int connectiontype);
  void changeHoldTime();
  void updateBPMlabel(double value, double timestamp);
  void BPMlabelcaller(void);
private:
  QString demoName;
  QTimer dataTimer;
  QCPItemTracer *itemDemoPhaseTracer;
  QCustomPlot *customPlot_;
  bool demomode;//If this is true we run in demo mode
  QCPItemText *textLabeltop;
  QCPItemText *textLabelbottom;
  QCPItemText *textLabelbpm;
  qint16 chanmask;
  quint16 holdtime;
  connectionManager* connection;
  BPMestimator* bpmestimator;
};

#endif // MAINWINDOW_H
