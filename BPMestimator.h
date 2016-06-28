#ifndef BPMESTIMATE_H
#define BPMESTIMATE_H

#include <math.h>
#include "connectionmanager.h"

#define MAX_DELTA 300
#define MIN_DELTA 75

#define SEARCH_FRACTION (10.0) /*10% of the range goes into the peak evaluator*/ 

class BPMestimator : public QObject 
{
	Q_OBJECT
	public:
		explicit BPMestimator(float delta, bool evaluate_peak);
		~BPMestimator();
		signals:
		 void foundbpm(double current_BPM, double timestamp);
	public slots:
		void runloopfilter(datasample_t* data);
	private:
		void updatedelay(float data, int iterations);
		double current_BPM;
		double* delayed;//delay buffer
		double* correlator;
		float autocorr;
		float delta_t;
		float corr_filter; 
		bool evalpeak;		//if this is true, we use a derivative method to find the peak
};

#endif
