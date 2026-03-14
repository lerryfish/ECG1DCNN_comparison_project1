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



int MoveDataDMA(u32 * source, u32 * destination, int num) {
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


	// Setup DMA Controller
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
//		print("Starting transfer through DMA in poll mode\r\n");
		DmaCmd.BD.SrcAddr = (u32)source;
		DmaCmd.BD.DstAddr = (u32)destination;
		DmaCmd.BD.Length = num * sizeof(int);

	// setting up for interrupt driven DMA


		if (debug_flag)
			print("Setting up interrupt system\r\n");

		Status = SetupIntrSystem(&Gic, &Dma);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

		Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

		XDmaPs_SetDoneHandler(&Dma,0,DmaDoneHandler,0);
		Status = XDmaPs_Start(&Dma, Channel, &DmaCmd, 0);	// release DMA buffer as we are done

		// reset timer
		XScuTimer_RestartTimer(TimerInstancePtr);

		while ((Done==0) & (Error==0));

		if (Error)
			print("Error occurred during DMA transfer\r\n");

		CntValue1 = XScuTimer_GetCounterValue(TimerInstancePtr);

		if (debug_flag)
			print("Transfer complete\r\n");
		// Disable the interrupt for the device
		XScuGic_Disable(&Gic, XPAR_XDMAPS_0_DONE_INTR_0);

		Error = 0;
		Done = 0;
		return CntValue1;
}

void gen_random_data(s32 *input, int gen_datanum)
{
    for (int i = 0; i < gen_datanum; i++)
    {
        input[i] = (rand() % 64) - 32;  // -32 ~ +31
    }
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


	// Setup DMA Controller
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
//		print("Starting transfer through DMA in poll mode\r\n");
		DmaCmd.BD.SrcAddr = (s32)source;
		DmaCmd.BD.DstAddr = (s32)destination;
		DmaCmd.BD.Length = num * sizeof(int);

	// setting up for interrupt driven DMA


		if (debug_flag)
			print("Setting up interrupt system\r\n");

		Status = SetupIntrSystem(&Gic, &Dma);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

		Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

		XDmaPs_SetDoneHandler(&Dma,0,DmaDoneHandler,0);
		Status = XDmaPs_Start(&Dma, Channel, &DmaCmd, 0);	// release DMA buffer as we are done

		// reset timer
		XScuTimer_RestartTimer(TimerInstancePtr);

		while ((Done==0) & (Error==0));

		if (Error)
			print("Error occurred during DMA transfer\r\n");

		CntValue1 = XScuTimer_GetCounterValue(TimerInstancePtr);

		if (debug_flag)
			print("Transfer complete\r\n");
		// Disable the interrupt for the device
		XScuGic_Disable(&Gic, XPAR_XDMAPS_0_DONE_INTR_0);

		Error = 0;
		Done = 0;
		return CntValue1;
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


	if (debug_flag)
		print("-- Memory Copy Performance test --\r\n");


	// =================================================================

	s32 *input, *output1, *output2, *output3, *output4; //
	s32 *bram_input, bram_output;
	input = (s32 *) DDR_MEMORY + 0x00001000;
	output1 = (s32 *) input + ( sizeof(s32) * 256 );
	output2 = (s32 *) output1 + ( sizeof(s32) * 1024);
	output3 = (s32 *) output2 + ( sizeof(s32) * 2048);
	output4 = (s32 *) output3 + ( sizeof(s32) * 2048);

	bram_input = (u32 *) BRAM_MEMORY0;
	bram_output = (u32 *) BRAM_MEMORY1;

	//prepare input data and bias
	srand(10);	// seed

	int repeat_time = 10;
	u64 average_tick = 0;
	for ( int i = 0 ; i < repeat_time ; i++) {	//repeat time
		gen_random_data(input, 256);
		printf("Times of input data. %d\n", i+1);
		for ( int i = 0 ; i < 256 ; i++ ) {
			printf("%d\n", input[i]);
		}

		Biasdata(output1, 1024, 1);
		Biasdata(output2, 2048, 2);
		Biasdata(output3, 2048, 3);
		Biasdata(output4, 2048, 4);

		XTime t0, t1;
		XTime_GetTime(&t0);
		// conv1 layer
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000004) ; // rst = 1
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000002) ; // data_in = 1
		MoveDataDMAS32(input, bram_input, 256);
		MoveDataDMAS32(output1, bram_output, 1024);
		//sleep(2) ;
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000001) ; // op_st = 1, mode = 00
		while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 4) == 0) {}
		MoveDataDMAS32(bram_output, output1, 512);
		//sleep(1) ;

		// conv2 layer
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000004) ; // rst = 1
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000002) ; // data_in = 1
		MoveDataDMAS32(output1, bram_input, 512);
		MoveDataDMAS32(output2, bram_output, 2048);
		//sleep(2) ;
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000009) ; // op_st = 1, mode = 01
		while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 4) == 0) {}
		MoveDataDMAS32(bram_output, output2, 1024);
		//sleep(1) ;

		// conv3 layer
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000004) ; // rst = 1
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000002) ; // data_in = 1
		MoveDataDMAS32(output2, bram_input, 1024);
		MoveDataDMAS32(output3, bram_output, 2048);
		//sleep(2) ;
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000011) ; // op_st = 1, mode = 10
		while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 4) == 0) {}
		MoveDataDMAS32(bram_output, output3, 1024);
		//sleep(1) ;

		// conv4 layer
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000004) ; // rst = 1
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000002) ; // data_in = 1
		MoveDataDMAS32(output3, bram_input, 1024);
		MoveDataDMAS32(output4, bram_output, 2048);
		//sleep(2) ;
		Xil_Out32(XPAR_MYIP_0_S00_AXI_BASEADDR, 0x00000019) ; // op_st = 1, mode = 11
		while(Xil_In32(XPAR_MYIP_0_S00_AXI_BASEADDR + 4) == 0) {}
		MoveDataDMAS32(bram_output, output4, 1024);
		//sleep(1) ;
		XTime_GetTime(&t1);

		u64 gticks1 = t1 - t0;
		u64 us1     = (gticks1 * 1000000ULL) / COUNTS_PER_SECOND;
		printf("Times of output data : %d . Cost Time : %llu us\n", i+1, us1);

		for ( int i = 0 ; i < 1024 ; i++) {
			printf("%d\n", output4[i]);
		}

		average_tick = average_tick + gticks1;
	}

	average_tick = average_tick / repeat_time;
	u64 average_cost_time = (average_tick * 1000000ULL) / COUNTS_PER_SECOND;
	printf("Average Cost time : %llu us\n", average_cost_time);

	printf("Finish\n");
    return 0;
} // main()
