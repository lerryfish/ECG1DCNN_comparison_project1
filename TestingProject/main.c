#include "xparameters.h"
#include "stdio.h"
#include "xil_exception.h"
#include "unistd.h"

#include "xuartps.h"	// if PS uart is used
#include "xscutimer.h"  // if PS Timer is used
#include "xdmaps.h"		// if PS DMA is used
#include "xscugic.h" 	// if PS GIC is used
#include "xil_exception.h"	// if interrupt is used
#include "xil_printf.h"

#include "stdlib.h"
#include "Bias.h"

#include "xtime_l.h"

//input data
#include "15class_signal_and_symbol_data_1000.h"
#include "Dense1_weight_bias.h"
#include "Dense2_weight_bias.h"

#define RESET_LOOP_COUNT	10	// Number of times to check reset is done
#define LENGTH 8192 // source and destination buffers lengths in number of words

#define OCM_MEMORY XPAR_PS7_OCMC_0_S_AXI_BASEADDR
#define BRAM_MEMORY0 XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR
#define BRAM_MEMORY1 XPAR_AXI_BRAM_CTRL_1_S_AXI_BASEADDR
#define DDR_MEMORY XPAR_PS7_DDR_0_S_AXI_BASEADDR+0x00020000 // pass all code and data sections

#define TIMER_DEVICE_ID	XPAR_SCUTIMER_DEVICE_ID
#define TIMER_LOAD_VALUE 0xFFFFFFFF
#define DMA0_ID XPAR_XDMAPS_1_DEVICE_ID
#define INTC_DEVICE_INT_ID XPAR_SCUGIC_SINGLE_DEVICE_ID

volatile static int Done = 0;	/* Dma transfer is done */
volatile static int Error = 0;	/* Dma Bus Error occurs */

int debug_flag = 1;		// 1 to print debug message, 0 ffor silent mode (profiling)

XUartPs Uart_PS;		/* Instance of the UART Device */
XScuTimer Timer;		/* Cortex A9 SCU Private Timer Instance */
XDmaPs Dma;				/* PS DMA */
XScuGic Gic;			/* PS GIC */

XScuTimer_Config *ConfigPtr;
XScuTimer *TimerInstancePtr = &Timer;

// PS Interrupt related definitions
XScuGic_Config *GicConfig;

XDmaPs_Config *DmaCfg;


//int DmaPs_Start(XDmaPs *InstPtr, unsigned int Channel,
//		  XDmaPs_Cmd *Cmd,
//		  int HoldDmaProg);

void DmaDoneHandler(unsigned int Channel,
		    XDmaPs_Cmd *DmaCmd,
		    void *CallbackRef)
{
	/* done handler */
  	Done = 1;
}

void DmaFaultHandler(unsigned int Channel,
		     XDmaPs_Cmd *DmaCmd,
		     void *CallbackRef)
{
	/* fault handler */

	Error = 1;
}

int SetupIntrSystem(XScuGic *GicPtr, XDmaPs *DmaPtr)
{
	int Status;

	Xil_ExceptionInit();

	// Connect the interrupt controller interrupt handler to the hardware
	// interrupt handling logic in the processor.
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
			     (Xil_ExceptionHandler)XScuGic_InterruptHandler,
			     GicPtr);

	// Connect a device driver handler that will be called when an interrupt
	// for the device occurs, the device driver handler performs the specific
	// interrupt processing for the device

	// Connect the Fault ISR
	Status = XScuGic_Connect(GicPtr,
				 XPAR_XDMAPS_0_FAULT_INTR,
				 (Xil_InterruptHandler)XDmaPs_FaultISR,
				 (void *)DmaPtr);
	if (Status != XST_SUCCESS)
		return XST_FAILURE;

	// Connect the Done ISR for channel 0 of DMA 0
	Status = XScuGic_Connect(GicPtr,
				 XPAR_XDMAPS_0_DONE_INTR_0,
				 (Xil_InterruptHandler)XDmaPs_DoneISR_0,
				 (void *)DmaPtr);

	if (Status != XST_SUCCESS)
		return XST_FAILURE;

	// Enable the interrupt for the device
	XScuGic_Enable(GicPtr, XPAR_XDMAPS_0_DONE_INTR_0);

	return XST_SUCCESS;
}

