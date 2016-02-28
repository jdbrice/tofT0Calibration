
#include "Logger.h"
#include "XmlConfig.h"
using namespace jdb;

#include <iostream>
#include <exception>

#include "T0Calib.h"

int main( int argc, char* argv[] ) {

	Logger::setGlobalLogLevel( "Trace" );

	if ( argc >= 2 ){

		string fileList = "";
		string jobPrefix = "";

		// try block to load the XML config if given
		try{
			XmlConfig config( argv[ 1 ] );
			
			// if more arguments are given they will be used for 
			// parallel job running with examples of
			// argv[ 2 ] = fileList.lis
			// argv[ 3 ] = jobPrefix_ 
			if ( argc >= 4){
				fileList = (string) argv[ 2 ];
				jobPrefix = (string) argv[ 3 ];
			}

			T0Calib calib;
			calib.init( config, "T0Calib", -1 );
			calib.run();


		} catch ( exception &e ){
			cout << e.what() << endl;
		}

	}

	return 0;
}
