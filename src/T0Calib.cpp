#include "T0Calib.h"
#include "math.h"


T0Calib::T0Calib( XmlConfig * config, string np, string fileList, string jobPrefix) : TreeAnalyzer( config, np, fileList, jobPrefix ){

	Logger::setGlobalLogLevel("info");

	INFO( tag, "" );

	pico = new TOFrPicoDst( chain );
	book->makeAll( nodePath + "histograms" );
}

T0Calib::~T0Calib(){
	INFO(tag, "");
}

void T0Calib::preEventLoop(){
	book->cd();

	// inverseBeta();
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

		string n = nameFor( pico->tray[ iHit ], pico->module[ iHit ], pico->cell[ iHit ] );
		aggregate[ n ].push_back( 
			
				(pico->tofCorr[ iHit ] - expectedTof( pico->length[iHit], pico->pt[ iHit ] * cosh( pico->eta[ iHit ] ) ) ) 

				);
	}


}

void T0Calib::postEventLoop(){
	INFO(tag, "");

	makeCorrections();
	
	inverseBeta(1);

	exportParameters();
}


void T0Calib::inverseBeta( int iteration ){

    if ( !chain ){
        logger->error(__FUNCTION__) << "Invalid DataSource " << endl;
        return;
    }

    /**
     * Make histos
     */
    HistoBins pBins( cfg, "b.p" );
    string ibName = "b.iBeta";
    if ( iteration <= 0 )
    	ibName = "b.iBetaFirst";
	HistoBins ibBins( cfg, ibName ); 
	TH2F * h2 = new TH2F( ("inverseBeta"+ts(iteration)).c_str() , ("1/beta : it " + ts(iteration)).c_str(), pBins.nBins(), pBins.getBins().data(), ibBins.nBins(), ibBins.getBins().data() );
    book->add( "inverseBeta"+ts(iteration), h2 );
    
    TaskTimer t;
    t.start();

    Int_t nEvents = (Int_t)chain->GetEntries();
    INFO( tag, "Loaded: " << nEvents << " events " );
    
    TaskProgress tp( "Plotting 1/beta", nEvents );
    
    // loop over all events
    for(Int_t i=0; i<nEvents; i++) {
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
            
            if (aggregate[n].size() >= 10 ){
            	float corr = correction[n];
            	corrTof = corrTof - corr;
            } else if ( iteration >=1 &&  aggregate[n].size() < 20 )
            	continue;

            if ( keepHit( iHit ) && iteration >= 1 ){
        		float eTof = expectedTof( pico->length[iHit], pico->pt[ iHit ] * cosh( pico->eta[ iHit ] ) );
        		float dt = corrTof - eTof;
        		book->fill( "pionDeltaT", pico->tray[iHit], dt );
        	}
            
            double iBeta = (corrTof / tLength )*cLight;

            book->fill( "inverseBeta"+ts(iteration), p, iBeta );
            
        } // loop on tofHits
    } // end loop on events
    INFO( tag, "Completed in " << t.elapsed() );
}

void T0Calib::exportParameters(){

	string output = cfg->getString( nodePath + "output.params", "params.dat" );
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
	for ( int iTray = 1; iTray <= 120; iTray++ ){
		for ( int iMod = 1; iMod <= 32; iMod ++ ){
			for ( int iCell = 1; iCell <= 6; iCell ++ ){

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

				correction[ n ] = corr;
			}
		}
	}
}