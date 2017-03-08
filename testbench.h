#define SC_INCLUDE_FX
#include <systemc.h>
#include <stdio.h>
#include <fstream>     //C++ library (file stream,) used to read and write in files
#include <iostream>


//in the testbench module we read the bitmap image matrix 504 by 504 from a text file, and decompose it in 8 by 8 blocks in order to send it to the DCT module
//we also read the result of the DCT from the DCT module and perform the IDCT in order to check the functionality of the DCT UUT before synthetize it


SC_MODULE(testbench){
	sc_in< bool > clk;
	sc_in< bool > rst;
	sc_in< bool > startIDCT;
	sc_in< sc_fixed<35, 11> > DCToutput[8][8];
	sc_in < bool > stopDCT;

	sc_out< sc_int<10> > Image_in[8][8];     //the output of the testbench module is the input of the DCT module (in the testbench, we give values to the input)
	sc_out< bool > startDCT;

	//to rebuild WITHIN testbench
	sc_int<12> r;
	sc_int<12> c;
	bool DCTflag; //If DCTflag==1, perform DCT
	bool IDCTflag; //If IDCTflag==1, perform IDCT

	void send_cthread();
	void receive_cthread();

	//dos procesos, uno para introducir datos y otro para recogerlos
	SC_CTOR(testbench) {

		SC_CTHREAD(send_cthread, clk.pos());  //give values to the input
		reset_signal_is(rst,true);

		SC_CTHREAD(receive_cthread, clk.pos());  //check the result of the simulation
		reset_signal_is(rst,true);
	}
};
