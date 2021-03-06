/****************************************************************
 NAME: u2440mon.c
 DESC: u2440mon entry point,menu,download
 HISTORY:
 Mar.25.2002:purnnamu: S3C2400X profile.c is ported for S3C2410X.
 Mar.27.2002:purnnamu: DMA is enabled.
 Apr.01.2002:purnnamu: isDownloadReady flag is added.
 Apr.10.2002:purnnamu: - Selecting menu is available in the waiting loop. 
                         So, isDownloadReady flag gets not needed
                       - UART ch.1 can be selected for the console.
 Aug.20.2002:purnnamu: revision number change 0.2 -> R1.1       
 Sep.03.2002:purnnamu: To remove the power noise in the USB signal, the unused CLKOUT0,1 is disabled.
 ****************************************************************/

#include <stdlib.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"
#include "mmu.h"
#include "profile.h"
#include "memtest.h"

#include "usbmain.h"
#include "usbout.h"
#include "usblib.h"
#include "2440usb.h"

void Isr_Init(void);
void HaltUndef(void);
void HaltSwi(void);
void HaltPabort(void);
void HaltDabort(void);
void Lcd_Off(void);
void WaitDownload(void);
void Menu(void);
void ClearMemory(void);


void Clk0_Enable(int clock_sel);	
void Clk1_Enable(int clock_sel);
void Clk0_Disable(void);
void Clk1_Disable(void);

//#define DOWNLOAD_ADDRESS _RAM_STARTADDRESS
volatile U32 downloadAddress;

void (*restart)(void)=(void (*)(void))0x0;
void (*run)(void);


volatile unsigned char *downPt;
volatile U32 downloadFileSize;
volatile U16 checkSum;
volatile unsigned int err=0;
volatile U32 totalDmaCount;

volatile int isUsbdSetConfiguration;

int download_run=0;
U32 tempDownloadAddress;
int menuUsed=0;

//extern char Image$$RW$$Limit[];
U32 *pMagicNum = 0;//=(U32 *)Image$$RW$$Limit;
int consoleNum;

int main(void)
{
	char *mode;
	int i;
//	U8 key;
	U32 mpll_val, divn_upll=0;
    

	Port_Init();
	// USB device detection control
	rGPGCON &= ~(3<<24);
	rGPGCON |=  (1<<24); // output
	rGPGUP  |=  (1<<12); // pullup disable
	rGPGDAT |=  (1<<12); // output	
	

	ChangeUPllValue(60,4,2);		// 48MHz
	for(i=0; i<7; i++);
//	ChangeClockDivider(13,12);
//	ChangeMPllValue(97,1,2);		//296Mhz

	Isr_Init();

	consoleNum=1;		// Uart 1 select for debug.
	Uart_Init(0,115200);
	Uart_Select(0);

	rMISCCR=rMISCCR&~(1<<3); // USBD is selected instead of USBH1 
	rMISCCR=rMISCCR&~(1<<13); // USB port 1 is enabled.


//
//  USBD should be initialized first of all.
//
	isUsbdSetConfiguration=0;

#if 0
	UsbdMain(); 
	MMU_Init(); //MMU should be reconfigured or turned off for the debugger, 
	//After downloading, MMU should be turned off for the MMU based program,such as WinCE.	
#else
//	MMU_EnableICache();  
//	UsbdMain(); 
#endif

	Delay(0);  //calibrate Delay()
	
	pISR_SWI=(_ISR_STARTADDRESS+0xf0);	//for pSOS

	Led_Display(0x6);

#if USBDMA
	mode="DMA";
#else
	mode="Int";
#endif

	// CLKOUT0/1 select.
	//Uart_Printf("CLKOUT0:MPLL in, CLKOUT1:RTC clock.\n");
	//Clk0_Enable(0);	// 0:MPLLin, 1:UPLL, 2:FCLK, 3:HCLK, 4:PCLK, 5:DCLK0
	//Clk1_Enable(2);	// 0:MPLLout, 1:UPLL, 2:RTC, 3:HCLK, 4:PCLK, 5:DCLK1	
	Clk0_Disable();
	Clk1_Disable();
	
	mpll_val = rMPLLCON;
	Uart_Printf("DIVN_UPLL%x\n", divn_upll);
	Uart_Printf("MPLLVal [M:%xh,P:%xh,S:%xh]\n", (mpll_val&(0xff<<12))>>12,(mpll_val&(0x3f<<4))>>4,(mpll_val&0x3));
	Uart_Printf("CLKDIVN:%xh\n", rCLKDIVN);

	Uart_Printf("\n\n");
	Uart_Printf("+---------------------------------------------+\n");
	Uart_Printf("| S3C2440A USB Downloader ver R0.03 2004 Jan  |\n");
	Uart_Printf("+---------------------------------------------+\n");
	Uart_Printf("FCLK=%4.1fMHz,%s mode\n",FCLK/1000000.,mode); 
	Uart_Printf("USB: IN_ENDPOINT:1 OUT_ENDPOINT:3\n"); 
	Uart_Printf("FORMAT: <ADDR(DATA):4>+<SIZE(n+10):4>+<DATA:n>+<CS:2>\n");
	Uart_Printf("NOTE: 1. Power off/on or press the reset button for 1 sec\n");
	Uart_Printf("		 in order to get a valid USB device address.\n");
	Uart_Printf("	  2. For additional menu, Press any key. \n");
	Uart_Printf("\n");

	download_run=1; //The default menu is the Download & Run mode.

	while(1)
	{
		if(menuUsed==0)Menu();
		WaitDownload();	
	}

}