int MoveDataCPU(u32 * source, u32 * destination, int num) {
	volatile u32 CntValue1;
    int i;

	// reset timer
	XScuTimer_RestartTimer(TimerInstancePtr);

	// start moving data through the processor - no CDMA, no interrupt
	// gives base consumed cycles
	for (i=0; i<num; i++)
		*(destination+i) = *(source+i);

	CntValue1 = XScuTimer_GetCounterValue(TimerInstancePtr);

	return CntValue1;
}




void Biasdata(s32 *bias_data, int size, int which_layer){
	int addr = 0;
	if (which_layer == 1){
		int each_num = size / cv1B_num;
		for ( int i = 0 ; i < cv1B_num ; i++ ) {
			for ( int k = 0 ; k < each_num ; k++ ) {
				bias_data[addr+k] = cv1B[i];
			}
			addr = addr + each_num;
		}

		return;
	}

	else if ( which_layer == 2 ){
		int each_num = size / cv2B_num;
		for ( int i = 0 ; i < cv2B_num ; i++ ) {
			for ( int k = 0 ; k < each_num ; k++ ) {
				bias_data[addr+k] = cv2B[i];
			}
			addr = addr + each_num;
		}
		return;
	}

	else if ( which_layer == 3 ){
		int each_num = size / cv3B_num;
		for ( int i = 0 ; i < cv3B_num ; i++ ) {
			for ( int k = 0 ; k < each_num ; k++ ) {
				bias_data[addr+k] = cv3B[i];
			}
			addr = addr + each_num;
		}
		return;
	}

	else if ( which_layer == 4 ){
		int each_num = size / cv4B_num;
		for ( int i = 0 ; i < cv4B_num ; i++ ) {
			for ( int k = 0 ; k < each_num ; k++ ) {
				bias_data[addr+k] = cv4B[i];
			}
			addr = addr + each_num;
		}
		return;
	}

	else {
		printf("Prepare Bias Error.\n");
		return;
	}
}

int MoveDataDMAS32(s32 * source, s32 * destination, int num) {
    int Status;
	volatile u32 CntValue1;

	// PS DMA related definitions

	XDmaPs_Cmd DmaCmd = {
		.ChanCtrl = {
			.SrcBurstSize = 4,
			.SrcBurstLen = 4,
			.SrcInc = 1,		// increment source address
			.DstBurstSize = 4,
			.DstBurstLen = 4,
			.DstInc = 1,		// increment destination address
		},
	};
	unsigned int Channel = 0;

	DmaCmd.BD.SrcAddr = (s32)source;
	DmaCmd.BD.DstAddr = (s32)destination;
	DmaCmd.BD.Length = num * sizeof(int);

	Status = XDmaPs_Start(&Dma, Channel, &DmaCmd, 0);	// release DMA buffer as we are done

		// reset timer
	XScuTimer_RestartTimer(TimerInstancePtr);

	while ((Done==0) && (Error==0)){
		//xil_printf("Test DMA Done = %d\n", Done);
	}
	if (Error){
		print("Error occurred during DMA transfer\r\n");
	}
	CntValue1 = XScuTimer_GetCounterValue(TimerInstancePtr);

	if (debug_flag){
		//print("Transfer complete\r\n");
	}

	// Disable the interrupt for the device
	//XScuGic_Disable(&Gic, XPAR_XDMAPS_0_DONE_INTR_0);

	Error = 0;
	Done = 0;
	return CntValue1;
}

void Dense1(s32 *input, float *linear1_output) {

  for ( int i = 0 ; i < 128 ; i++ ) {
	  linear1_output[i] = 0.0 ;

	for ( int j = 0 ; j < 1024 ; j++ ){
		float temp = input[j] / 64.0;
		linear1_output[i] = linear1_output[i] + temp * dense128[i][j] ;
	}

	linear1_output[i] = linear1_output[i] + dense128_bias[i] ;

	if ( linear1_output[i] < 0.0 )
		linear1_output[i] = 0.0 ;

  } // for

} // Dense1()

void Dense2(float *input, float *linear2_output) {

  for ( int i = 0 ; i < 15 ; i++ ) {
	  linear2_output[i] = 0.0 ;
  	for ( int j = 0 ; j < 128 ; j++ ){
  		linear2_output[i] = linear2_output[i] + input[j] * dense15[i][j] ;
  	}
  	linear2_output[i] = linear2_output[i] + dense15_bias[i] ;

  } // for

} // Dense2()

