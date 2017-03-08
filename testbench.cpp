#include "testbench.h"

void testbench::send_cthread(){
	wait();  //initial wait is important to synchronize cthreads
	int i,j,a,b; //index variables
	DCTflag = false;
	IDCTflag = false;
	bool stopDCTflag;
	stopDCTflag = false;

	//Initialization of signal
	for (i=0; i<8; i++){
		for (j=0; j<8; j++){
			Image_in[i][j]->write(0);}}

	//allocate dynamic memory for Image Data
	int** ImgData = new int*[504];
	for(i=0;i<504;++i){
		ImgData[i]= new int[504];}

	//Read image data from the file (already zero-padded to be divisible into 8x8 blocks)
	//Source: http://stackoverflow.com/questions/26979236/reading-integers-from-file-and-store-them-in-array-c as our source for how to do this
	ifstream myfile("tophatpup.txt");
	for(i=0;i<504;++i){
		for(j=0;j<504;++j){if(myfile >> a) ImgData[i][j]=a;} //read image in
	}
	myfile.close();

	//Center at 0 by subtracting 128 from each value
	for(i=0;i<504;++i){
		for(j=0;j<504;++j){
			ImgData[i][j]-=128;
		}
	}

	//write the image data to the port so that it may be sent as a signal
	for(i=0;i<504;i+=8){ //increment by 8 (rows) over the full 504x504 image
		for(j=0;j<504;j+=8){ //increment by 8 (columns)
			wait(); //synchronize before reading
			stopDCTflag = stopDCT->read(); //check to determine whether or not the DCT is still busy
			if(stopDCTflag){
				startDCT->write(false);
				DCTflag = false;
				wait();
			}

			if((DCTflag || IDCTflag)&&(stopDCTflag==false)){
				wait();//wait until all flags are 0
			}
			else{ //this will ONLY execute if IDCT/DCT flags are 0, and stopDCT is true
				for(int a=0;a<8;a++){
					for(b=0;b<8;b++){
						Image_in[a][b]->write(ImgData[a+i][b+j]);
					}
				}
				r=i; c=j; //index of starting points for 8x8 block (used in receive_cthread for reconstruction)
				startDCT->write(true); //change flag to begin DCT
				wait(); //synchronize with writing of flag signal
				wait(); //synchronize
			}
		}
	}
	for(i=0;i<8;++i){
		wait(); //wait 8 cycles to make sure DCT and IDCT have finished performing for the final 8x8 block
	}
	//free allocated dynamic memory
	for(i=0;i<504;++i){
		delete[] ImgData[i];
	} delete [] ImgData;

	sc_stop();
}

//read the testbench input (what we receive from the uut, in our case we are checking the final result of the simulation received fromt the IDCT module)
void testbench::receive_cthread(){
	wait();  //initial wait is important to synchronize cthreads
	int i,j,k,counter;  //index and counter variables

	//Declaration of IDCT variables
	sc_fixed<35, 11> tC[8][8]; //transpose of C matrix
	sc_fixed<35, 11> Y[8][8]; //result from DCT thread
	sc_fixed<35, 11> tCY[8][8]; //tC*Y
	sc_fixed<35, 11> idct_f[8][8]; //fixed point matrix of idct results
	sc_int<11> idct[8][8]; //integer matrix of idct results

	//declaration of kernel C
	sc_fixed<35, 11> C[8][8] = {{0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906,0.3535533906},
			{0.4903926402, 0.4157348062, 0.2777851165, 0.09754516101, -0.09754516101, -0.2777851165, -0.4157348062, -0.4903926402},
			{0.4619397663, 0.1913417162, -0.1913417162, -0.4619397663, -0.4619397663, -0.1913417162, 0.1913417162, 0.4619397663},
			{0.4157348062, -0.09754516101, -0.4903926402, -0.2777851165, 0.2777851165, 0.4903926402, 0.09754516101, -0.4157348062},
			{0.3535533906, -0.3535533906, -0.3535533906, 0.3535533906, 0.3535533906, -0.3535533906, -0.3535533906, 0.3535533906},
			{0.2777851165, -0.4903926402, 0.09754516101, 0.4157348062, -0.4157348062, -0.09754516101, 0.4903926402, -0.2777851165},
			{0.1913417162, -0.4619397663, 0.4619397663, -0.1913417162, -0.1913417162, 0.4619397663, -0.4619397663, 0.1913417162},
			{0.09754516101, -0.2777851165, 0.4157348062, -0.4903926402, 0.4903926402, -0.4157348062, 0.2777851165, -0.09754516101}};

	//declare and create the transpose matrix of kernel C
	for(int i=0;i<8;++i){for(int j=0;j<8;++j){tC[i][j]=C[j][i]; }}

	//use dynamic memory to rebuild IDCT
	sc_int<12>** aux = new sc_int<12>*[504];
	for(int i=0;i<504;++i){
		aux[i]= new sc_int<12>[504];}

	wait();
	IDCTflag=startIDCT->read();
	//loop to repetitive execution of IDCT (it should execute (504/8)*(504/8) times)
	while(counter<3969){
		wait(); //synchronize before reading -- should synchronize startIDCT and DCToutput signals
		wait();
		wait();
		IDCTflag=stopDCT->read();
		DCTflag=startDCT->read();
		if(IDCTflag==true){
			//read result of DCT
			for(i=0;i<8;++i){
				for(j=0;j<8;++j){
					Y[i][j]=DCToutput[i][j]->read();
					//initialize all IDCT 8x8 blocks to 0 (to avoid building on prev. values)
					tCY[i][j]=0;
					idct_f[i][j]=0;
				}
			}
			//Perform IDCT
			//multiply tC*Y
			for(i = 0; i<8; ++i){for(j = 0; j<8; ++j){for(k = 0; k<8; ++k){
				tCY[i][j] += (tC[i][k] * Y[k][j]);}}}

			//multiply (tC*Y)*C
			for(i=0; i<8; ++i){
				for(j = 0; j<8; ++j){
					for(k = 0; k<8; ++k){
						idct_f[i][j] += (tCY[i][k] * C[k][j]);
					}
					aux[r+i][c+j]=idct_f[i][j]+128; //rebuild IDCT into 504x504
				}
			}
			IDCTflag=false; //reset this flag variable to wait for next dct block
			DCTflag=false; //reset DCT flag
			counter++; //increment counter
			wait();
		}
	}
	//write the IDCT result in a file
	ofstream myFileResult ("IDCTresult.txt");
	for(i=0;i<504;++i)
	{
		for(j=0;j<504;++j)
			if(myFileResult)
				myFileResult << aux[i][j] <<"\t" ;
		myFileResult << "\n";
	}

	//free allocated dynamic memory
	for(int i=0;i<504;++i){
		delete[] aux[i];
	} delete [] aux;
}
