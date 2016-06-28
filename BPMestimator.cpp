#include <math.h>
#include <QString>
#include <QtGlobal>
#include <QDebug>

#include "connectionmanager.h"
#include "BPMestimator.h"

//Constructor
BPMestimator::BPMestimator(float delta, bool evaluate_peak) //:
//QObject(parent)
{
	delta_t = delta;				//This is the time step
	autocorr = 0;					//Set these to zero
	delayed=(double*)malloc(MAX_DELTA*sizeof(double));
	correlator=(double*)malloc((MAX_DELTA-MIN_DELTA+1)*sizeof(double));
	memset(correlator,0,(MAX_DELTA-MIN_DELTA+1)*sizeof(double));//Zero all correlator bins (they might have been allocated from heap or somewhere full of junk)
	for(quint16 ind=0; ind<MAX_DELTA; ind++)
		delayed[ind]=qSNaN();			//None of this data was available
	current_BPM=qSNaN();
	evalpeak=evaluate_peak;				//Set this variable
	corr_filter=0.2*delta;				//A 5 second effective averaging period for the autocorrelation evluation
}


void BPMestimator::runloopfilter(datasample_t* data) {
	double sample=0;
	static double lasttimestamp;
	quint8 numsamples=0;
	for(quint8 n=0; n<8; n++) {
		if(data->channelmask & (1<<n)) {
			if(data->samples[n]<((1<<15)-1) && data->samples[n]>-((1<<15)-2)) {//Valid sample range, not an error code
				sample+=abs(data->samples[n]);
				numsamples++;
			}
		}
	}
	if(numsamples) {
		sample/=numsamples;			//This is then multiplied with a delayed copy of itself
		autocorr=(autocorr*(1-corr_filter))+(corr_filter*sample*sample);//for normalization of the autocorrelation function
	}
	else
		sample=qSNaN();				//Indicates that the sample is not usable
	//Padder and padding estimator based on timestamps 
	double skip=data->sampletime-lasttimestamp;
	quint16 iterations_skip;
	if(lasttimestamp!=0.0) 				//Not the first iteration
		iterations_skip=round(skip/delta_t);
	else 
		iterations_skip=0;
	lasttimestamp=data->sampletime;
	updatedelay(sample, iterations_skip);		//iterations skip is the number of samples to skip (always at least 1, more than one gives NaN padding)
	for(quint16 ind=MIN_DELTA;ind<=MAX_DELTA;ind++) {
		if(!qIsNaN(delayed[ind-1]) && !qIsNaN(sample)) {//If the data is usable, add it in
			correlator[ind-MIN_DELTA]=(correlator[ind-MIN_DELTA]*(1-corr_filter))+(corr_filter*delayed[ind-1]*sample);
		}
	}
	//Then we normalize the low passed correlator, and search for a sharp peak using maximum and the second derivative. BPM found to nearest bin, approx +-0.2BPM
	float derivative[MAX_DELTA-MIN_DELTA+1];
	float derivative_[MAX_DELTA-MIN_DELTA+1];
	derivative_[0]=0;
	derivative_[MAX_DELTA-MIN_DELTA]=0;		//zero pad the second derivative
	if(evalpeak) {					//peakfinder activated
		for(quint16 ind=0; ind<(MAX_DELTA-MIN_DELTA);ind++)//first derivative
			derivative[ind]=(correlator[ind+1]-correlator[ind])/autocorr;
		for(quint16 ind=0; ind<(MAX_DELTA-MIN_DELTA);ind++)//first derivative
			derivative_[ind]=(derivative[ind+1]-derivative[ind])/(delta_t*delta_t);//compensate for time units
	}
	float max=0,sum_correlation=0;
	quint16 index=0;
	for(quint16 ind=0; ind<(MAX_DELTA-MIN_DELTA);ind++) {//the function to be optimised
		if(evalpeak)
			derivative_[ind]*=-correlator[ind]/autocorr;//negative as we look for spike of negative second derivative
		else
			derivative_[ind]=correlator[ind]/autocorr;//ignore the peak finder
	}
	for(quint16 ind=0; ind<(MAX_DELTA-MIN_DELTA);ind++) {
		if(derivative_[ind]>max) {
			index=ind;
			max=derivative_[ind];
		}
	//qDebug() << correlator[ind]/autocorr << ',';
		sum_correlation+=correlator[ind]/autocorr;//used for (secondary) normalization purposes	
	}
	//qDebug() << endl;
	//Calculate the relative fraction of the correlation within a band around the best match point, be sure to set the band appropriatly	
	qint16 lower=0,upper=MAX_DELTA-MIN_DELTA;	//Upper and lower limits of the search window
	if(index-(upper/(2*SEARCH_FRACTION))>0)
		lower=index-(upper/(2*SEARCH_FRACTION));//This will be the bottom of the search range
	else
		lower=0;
	if(lower) {//This isn't pegged at the bottom
		if(index+(upper/(2*SEARCH_FRACTION))<upper)
			upper=index+(upper/(2*SEARCH_FRACTION));//This will be the upper limit of the range
		else
			lower=upper-(upper/SEARCH_FRACTION);//Redraw the range, as upper is pegged at the top
	}
	else //Lower is pegged
		upper=(upper/SEARCH_FRACTION);
	float fraction=0;
	for(quint16 ind=lower; ind<upper; ind++)
		fraction+=correlator[ind]/autocorr;
	fraction/=sum_correlation;
	fraction*=MAX_DELTA/(index+MIN_DELTA);//Lower index needs a boost due to the fact that there will be a chain of repeating peaks
	//qDebug()<<fraction << ',' << index << ',' <<correlator[index]/autocorr << ',' << lower << ',' << upper;
	//The autocorrelation has to be over 0.45 and more than 25% of the total correlation has to be within a surrounding window of the identified peak if its valid
	if(correlator[index]/autocorr>0.45 && fraction>(evalpeak?0.25:0.15)) {//lower the region limit to 15% when evalpeak is not activated
		if(!qIsNaN(sample) && index && index !=(MAX_DELTA-MIN_DELTA-1))//the index is not allowed to be pegged at the bottom or top of the range
			current_BPM=60.0/((index+MIN_DELTA)*delta_t);
		else
			current_BPM=qSNaN();//Need a good correlator output and a current signal to give an output	
	}
	else
		current_BPM=qSNaN();
	emit foundbpm(current_BPM, data->sampletime);	//this signal can be linked to display slots to show the heartrate
}

//Update the sample lag buffer
void BPMestimator::updatedelay(float data, int iterations) {
	for(;iterations;iterations--) {		//Iterate through
		for(quint16 n=MAX_DELTA-1; n; n--)//Run backwards, shifting the delay train
			delayed[n]=delayed[n-1];
		delayed[0]=qSNaN();		//NaN padding is used for missed samples (the NaN is overwritten if there is only a single missed/skipped sample)
	}
	delayed[0]=data;
}

BPMestimator::~BPMestimator()
{
}