int Predicted(float *linear2_output){
	int max = 0;
	for ( int i = 1 ; i < 15 ; i++ ) {
		if ( linear2_output[i] > linear2_output[max] ) {
			max = i;
		}
	}

	return max;
}

void Prepare_input_data(s32 *input, int index){
	int a = index %1000;
	int base_addr = a * 256 ;

	for ( int i = 0 ; i < 256 ; i++ ) {
		input[i] = test_signal_1000[base_addr+i];
	}
}

int main (void) {
	// UART related definitions
    int Status;
	XUartPs_Config *Config;

	// Initialize UART
	// Look up the configuration in the config table, then initialize it.
	Config = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
	if (NULL == Config) {
		return XST_FAILURE;
	}

	Status = XUartPs_CfgInitialize(&Uart_PS, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Initialize timer counter
	ConfigPtr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);

	Status = XScuTimer_CfgInitialize(TimerInstancePtr, ConfigPtr,
				 ConfigPtr->BaseAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Initialize GIC
	GicConfig = XScuGic_LookupConfig(INTC_DEVICE_INT_ID);
	if (NULL == GicConfig) {
		xil_printf("XScuGic_LookupConfig(%d) failed\r\n",
				INTC_DEVICE_INT_ID);
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(&Gic, GicConfig,
				       GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("XScuGic_CfgInitialize failed\r\n");
		return XST_FAILURE;
	}

	// Set options for timer/counter 0
	// Load the timer counter register.
	XScuTimer_LoadTimer(TimerInstancePtr, TIMER_LOAD_VALUE);

	// Start the Scu Private Timer device.
	XScuTimer_Start(TimerInstancePtr);



	// Setup DMA Controller
	if (debug_flag)
		print("-- Memory Copy Performance test --\r\n");

	DmaCfg = XDmaPs_LookupConfig(DMA0_ID);
	if (!DmaCfg) {
		xil_printf("Lookup DMAC %d failed\r\n", DMA0_ID);
		return XST_FAILURE;
	}

	Status = XDmaPs_CfgInitialize(&Dma,DmaCfg,DmaCfg->BaseAddress);
	if (Status) {
		xil_printf("XDmaPs_CfgInitialize failed\r\n");
		return XST_FAILURE;
	}
	// DMA in polling mode
	if (debug_flag)
		print("Setting up interrupt system\r\n");

	Status = SetupIntrSystem(&Gic, &Dma);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	XDmaPs_SetDoneHandler(&Dma,0,DmaDoneHandler,0);

	// =================================================================
	//Xil_DCacheDisable();
	s32 *input, *output1, *output2, *output3, *output4, *bias1, *bias2, *bias3, *bias4;
	s32 *bram_input, *bram_output;

	input = (s32 *) DDR_MEMORY + 0x000FFFFF;
	output1 = (s32 *) input + (256);
	output2 = (s32 *) output1 + (512);
	output3 = (s32 *) output2 + (1024);
	output4 = (s32 *) output3 + (1024);
	bias1 = (s32 *)output4 + (1024);
	bias2 = (s32 *)bias1 + (1024);
	bias3 = (s32 *)bias2 + (2048);
	bias4 = (s32 *)bias3 + (2048);

	Biasdata(bias1, 1024, 1);
	Biasdata(bias2, 2048, 2);
	Biasdata(bias3, 2048, 3);
	Biasdata(bias4, 2048, 4);


	float *linear1_output = (s32 *)bias4 + (2048 );
	float *linear2_output = (float *)linear1_output + (128);
	int *predicted_answer = (float *)linear2_output + (15);
	bram_input = (u32 *) BRAM_MEMORY0;
	bram_output = (u32 *) BRAM_MEMORY1;

	//prepare input data and bias
	srand(10);	// seed

	int repeat_time = 0;
	int test_data_num = 1000;
	u64 average_tick = 0;

    XTime tStart, tNow;
    XTime_GetTime(&tStart);
    //while (1) {	// testing base on time
    for ( int i = 0 ; i < test_data_num ; i++ ) { // testing base on data num
    	repeat_time = repeat_time + 1 ;
    	//xil_printf("Current index = %d\n", i);
    	/*
    	XTime_GetTime(&tNow);
    	double elapsed = (double)(tNow - tStart) / COUNTS_PER_SECOND;
        if (elapsed >= 1800.0) {
            break;
        }
        */

		//XTime t0, t1;
		//XTime_GetTime(&t0);
    	//Prepare_input_data(input, 189);
    	Prepare_input_data(input, i);
    	Xil_DCacheFlushRange( input, 256*4);

		// conv1 layer
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000004) ; // rst = 1
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000002) ; // data_in = 1, mode =00
		MoveDataDMAS32(input, bram_input, 256);					// transfer input data
		Xil_DCacheFlushRange( bias1, 1024*4);
		MoveDataDMAS32(bias1, bram_output, 1024);				// transfer bias
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000001) ; // op_st = 1, mode = 00
		while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 4) == 0) {}	// wait operation done
		MoveDataDMAS32(bram_output, output1, 512);					// take out output
		Xil_DCacheInvalidateRange( output1, 512*4);
		//sleep(1);

		// conv2 layer
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000004) ; // rst = 1
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x0000000A) ; // data_in = 1, mode = 01
		Xil_DCacheFlushRange( output1, 512*4);
		MoveDataDMAS32(output1, bram_input, 512);				// transfer input data
		Xil_DCacheFlushRange( bias2, 2048*4);
		MoveDataDMAS32(bias2, bram_output, 2048);				// transfer bias
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000009) ; // op_st = 1, mode = 01
		while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 4) == 0) {} // wait operation done
		MoveDataDMAS32(bram_output, output2, 1024);					// take out output
		Xil_DCacheInvalidateRange( output2, 1024*4);
		//sleep(1);

		// conv3 layer
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000004) ; // rst = 1
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000012) ; // data_in = 1, mode = 10
		Xil_DCacheFlushRange( output2, 1024*4);
		MoveDataDMAS32(output2, bram_input, 1024);				// transfer input data
		Xil_DCacheFlushRange( bias3, 2048*4);
		MoveDataDMAS32(bias3, bram_output, 2048);				// transfer bias
		//while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 8) == 0) {} // wait input and bias prepare done
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000011) ; // op_st = 1, mode = 10
		while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 4) == 0) {}
		MoveDataDMAS32(bram_output, output3, 1024);
		Xil_DCacheInvalidateRange( output3, 1024*4);
		//sleep(1);

		// conv4 layer
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000004) ; // rst = 1
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x0000001A) ; // data_in = 1, mode = 11
		Xil_DCacheFlushRange( output3, 1024*4);
		MoveDataDMAS32(output3, bram_input, 1024);				// transfer input data
		Xil_DCacheFlushRange( bias4, 2048*4);
		MoveDataDMAS32(bias4, bram_output, 2048);				// transfer bias
		//while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 8) == 0) {} // wait input and bias prepare done
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000019) ; // op_st = 1, mode = 11
		while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 4) == 0) {}
		MoveDataDMAS32(bram_output, output4, 1024);
		Xil_DCacheInvalidateRange( output4, 1024*4);
		//sleep(1);

		Dense1(output4, linear1_output);
		Dense2(linear1_output, linear2_output);
		int predicted = Predicted(linear2_output);
		predicted_answer[i] = predicted;	//use to check answer

		//XTime_GetTime(&t1);

		//u64 gticks1 = t1 - t0;
		//u64 us1     = (gticks1 * 1000000ULL) / COUNTS_PER_SECOND;
		//printf("Times of output data : %d . Cost Time : %llu us\n", i+1, us1);


		//average_tick = average_tick + gticks1;


	}// end for // end while

	//average_tick = average_tick / repeat_time;
	//u64 average_cost_time = (average_tick * 1000000ULL) / COUNTS_PER_SECOND;
	//printf("Average Cost time : %llu us\n", average_cost_time);

    // use to check answer **********************

    xil_printf("Repeat time = %d\n", repeat_time);
    int fail_predict = 0;
    for ( int i = 0 ; i < test_data_num ; i++ ) {
    	int index = i % 1000;
    	if ( predicted_answer[i] != test_symbol_1000[index] ){
    		xil_printf("Predicted = %d, Answer = %d, Index = %d\n", predicted_answer[i], test_symbol_1000[index], i);
    		fail_predict++;
    	}
    }
    xil_printf("Fail predict num = %d\n", fail_predict);
    xil_printf("Finish.\n");

    return 0;
} // main()
