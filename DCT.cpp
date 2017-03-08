#include "DCT.h"

void DCT::dct_process() {
	wait();  //initial wait is important to synchronize cthreads
	int counter; //CONTROL: the DCT will perform (504/8)*(504/8) times only
	bool DCTflag; //CONTROL: this flag tells us when to begin the DCT by reading the input signal from the send thread
	stopDCT->write(false); //we must initially set the stopDCT to false, both for the DCT thread, and the IDCT thread

	wait(); //synchronized with the SEND thread

	//declare the kernel C matrix (derived from MATLAB)
	sc_fixed<35, 11> C[8][8] = {{0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906,0.3535533906},
			{0.4903926402, 0.4157348062, 0.2777851165, 0.09754516101, -0.09754516101, -0.2777851165, -0.4157348062, -0.4903926402},
			{0.4619397663, 0.1913417162, -0.1913417162, -0.4619397663, -0.4619397663, -0.1913417162, 0.1913417162, 0.4619397663},
			{0.4157348062, -0.09754516101, -0.4903926402, -0.2777851165, 0.2777851165, 0.4903926402, 0.09754516101, -0.4157348062},
			{0.3535533906, -0.3535533906, -0.3535533906, 0.3535533906, 0.3535533906, -0.3535533906, -0.3535533906, 0.3535533906},
			{0.2777851165, -0.4903926402, 0.09754516101, 0.4157348062, -0.4157348062, -0.09754516101, 0.4903926402, -0.2777851165},
			{0.1913417162, -0.4619397663, 0.4619397663, -0.1913417162, -0.1913417162, 0.4619397663, -0.4619397663, 0.1913417162},
			{0.09754516101, -0.2777851165, 0.4157348062, -0.4903926402, 0.4903926402, -0.4157348062, 0.2777851165, -0.09754516101}};
	//fill the transpose matrix of C - to be used in DCT calculation
	for(int i=0; i<8; ++i){for(int j=0; j<8; ++j){tC[i][j]=C[j][i];}}

	//Infinite Loop will perform until the simulation is stopped (sc_stop) by any CTHREAD
	for(;;){

		//reset initialization each time in order to avoid errors
		for (int i=0; i<8; ++i){for (int j=0; j<8; ++j){DctImage_out [i][j]-> write (false);}}

		wait(); //synchronization before read - should synchronize startDCT and Image_in signals
		DCTflag = startDCT->read(); //read flag that tells us whether or not to start the DCT - sent from SEND thread

		if((DCTflag==true) && counter<3969){ //if we are told to start AND we have not already performed all DCTs

			for(int i=0;i<8;++i){
				for(int j=0;j<8;++j){
					image[i][j]=Image_in[i][j]->read(); //read the value of the Input port received from testbench into variable image
				}
			}

			doDCT(C, image, tC, DCT_int); //perform DCT functionality

			//Write the result in the output port
			for(int i=0;i<8;++i){
				for(int j=0;j<8;++j){
					DctImage_out[i][j]->write(DCT_int[i][j]); //Write the result to the output port - send to RECEIVE thread for IDCT calculations
				}
			}
			startIDCT->write(true); //synchronize the data exchange between the two modules and the begining of the IDCT in the testbench module
			stopDCT->write(true); //important in DCT and RECEIVE threads as a CONTROL
			wait(); //synchronization of writing startIDCT and DCT_int signals
			wait(); //synchronization with SEND/RECEIVE threads
			DCTflag=false; //reset flag
			counter++; //increment counter
		}
	}
}

// DCT function implementation  Y = C * X * tC
void DCT::doDCT(sc_fixed<35, 11> C[8][8], sc_int<10> Image[8][8], sc_fixed<35, 11> tC[8][8], sc_fixed<35, 11> DCT_result[8][8]){
	int i,j,k; //index values for our multiplication
	sc_fixed<35, 11> CX[8][8]; //create intermediate array for our intermediate image (C*X)
	for(i=0;i<8;++i){for(j=0;j<8;++j){DCT_result[i][j]=0;}} //Initialize DCT_result to avoid building on previously calculated DCTs

	//multiply C*X
	for(i = 0; i<8; ++i){for(j = 0; j<8; ++j){for(k = 0; k<8; ++k){
		CX[i][j] += (C[i][k] * Image[k][j]);}}}

	//finish DCT by multiplying intermediate array by transpose of C (CX*tC)
	for(i = 0; i<8; ++i){for(j = 0; j<8; ++j){for(k = 0; k<8; ++k){
		DCT_result[i][j] += (CX[i][k] * tC[k][j]);}}}
}
