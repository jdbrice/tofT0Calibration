#include "T0Calib.h"
#include "math.h"


T0Calib::T0Calib( ) {

}

void T0Calib::initialize( ){
	

	INFO( classname(), "" );

	pico = new TOFrPicoDst( chain );
	book->makeAll( nodePath + ".histograms" );

	aggregateBy = config.getString( nodePath + ".Aggregate:by", "cell" );
	INFO( classname(), "Aggregating by " << aggregateBy );


	int nCells = 1;
	int nMods = 1;
	if ( "cell" == aggregateBy ){
		nCells = 6;
		nMods = 32;
	}
	if ( "module" == aggregateBy || "board" == aggregateBy )
		nMods = 32;

	for ( int iTray = 1; iTray <= 120; iTray++ ){
		for ( int iMod = 1; iMod <= nMods; iMod ++ ){
			for ( int iCell = 1; iCell <= nCells; iCell ++ ){
				string n = nameFor( iTray, iMod, iCell );
				correction[ n ] = 0;
			}
		}
	}

}

T0Calib::~T0Calib(){
	INFO(classname(), "");
}

void T0Calib::preEventLoop(){
	book->cd();

	if ( iEventLoop == 0 )
		inverseBeta();

	aggregate.clear();
}

bool T0Calib::keepEvent(){
	if ( 0 >= pico->numberOfVpdEast || 0 >= pico->numberOfVpdWest ) 
        return false;
    double vX = pico->vertexX;
    double vY = pico->vertexY;
    double vR = sqrt( vX * vX + vY * vY  );
    if ( vR > 1.0 ) 
        return false;
    if ( fabs( pico->vpdVz - pico->vertexZ ) > 6.0 ) 
        return false;
    return true;
}


void T0Calib::analyzeEvent(){

	book->cd();
	for ( int iHit = 0; iHit < pico->nTofHits; iHit++ ){
		book->fill( "occupancy_tray", pico->tray[iHit] );

		if ( !keepHit(iHit) )
			continue;

		DEBUG( classname(), "Getting name" );
		string n = nameFor( pico->tray[ iHit ], pico->module[ iHit ], pico->cell[ iHit ] );
		DEBUG( classname(), "Name = " << n );
		aggregate[ n ].push_back( 
			
				(pico->tofCorr[ iHit ] - correction[ n ] - expectedTof( pico->length[iHit], pico->pt[ iHit ] * cosh( pico->eta[ iHit ] ) ) ) 

				);

		INFO( classname(), "{" );
		INFO( classname(), "DT = " << (pico->tofCorr[ iHit ] - expectedTof( pico->length[iHit], pico->pt[ iHit ] * cosh( pico->eta[ iHit ] ) ) )  );
		INFO( classname(), "tofCorr = " << pico->tofCorr[ iHit ] );
		INFO( classname(), "eTOF = " << expectedTof( pico->length[iHit], pico->pt[ iHit ] * cosh( pico->eta[ iHit ] ) ) );
		INFO( classname(), "l, pt, eta = " << pico->length[iHit] <<", " << pico->pt[ iHit ] <<", " << cosh( pico->eta[ iHit ] ) );
		INFO( classname(), "}" );


	}


}

void T0Calib::postEventLoop(){
	INFO(classname(), "");

	makeCorrections();
	
	inverseBeta( iEventLoop + 1);

	exportParameters();
}


