#define SC_INCLUDE_FX
#include <systemc.h>
#include "DCT.h"
#include "testbench.h"

int sc_main(int argc, char* argv[]) {
	//module and signals instantiations:
	DCT uut("uut"); //Synthesizable module
	testbench tb("tb"); //Testbench module

	sc_clock clk("clk", 10, SC_NS, 0.5, 5, SC_NS, true); //clk and rst instances
	sc_signal< bool > rst;
	sc_signal< sc_int<10> > Image_signal[8][8]; //OUTPUT of SEND_CTHREAD INTO DCT_CTHREAD
	sc_signal< sc_fixed<35, 11> > DctImage_signal[8][8]; //OUTPUT of DCT_CTHREAD INTO RECEIVE_CTHREAD
	sc_signal< bool > startDCT_signal; //Control signal
	sc_signal< bool > startIDCT_signal; //Control signal
	sc_signal< bool > stopDCT_signal; //Control signal

	//DCT module
	uut.clk(clk); //synchronize clocks
	uut.rst(rst); //synchronize reset
	uut.startDCT(startDCT_signal); //begin DCT
	uut.startIDCT(startIDCT_signal); //begin IDCT by entering RECEIVE thread
	uut.stopDCT(stopDCT_signal); //end DCT
	for (int i=0;i<8;i++){
		for(int j=0;j<8;j++){
			uut.Image_in[i][j](Image_signal[i][j]);}} //8x8 image received from SEND thread
	for (int i=0; i<8; i++){
		for (int j=0; j<8; j++){
			uut.DctImage_out[i][j](DctImage_signal[i][j]);}} //Resulting 8x8 DCT sent to RECEIVE thread

	//TestBench module
	tb.clk(clk); //synchronize clocks
	tb.rst(rst); //synchronize reset
	tb.startDCT(startDCT_signal); //begin DCT by entering DCT thread
	tb.startIDCT(startIDCT_signal); //begin IDCT
	tb.stopDCT(stopDCT_signal); //the DCT is complete

	for (int i=0; i<8; i++){
		for (int j=0; j<8; j++){
			tb.Image_in[i][j](Image_signal[i][j]);}} //8x8 block of image data sent to DCT thread
	for (int i=0; i<8; i++){
		for (int j=0; j<8; j++){
			tb.DCToutput[i][j](DctImage_signal[i][j]);}} //8x8 block of DCT data received from DCT thread

	sc_start(); //begin simulation
	return 0;
}