void Menu(void)
{
//	int i;
	U8 key;
	menuUsed=1;
	while(1)
	{
		Uart_Printf("\n###### Select Menu ######\n");
		Uart_Printf(" [0] Download & Run\n");
		Uart_Printf(" [1] Download Only\n");
		Uart_Printf(" [2] Test SDRAM \n");
		Uart_Printf(" [3] Change The Console UART Ch.\n");
		Uart_Printf(" [4] Clear unused area in SDRAM \n");		
		key=Uart_Getch();
		
		switch(key)
		{
		case '0':
			Uart_Printf("\nDownload&Run is selected.\n\n");
			download_run=1;
			return;
		case '1':
			Uart_Printf("\nDownload Only is selected.\n");
			Uart_Printf("Enter a new temporary download address(0x3...):");
			tempDownloadAddress=Uart_GetIntNum();
			download_run=0;
			Uart_Printf("The temporary download address is 0x%x.\n\n",tempDownloadAddress);
			return;
		case '2':
			Uart_Printf("\nMemory Test is selected.\n");
		MemoryTest();
		Menu();
		return;
//			break;
		case '3':
			Uart_Printf("\nWhich UART channel do you want to use for the console?[0/1]\n");
			if(Uart_Getch()!='1')
			{
			*pMagicNum=0x0;
		Uart_Printf("UART ch.0 will be used for console at next boot.\n");					
		}
		else
		{
			*pMagicNum=0x12345678;
 		Uart_Printf("UART ch.1 will be used for console at next boot.\n");		
				Uart_Printf("UART ch.0 will be used after long power-off.\n");
		}
			Uart_Printf("System is waiting for a reset. Please, Reboot!!!\n");
			while(1);
//			break;
		case '4':
			Uart_Printf("\nMemory clear is selected.\n");
			ClearMemory();
		break;
		default:
			break;
	}	
	}		

}