void T0Calib::inverseBeta( int iteration ){

    if ( !chain ){
        ERROR( classname(), "Invalid DataSource ");
        return;
    }

    

    /**
     * Make histos
     */
    HistoBins pBins( config, "b.p" );
    string ibName = "b.iBeta";
    if ( iteration <= 0 )
    	ibName = "b.iBetaFirst";
	HistoBins ibBins( config, ibName ); 
	TH2F * h2 = new TH2F( ("inverseBeta"+ts(iteration)).c_str() , ("1/beta : it " + ts(iteration)).c_str(), pBins.nBins(), pBins.getBins().data(), ibBins.nBins(), ibBins.getBins().data() );
    book->add( "inverseBeta"+ts(iteration), h2 );
    book->clone( "pionDeltaT", "pionDeltaT" + ts(iteration) );

    
    TaskTimer t;
    t.start();

    Int_t nEvents = (Int_t)chain->GetEntries();
	nEventsToProcess = config.getInt( nodePath + ".input.dst:nEvents", nEvents );
	
	// if neg then process all
	if ( nEventsToProcess < 0 )
		nEventsToProcess = nEvents;
    INFO( classname(), "Loaded: " << nEventsToProcess << " events " );
    
    TaskProgress tp( "Plotting 1/beta", nEventsToProcess );
    
    // loop over all events
    for(Int_t i=0; i<nEventsToProcess; i++) {
        chain->GetEntry(i);

        tp.showProgress( i );

        /**
         * Select good events (should already be done in splitter )
         */
        if ( !keepEvent() )
            continue;

        int nTofHits = pico->nTofHits;
        for ( int iHit = 0; iHit < nTofHits; iHit++ ){

 
            const double tLength = pico->length[ iHit ];
            const double p = pico->pt[ iHit ] * cosh( pico->eta[ iHit ] );
            double corrTof = pico->tofCorr[ iHit ];
            string n = nameFor( pico->tray[ iHit ], pico->module[ iHit ], pico->cell[ iHit ] );
            
            if (aggregate[n].size() >= 3 ){
            	float corr = correction[n];
            	corrTof = corrTof - corr;
            } else if ( iteration >=1 && aggregate[n].size() < 20 )
            	continue;

            if ( keepHit( iHit ) ){
        		float eTof = expectedTof( pico->length[iHit], pico->pt[ iHit ] * cosh( pico->eta[ iHit ] ) );
        		float dt = corrTof - eTof;
        		book->fill( "pionDeltaT" + ts(iteration), pico->tray[iHit], dt );
        	}
            
            double iBeta = (corrTof / tLength )*cLight;

            book->fill( "inverseBeta"+ts(iteration), p, iBeta );
            
        } // loop on tofHits
    } // end loop on events
    INFO( classname(), "Completed in " << t.elapsed() );
}

void T0Calib::exportParameters(){

	string output = config.getString( nodePath + ".output.params", "params.dat" );
	ofstream out( output.c_str() );
	for ( int iTray = 1; iTray <= 120; iTray++ ){
		for ( int iMod = 1; iMod <= 32; iMod ++ ){
			for ( int iCell = 1; iCell <= 6; iCell ++ ){

				string n = nameFor( iTray, iMod, iCell );
				float corr = correction[n];

				out << setw(12) << left << iTray << setw(12) << iMod << setw(12) << iCell << endl;
				out << corr << endl;
			}
		}
	}

	out.close();
}

void T0Calib::makeCorrections(){

	int nCells = 1;
	int nMods = 1;
	if ( "cell" == aggregateBy ){
		nCells = 6;
		nMods = 32;
	}
	if ( "module" == aggregateBy || "board" == aggregateBy )
		nMods = 32;

	for ( int iTray = 1; iTray <= 120; iTray++ ){
		for ( int iMod = 1; iMod <= nMods; iMod ++ ){
			for ( int iCell = 1; iCell <= nCells; iCell ++ ){

				string n = nameFor( iTray, iMod, iCell );
				if ( aggregate[n].size() < 10 ){
					correction[n] = 0.0f;
					continue;
				}
				float corr = calcMean( aggregate[n] );

				// ensure that the mean stays reasonable
				corr = truncMean( aggregate[n], 3.0, corr );
				corr = truncMean( aggregate[n], 2.0, corr );
				corr = truncMean( aggregate[n], 1.0, corr );

				correction[ n ] += corr;
			}
		}
	}
}