#ifndef T0CALIB_H
#define T0CALIB_H

#include "TOFrPicoDst.h"
#include "math.h"

// RooBarb
#include "TreeAnalyzer.h"
#include "Logger.h"
#include "Utils.h"
using namespace jdb;

#include <map>
using namespace std;

class T0Calib : public TreeAnalyzer
{
protected:

	TOFrPicoDst * pico;

	string aggregateBy;
	map<string, float> correction;
	map<string, vector<float>> aggregate;
	

	static constexpr float cLight = 29.9792458;

	void inverseBeta( int iteration = 0 );

public:
	virtual const char* classname() const { return "T0Calib"; }

	T0Calib( );
	virtual void initialize( );
	~T0Calib();

	virtual bool keepEvent();
	virtual void analyzeEvent();
	virtual void preEventLoop();
	virtual void postEventLoop();



	string nameFor( int tray, int module = -1, int cell = -1 ){
		
		DEBUG( classname(), "aggregating "<< aggregateBy  );

		// 
		// else if ( "module" == aggregateBy || "board" == aggregateBy )
		// 	return fullNameFor( tray, module, -1 );
		// else 
		if ( "tray" == aggregateBy )
			return fullNameFor( tray, -1, -1 );
		if ( "cell" == aggregateBy )
			return fullNameFor( tray, module, cell );
		if ( "module" == aggregateBy || "board" == aggregateBy )
			return fullNameFor( tray, module, -1 );

		return "";
	}

	string fullNameFor( int tray, int module = -1, int cell = -1 ){
		if ( -1 == module && -1 == cell )
			return "t"+ts( tray );
		if (  -1  == cell )
			return "t"+ts( tray )+"m"+ts(module);
		else
			return "t"+ts( tray )+"m"+ts(module)+"c"+ts(cell);

		return "";
	}


	float calcMean( vector<float> &set ){
		float t = 0.0f;
		for ( int i = 0; i < set.size(); i++ )
			t+= set[i];
		return t / (float)set.size();
	}

	float calcStd( vector<float> &set ){
		float total = 0;
		float _mean = calcMean( set );
		float _std = 0.0;
		for ( float v : set ){
			total += ( v - _mean ) * ( v - _mean );
		}
		if ( set.size() > 1 )
			_std = total / (set.size() - 1.0);
		_std = sqrt( _std );
		return _std;
	}

	float truncMean( vector<float> & set, float nSig = 1.0, float _mean = -999.999 ){
		
		if ( -999.999 >= _mean )
			_mean = calcMean(set);
		float _std = calcStd(set);

		float total = 0;
		int n = 0;
		for ( float v : set ){
			DEBUG( classname(), "v - mean(" << _mean << " ) = " << fabs(v - _mean) << " < " <<  (_std * nSig) )
			if ( fabs( v - _mean ) < _std * nSig ){
				total += v;
				n++;
				DEBUG( classname(), "accept value = " << v );
			} else {
				DEBUG( classname(), "rejected value = " << v  );
			}
		}
		DEBUG( classname(), "total=" << total << ", n = " << n );
		return total / (float)n;
	}


	bool keepHit( int iHit ){
		if ( pico->tofCorr[ iHit ] <= -999 )
			return false;

		double p = pico->pt[ iHit ] * cosh( pico->eta[ iHit ] );
	    double nSigPi = pico->nSigPi[ iHit ];

	    if ( pico->tofCorr[ iHit ] * 1 != pico->tofCorr[ iHit ] )
	    	return false;

	    if ( 0.3 > p || 0.6 < p ) 
	        return false;
	    if ( nSigPi > 2.0 ) 
	        return false;
	    if ( 15 > pico->nHitsFit[ iHit ] ) 
	        return false;
	    
		return true;
	}


	double expectedTof( double length, double p, float m = 0.13957018 /* [GeV/c] */ ){
		return sqrt( length*length / (cLight*cLight) * ( 1 + m*m / (p*p) ) );
	}
	

	void exportParameters( );

	void makeCorrections();

};




#endif