void WaitDownload(void)
{
	U32 i;
	U32 j;
	U16 cs;
	U32 temp;
	U16 dnCS;
	int first=1;
	float time;
	U8 tempMem[16];
	U8 key;
	
	checkSum=0;
	downloadAddress=(U32)tempMem; //_RAM_STARTADDRESS; 
	downPt=(unsigned char *)downloadAddress;
	//This address is used for receiving first 8 byte.
	downloadFileSize=0;
	
#if 0
	MMU_DisableICache(); 
		//For multi-ICE. 
		//If ICache is not turned-off, debugging is started with ICache-on.
#endif

	/*******************************/
	/*	Test program download	*/
	/*******************************/
	j=0;

	if(isUsbdSetConfiguration==0)
	{
	Uart_Printf("USB host is not connected yet.\n");
	}

	while(downloadFileSize==0)
	{
		if(first==1 && isUsbdSetConfiguration!=0)
		{
			Uart_Printf("USB host is connected. Waiting a download.\n");
			first=0;
		}

	if(j%0x50000==0)Led_Display(0x6);
	if(j%0x50000==0x28000)Led_Display(0x9);
	j++;

	key=Uart_GetKey();
	if(key!=0)
	{
		Menu();
			first=1; //To display the message,"USB host ...."
	}

	}

	Timer_InitEx();	  
	Timer_StartEx();  

#if USBDMA	

	rINTMSK&=~(BIT_DMA2);  

	ClearEp3OutPktReady(); 
		// indicate the first packit is processed.
		// has been delayed for DMA2 cofiguration.

	if(downloadFileSize>EP3_PKT_SIZE)
	{
		if(downloadFileSize<=(0x80000))
		{
	  		ConfigEp3DmaMode(downloadAddress+EP3_PKT_SIZE-8,downloadFileSize-EP3_PKT_SIZE);	
 
	  		//will not be used.
/*	   rDIDST2=(downloadAddress+downloadFileSize-EP3_PKT_SIZE);  
		   rDIDSTC2=(0<<1)|(0<<0);  
		rDCON2=rDCON2&~(0xfffff)|(0);				
*/
		}
	  	else
	  	{
	  		ConfigEp3DmaMode(downloadAddress+EP3_PKT_SIZE-8,0x80000-EP3_PKT_SIZE);
	   		
			if(downloadFileSize>(0x80000*2))//for 1st autoreload
			{
				rDIDST2=(downloadAddress+0x80000-8);  //for 1st autoreload.
			 rDIDSTC2=(1<<2)|(0<<1)|(0<<0);  
				rDCON2=rDCON2&~(0xfffff)|(0x80000);			  

  		while(rEP3_DMA_TTC<0xfffff)
  		{
  			rEP3_DMA_TTC_L=0xff; 
  			rEP3_DMA_TTC_M=0xff;
  			rEP3_DMA_TTC_H=0xf;
  		}
			}	
 		else
 		{
 			rDIDST2=(downloadAddress+0x80000-8);  //for 1st autoreload.
	  			rDIDSTC2=(1<<2)|(0<<1)|(0<<0);  
 			rDCON2=rDCON2&~(0xfffff)|(downloadFileSize-0x80000); 		

  		while(rEP3_DMA_TTC<0xfffff)
  		{
  			rEP3_DMA_TTC_L=0xff; 
  			rEP3_DMA_TTC_M=0xff;
  			rEP3_DMA_TTC_H=0xf;
  		}
		}
	}
 	totalDmaCount=0;
	}
	else
	{
	totalDmaCount=downloadFileSize;
	}
#endif

	Uart_Printf("\nNow, Downloading [ADDRESS:%xh,TOTAL:%d]\n",
			downloadAddress,downloadFileSize);
	Uart_Printf("RECEIVED FILE SIZE:%8d",0);
   
#if USBDMA	
	j=0x10000;

	while(1)
	{
		if( (rDCDST2-(U32)downloadAddress+8)>=j)
	{
		Uart_Printf("\b\b\b\b\b\b\b\b%8d",j);
   		j+=0x10000;
		}
	if(totalDmaCount>=downloadFileSize)break;
	}

#else
	j=0x10000;

	while(((U32)downPt-downloadAddress)<(downloadFileSize-8))
	{
	if( ((U32)downPt-downloadAddress)>=j)
	{
		Uart_Printf("\b\b\b\b\b\b\b\b%8d",j);
   		j+=0x10000;
	}
	}
#endif

	time=Timer_StopEx();
	
	Uart_Printf("\b\b\b\b\b\b\b\b%8d",downloadFileSize);	
	Uart_Printf("\n(%5.1fKB/S,%3.1fS)\n",(float)(downloadFileSize/time/1000.),time);
	
#if USBDMA	
	/*******************************/
	/*	 Verify check sum		*/
	/*******************************/

	Uart_Printf("Now, Checksum calculation\n");

	cs=0;	
	i=(downloadAddress);
	j=(downloadAddress+downloadFileSize-10)&0xfffffffc;
	while(i<j)
	{
		temp=*((U32 *)i);
		i+=4;
		cs+=(U16)(temp&0xff);
		cs+=(U16)((temp&0xff00)>>8);
		cs+=(U16)((temp&0xff0000)>>16);
		cs+=(U16)((temp&0xff000000)>>24);
	}

	i=(downloadAddress+downloadFileSize-10)&0xfffffffc;
	j=(downloadAddress+downloadFileSize-10);
	while(i<j)
	{
  	cs+=*((U8 *)i++);
	}
	
	checkSum=cs;
#else
	//checkSum was calculated including dnCS. So, dnCS should be subtracted.
	checkSum=checkSum - *((unsigned char *)(downloadAddress+downloadFileSize-8-2))
		 - *( (unsigned char *)(downloadAddress+downloadFileSize-8-1) );	
#endif	  

	dnCS=*((unsigned char *)(downloadAddress+downloadFileSize-8-2))+
	(*( (unsigned char *)(downloadAddress+downloadFileSize-8-1) )<<8);

	if(checkSum!=dnCS)
	{
	Uart_Printf("Checksum Error!!! MEM:%x DN:%x\n",checkSum,dnCS);
	return;
	}

	Uart_Printf("Download O.K.\n\n");
	Uart_TxEmpty(consoleNum);


	if(download_run==1)
	{
		rINTMSK=BIT_ALLMSK;
		run=(void (*)(void))downloadAddress;
	run();
	}
}




