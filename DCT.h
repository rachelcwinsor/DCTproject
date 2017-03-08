#define SC_INCLUDE_FX
#include <systemc.h>
#include <stdio.h>

//the DCT module will perform the DCT on the 8 by 8 blocks from a bitmap image matrix 504 by 504



SC_MODULE(DCT){
	//Port declarations:
	sc_in< bool > clk;
	sc_in< bool > rst;
	sc_in< sc_int<10> > Image_in[8][8];   //8 by 8 blocks of the original image matrix received from the testbench module (in the simulation)
	sc_out< sc_fixed<35, 11> > DctImage_out[8][8]; //8 by 8 blocks of the matrix resulting from the DCT function will be sent to the Testbench module to perform the IDCT
	sc_in< bool > startDCT;         // to synchronize the data exchange between the DCT module and the Testbench module and start the DCT
	sc_out< bool > startIDCT;       // to start processing the IDCT in the Testbench module
	sc_out < bool > stopDCT;

	//Process declarations:
	void dct_process();

	// DCT function prototype:
	void doDCT(sc_fixed<35, 11> C[8][8], sc_int<10> Image[8][8], sc_fixed<35, 11> tC[8][8], sc_fixed<35, 11> DCT_int[8][8]);

	// module variables
	sc_int<10> image[8][8];
	sc_fixed<35, 11> DCT_int[8][8];
	sc_fixed<35, 11> tC[8][8];   //the transpose of the C Kernel Matrix

	SC_CTOR(DCT)
	{
		SC_CTHREAD(dct_process, clk.pos());  //clk.positive (reset working on the rising edge of the clock)
		reset_signal_is(rst,true); //reseteo siempre síncrono (el reset no lo hemos incluido en la lista de sensibilidad, se resetea el primer ciclo de reloj)
	}
};