void Isr_Init(void)
{
	pISR_UNDEF=(unsigned)HaltUndef;
	pISR_SWI  =(unsigned)HaltSwi;
	pISR_PABORT=(unsigned)HaltPabort;
	pISR_DABORT=(unsigned)HaltDabort;
	rINTMOD=0x0;	  // All=IRQ mode
	rINTMSK=BIT_ALLMSK;	  // All interrupt is masked.

	//pISR_URXD0=(unsigned)Uart0_RxInt;	
	//rINTMSK=~(BIT_URXD0);   //enable UART0 RX Default value=0xffffffff

#if 1
	pISR_USBD =(unsigned)IsrUsbd;
	pISR_DMA2 =(unsigned)IsrDma2;
#else
	pISR_IRQ =(unsigned)IsrUsbd;	
		//Why doesn't it receive the big file if use this. (???)
		//It always stops when 327680 bytes are received.
#endif	
	ClearPending(BIT_DMA2);
	ClearPending(BIT_USBD);
	//rINTMSK&=~(BIT_USBD);  
   
	//pISR_FIQ,pISR_IRQ must be initialized
}


void HaltUndef(void)
{
	Uart_Printf("Undefined instruction exception!!!\n");
	while(1);
}

void HaltSwi(void)
{
	Uart_Printf("SWI exception!!!\n");
	while(1);
}

void HaltPabort(void)
{
	Uart_Printf("Pabort exception!!!\n");
	while(1);
}

void HaltDabort(void)
{
	Uart_Printf("Dabort exception!!!\n");
	while(1);
}


void ClearMemory(void)
{
//	int i;
//	U32 data;
	int memError=0;
	U32 *pt;
	
	//
	// memory clear
	//
	Uart_Printf("Clear Memory (%xh-%xh):WR",_MMUTT_STARTADDRESS,_ISR_STARTADDRESS);

	pt=(U32 *)_MMUTT_STARTADDRESS;
	while((U32)pt < _ISR_STARTADDRESS)
	{
		*pt=(U32)0x0;
		pt++;
	}
	
	if(memError==0)Uart_Printf("\b\bO.K.\n");
}

void Clk0_Enable(int clock_sel)	
{	// 0:MPLLin, 1:UPLL, 2:FCLK, 3:HCLK, 4:PCLK, 5:DCLK0
	rMISCCR = rMISCCR&~(7<<4) | (clock_sel<<4);
	rGPHCON = rGPHCON&~(3<<18) | (2<<18);
}
void Clk1_Enable(int clock_sel)
{	// 0:MPLLout, 1:UPLL, 2:RTC, 3:HCLK, 4:PCLK, 5:DCLK1	
	rMISCCR = rMISCCR&~(7<<8) | (clock_sel<<8);
	rGPHCON = rGPHCON&~(3<<20) | (2<<20);
}
void Clk0_Disable(void)
{
	rGPHCON = rGPHCON&~(3<<18);	// GPH9 Input
}
void Clk1_Disable(void)
{
	rGPHCON = rGPHCON&~(3<<20);	// GPH10 Input
}

