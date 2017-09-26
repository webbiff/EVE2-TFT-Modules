/*

Copyright (c) Future Technology Devices International 2014

THIS SOFTWARE IS PROVIDED BY FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

FTDI DRIVERS MAY BE USED ONLY IN CONJUNCTION WITH PRODUCTS BASED ON FTDI PARTS.

FTDI DRIVERS MAY BE DISTRIBUTED IN ANY FORM AS LONG AS LICENSE INFORMATION IS NOT MODIFIED.

IF A CUSTOM VENDOR ID AND/OR PRODUCT ID OR DESCRIPTION STRING ARE USED, IT IS THE
RESPONSIBILITY OF THE PRODUCT MANUFACTURER TO MAINTAIN ANY CHANGES AND SUBSEQUENT WHQL
RE-CERTIFICATION AS A RESULT OF MAKING THESE CHANGES.

Abstract:

Application to demonstrate gradient function of EVE.

Author : FTDI

Revision History:

*/

#include <stdio.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include "FT_Platform.h"

#define F16(s)        ((ft_int32_t)((s) * 65536))
#define SQ(v) (v*v)

/* Global variables for display resolution to support various display panels */
/* Default is WQVGA - 480x272 */
ft_int16_t FT_DispWidth = 480;
ft_int16_t FT_DispHeight = 272;
ft_int16_t FT_DispHCycle =  548;
ft_int16_t FT_DispHOffset = 43;
ft_int16_t FT_DispHSync0 = 0;
ft_int16_t FT_DispHSync1 = 41;
ft_int16_t FT_DispVCycle = 292;
ft_int16_t FT_DispVOffset = 12;
ft_int16_t FT_DispVSync0 = 0;
ft_int16_t FT_DispVSync1 = 10;
ft_uint8_t FT_DispPCLK = 5;
ft_char8_t FT_DispSwizzle = 0;
ft_char8_t FT_DispPCLKPol = 1;
ft_char8_t FT_DispCSpread = 1;
ft_char8_t FT_DispDither = 1;

/* Global used for buffer optimization */
Ft_Gpu_Hal_Context_t host,*phost;

ft_uint32_t Ft_CmdBuffer_Index;
ft_uint32_t Ft_DlBuffer_Index;

#ifdef BUFFER_OPTIMIZATION
ft_uint8_t  Ft_DlBuffer[FT_DL_SIZE];
ft_uint8_t  Ft_CmdBuffer[FT_CMD_FIFO_SIZE];
#endif
/* Boot up for FT800 followed by graphics primitive sample cases */
/* Initial boot up DL - make the back ground green color */
const ft_uint8_t FT_DLCODE_BOOTUP[12] = 
{
    0,0,0,2,//GPU instruction CLEAR_COLOR_RGB
    7,0,0,38, //GPU instruction CLEAR

    0,0,0,0,  //GPU instruction DISPLAY
};
ft_void_t Ft_App_WrCoCmd_Buffer(Ft_Gpu_Hal_Context_t *phost,ft_uint32_t cmd)
{
#ifdef  BUFFER_OPTIMIZATION
   /* Copy the command instruction into buffer */
   ft_uint32_t *pBuffcmd;
   pBuffcmd =(ft_uint32_t*)&Ft_CmdBuffer[Ft_CmdBuffer_Index];
   *pBuffcmd = cmd;
#endif
#ifdef ARDUINO_PLATFORM
   Ft_Gpu_Hal_WrCmd32(phost,cmd);
#endif
#ifdef FT900_PLATFORM
   Ft_Gpu_Hal_WrCmd32(phost,cmd);
#endif
   /* Increment the command index */
   Ft_CmdBuffer_Index += FT_CMD_SIZE;  
}

ft_void_t Ft_App_WrDlCmd_Buffer(Ft_Gpu_Hal_Context_t *phost,ft_uint32_t cmd)
{
#ifdef BUFFER_OPTIMIZATION  
   /* Copy the command instruction into buffer */
   ft_uint32_t *pBuffcmd;
   pBuffcmd =(ft_uint32_t*)&Ft_DlBuffer[Ft_DlBuffer_Index];
   *pBuffcmd = cmd;
#endif

#ifdef ARDUINO_PLATFORM
   Ft_Gpu_Hal_Wr32(phost,(RAM_DL+Ft_DlBuffer_Index),cmd);
#endif
#ifdef FT900_PLATFORM
   Ft_Gpu_Hal_Wr32(phost,(RAM_DL+Ft_DlBuffer_Index),cmd);
#endif
   /* Increment the command index */
   Ft_DlBuffer_Index += FT_CMD_SIZE;  
}

ft_void_t Ft_App_WrCoStr_Buffer(Ft_Gpu_Hal_Context_t *phost,const ft_char8_t *s)
{
#ifdef  BUFFER_OPTIMIZATION  
  ft_uint16_t length = 0;
  length = strlen(s) + 1;//last for the null termination
  
  strcpy(&Ft_CmdBuffer[Ft_CmdBuffer_Index],s);  

  /* increment the length and align it by 4 bytes */
  Ft_CmdBuffer_Index += ((length + 3) & ~3);  
#endif  
}

ft_void_t Ft_App_Flush_DL_Buffer(Ft_Gpu_Hal_Context_t *phost)
{
#ifdef  BUFFER_OPTIMIZATION    
   if (Ft_DlBuffer_Index> 0)
     Ft_Gpu_Hal_WrMem(phost,RAM_DL,Ft_DlBuffer,Ft_DlBuffer_Index);
#endif     
   Ft_DlBuffer_Index = 0;
   
}

ft_void_t Ft_App_Flush_Co_Buffer(Ft_Gpu_Hal_Context_t *phost)
{
#ifdef  BUFFER_OPTIMIZATION    
   if (Ft_CmdBuffer_Index > 0)
     Ft_Gpu_Hal_WrCmdBuf(phost,Ft_CmdBuffer,Ft_CmdBuffer_Index);
#endif     
   Ft_CmdBuffer_Index = 0;
}


/* API to give fadeout effect by changing the display PWM from 100 till 0 */
ft_void_t SAMAPP_fadeout()
{
   ft_int32_t i;
	
	for (i = 100; i >= 0; i -= 3) 
	{
		Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);

		Ft_Gpu_Hal_Sleep(2);//sleep for 2 ms
	}
}

/* API to perform display fadein effect by changing the display PWM from 0 till 100 and finally 128 */
ft_void_t SAMAPP_fadein()
{
	ft_int32_t i;
	
	for (i = 0; i <=100 ; i += 3) 
	{
		Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);
		Ft_Gpu_Hal_Sleep(2);//sleep for 2 ms
	}
	/* Finally make the PWM 100% */
	i = 128;
	Ft_Gpu_Hal_Wr8(phost,REG_PWM_DUTY,i);
}


/* API to check the status of previous DLSWAP and perform DLSWAP of new DL */
/* Check for the status of previous DLSWAP and if still not done wait for few ms and check again */
ft_void_t SAMAPP_GPU_DLSwap(ft_uint8_t DL_Swap_Type)
{
	ft_uint8_t Swap_Type = DLSWAP_FRAME,Swap_Done = DLSWAP_FRAME;

	if(DL_Swap_Type == DLSWAP_LINE)
	{
		Swap_Type = DLSWAP_LINE;
	}

	/* Perform a new DL swap */
	Ft_Gpu_Hal_Wr8(phost,REG_DLSWAP,Swap_Type);

	/* Wait till the swap is done */
	while(Swap_Done)
	{
		Swap_Done = Ft_Gpu_Hal_Rd8(phost,REG_DLSWAP);

		if(DLSWAP_DONE != Swap_Done)
		{
			Ft_Gpu_Hal_Sleep(10);//wait for 10ms
		}
	}	
}

ft_void_t Ft_BootupConfig()
{
	Ft_Gpu_Hal_Powercycle(phost,FT_TRUE);

		/* Access address 0 to wake up the FT800 */
		Ft_Gpu_HostCommand(phost,FT_GPU_ACTIVE_M);
		Ft_Gpu_Hal_Sleep(20);

		/* Set the clk to external clock */
#if (!defined(ME800A_HV35R) && !defined(ME810A_HV35R))
		Ft_Gpu_HostCommand(phost,FT_GPU_EXTERNAL_OSC);
		Ft_Gpu_Hal_Sleep(10);
#endif

		{
			ft_uint8_t chipid;
			//Read Register ID to check if FT800 is ready.
			chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
			while(chipid != 0x7C)
			{
				chipid = Ft_Gpu_Hal_Rd8(phost, REG_ID);
				ft_delay(100);
			}
	#if defined(MSVC_PLATFORM) || defined (FT900_PLATFORM)
			printf("VC1 register ID after wake up %x\n",chipid);
	#endif
	}
	/* Configuration of LCD display */
#ifdef DISPLAY_RESOLUTION_QVGA
	/* Values specific to QVGA LCD display */
	FT_DispWidth = 320;
	FT_DispHeight = 240;
	FT_DispHCycle =  408;
	FT_DispHOffset = 70;
	FT_DispHSync0 = 0;
	FT_DispHSync1 = 10;
	FT_DispVCycle = 263;
	FT_DispVOffset = 13;
	FT_DispVSync0 = 0;
	FT_DispVSync1 = 2;
	FT_DispPCLK = 8;
	FT_DispSwizzle = 2;
	FT_DispPCLKPol = 0;
	FT_DispCSpread = 1;
	FT_DispDither = 1;

#endif
#ifdef DISPLAY_RESOLUTION_WVGA
	/* Values specific to QVGA LCD display */
	FT_DispWidth = 800;
	FT_DispHeight = 480;
	FT_DispHCycle =  928;
	FT_DispHOffset = 88;
	FT_DispHSync0 = 0;
	FT_DispHSync1 = 48;
	FT_DispVCycle = 525;
	FT_DispVOffset = 32;
	FT_DispVSync0 = 0;
	FT_DispVSync1 = 3;
	FT_DispPCLK = 2;
	FT_DispSwizzle = 0;
	FT_DispPCLKPol = 0;
	FT_DispCSpread = 0;
	FT_DispDither = 1;
#endif
#ifdef DISPLAY_RESOLUTION_HVGA_PORTRAIT
	/* Values specific to HVGA LCD display */

	FT_DispWidth = 320;
	FT_DispHeight = 480;
	FT_DispHCycle =  400;
	FT_DispHOffset = 40;
	FT_DispHSync0 = 0;
	FT_DispHSync1 = 10;
	FT_DispVCycle = 500;
	FT_DispVOffset = 10;
	FT_DispVSync0 = 0;
	FT_DispVSync1 = 5;
	FT_DispPCLK = 4;
	FT_DispSwizzle = 2;
	FT_DispPCLKPol = 1;
	FT_DispCSpread = 1;
	FT_DispDither = 1;

#ifdef ME810A_HV35R
	FT_DispPCLK = 5;
#endif

#endif

#if (defined(ME800A_HV35R) || defined(ME810A_HV35R))
	/* After recognizing the type of chip, perform the trimming if necessary */
    Ft_Gpu_ClockTrimming(phost,LOW_FREQ_BOUND);
#endif

	Ft_Gpu_Hal_Wr16(phost, REG_HCYCLE, FT_DispHCycle);
	Ft_Gpu_Hal_Wr16(phost, REG_HOFFSET, FT_DispHOffset);
	Ft_Gpu_Hal_Wr16(phost, REG_HSYNC0, FT_DispHSync0);
	Ft_Gpu_Hal_Wr16(phost, REG_HSYNC1, FT_DispHSync1);
	Ft_Gpu_Hal_Wr16(phost, REG_VCYCLE, FT_DispVCycle);
	Ft_Gpu_Hal_Wr16(phost, REG_VOFFSET, FT_DispVOffset);
	Ft_Gpu_Hal_Wr16(phost, REG_VSYNC0, FT_DispVSync0);
	Ft_Gpu_Hal_Wr16(phost, REG_VSYNC1, FT_DispVSync1);
	Ft_Gpu_Hal_Wr8(phost, REG_SWIZZLE, FT_DispSwizzle);
	Ft_Gpu_Hal_Wr8(phost, REG_PCLK_POL, FT_DispPCLKPol);
	Ft_Gpu_Hal_Wr16(phost, REG_HSIZE, FT_DispWidth);
	Ft_Gpu_Hal_Wr16(phost, REG_VSIZE, FT_DispHeight);
	Ft_Gpu_Hal_Wr16(phost, REG_CSPREAD, FT_DispCSpread);
	Ft_Gpu_Hal_Wr16(phost, REG_DITHER, FT_DispDither);

#if (defined(FT_800_ENABLE) || defined(FT_810_ENABLE) ||defined(FT_812_ENABLE))
    /* Touch configuration - configure the resistance value to 1200 - this value is specific to customer requirement and derived by experiment */
    Ft_Gpu_Hal_Wr16(phost, REG_TOUCH_RZTHRESH,RESISTANCE_THRESHOLD);
#endif
    Ft_Gpu_Hal_Wr8(phost, REG_GPIO_DIR,0xff);
    Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0xff);


    /*It is optional to clear the screen here*/
    Ft_Gpu_Hal_WrMem(phost, RAM_DL,(ft_uint8_t *)FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
    Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);


    Ft_Gpu_Hal_Wr8(phost, REG_PCLK,FT_DispPCLK);//after this display is visible on the LCD


#ifdef ENABLE_ILI9488_HVGA_PORTRAIT
	/* to cross check reset pin */
	Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0xff);
	ft_delay(120);
	Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0x7f);
	ft_delay(120);
	Ft_Gpu_Hal_Wr8(phost, REG_GPIO,0xff);

//	ILI9488_Bootup();
	
	/* Reconfigure the SPI to communicate with FT8xx */
#ifdef FT900_PLATFORM
	printf("after ILI9488 bootup \n");
	//spi
	// Initialize SPIM HW
	sys_enable(sys_device_spi_master);
	gpio_function(27, pad_spim_sck); /* GPIO27 to SPIM_CLK */
	gpio_function(28, pad_spim_ss0); /* GPIO28 as CS */
	gpio_function(29, pad_spim_mosi); /* GPIO29 to SPIM_MOSI */
	gpio_function(30, pad_spim_miso); /* GPIO30 to SPIM_MISO */

	gpio_write(28, 1);
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);
#endif
#endif

	/* make the spi to quad mode - addition 2 bytes for silicon */
#ifdef FT_81X_ENABLE
	/* api to set quad and numbe of dummy bytes */
#ifdef ENABLE_SPI_QUAD
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_QUAD_CHANNEL,FT_GPU_SPI_TWODUMMY);
#elif ENABLE_SPI_DUAL
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_DUAL_CHANNEL,FT_GPU_SPI_TWODUMMY);
#else
	Ft_Gpu_Hal_SetSPI(phost,FT_GPU_SPI_SINGLE_CHANNEL,FT_GPU_SPI_ONEDUMMY);
#endif

#endif

#ifdef FT900_PLATFORM
    /* Change clock frequency to 25mhz */
	spi_init(SPIM, spi_dir_master, spi_mode_0, 4);

#if (defined(ENABLE_SPI_QUAD))
    /* Initialize IO2 and IO3 pad/pin for dual and quad settings */
    gpio_function(31, pad_spim_io2);
    gpio_function(32, pad_spim_io3);
    gpio_write(31, 1);
    gpio_write(32, 1);
#endif
	/* Enable FIFO of QSPI */
	spi_option(SPIM,spi_option_fifo_size,64);
	spi_option(SPIM,spi_option_fifo,1);
	spi_option(SPIM,spi_option_fifo_receive_trigger,1);
#endif

#ifdef ENABLE_SPI_QUAD
#ifdef FT900_PLATFORM
	spi_option(SPIM,spi_option_bus_width,4);
#endif
#elif ENABLE_SPI_DUAL	
#ifdef FT900_PLATFORM
	spi_option(SPIM,spi_option_bus_width,2);
#endif
#else
#ifdef FT900_PLATFORM
	spi_option(SPIM,spi_option_bus_width,1);
#endif

	
#endif


phost->ft_cmd_fifo_wp = Ft_Gpu_Hal_Rd16(phost,REG_CMD_WRITE);

}






/*****************************************************************************/
/* Example code to display few points at various offsets with various colors */



/* Boot up for FT800 followed by graphics primitive sample cases */
/* Initial boot up DL - make the back ground green color */


static ft_int32_t ox;
static ft_uint8_t noofch=0;

static ft_uint8_t home_star_icon[] = {0x78,0x9C,0xE5,0x94,0xBF,0x4E,0xC2,0x40,0x1C,0xC7,0x7F,0x2D,0x04,0x8B,0x20,0x45,0x76,0x14,0x67,0xA3,0xF1,0x0D,0x64,0x75,0xD2,0xD5,0x09,0x27,0x17,0x13,0xE1,0x0D,0xE4,0x0D,0x78,0x04,0x98,0x5D,0x30,0x26,0x0E,0x4A,0xA2,0x3E,0x82,0x0E,0x8E,0x82,0xC1,0x38,0x62,0x51,0x0C,0x0A,0x42,0x7F,0xDE,0xB5,0x77,0xB4,0x77,0x17,0x28,0x21,0x26,0x46,0xFD,0x26,0xCD,0xE5,0xD3,0x7C,0xFB,0xBB,0xFB,0xFD,0xB9,0x02,0xCC,0xA4,0xE8,0x99,0x80,0x61,0xC4,0x8A,0x9F,0xCB,0x6F,0x31,0x3B,0xE3,0x61,0x7A,0x98,0x84,0x7C,0x37,0xF6,0xFC,0xC8,0xDD,0x45,0x00,0xDD,0xBA,0xC4,0x77,0xE6,0xEE,0x40,0xEC,0x0E,0xE6,0x91,0xF1,0xD2,0x00,0x42,0x34,0x5E,0xCE,0xE5,0x08,0x16,0xA0,0x84,0x68,0x67,0xB4,0x86,0xC3,0xD5,0x26,0x2C,0x20,0x51,0x17,0xA2,0xB8,0x03,0xB0,0xFE,0x49,0xDD,0x54,0x15,0xD8,0xEE,0x73,0x37,0x95,0x9D,0xD4,0x1A,0xB7,0xA5,0x26,0xC4,0x91,0xA9,0x0B,0x06,0xEE,0x72,0xB7,0xFB,0xC5,0x16,0x80,0xE9,0xF1,0x07,0x8D,0x3F,0x15,0x5F,0x1C,0x0B,0xFC,0x0A,0x90,0xF0,0xF3,0x09,0xA9,0x90,0xC4,0xC6,0x37,0xB0,0x93,0xBF,0xE1,0x71,0xDB,0xA9,0xD7,0x41,0xAD,0x46,0xEA,0x19,0xA9,0xD5,0xCE,0x93,0xB3,0x35,0x73,0x0A,0x69,0x59,0x91,0xC3,0x0F,0x22,0x1B,0x1D,0x91,0x13,0x3D,0x91,0x73,0x43,0xF1,0x6C,0x55,0xDA,0x3A,0x4F,0xBA,0x25,0xCE,0x4F,0x04,0xF1,0xC5,0xCF,0x71,0xDA,0x3C,0xD7,0xB9,0xB2,0x48,0xB4,0x89,0x38,0x20,0x4B,0x2A,0x95,0x0C,0xD5,0xEF,0x5B,0xAD,0x96,0x45,0x8A,0x41,0x96,0x7A,0x1F,0x60,0x0D,0x7D,0x22,0x75,0x82,0x2B,0x0F,0xFB,0xCE,0x51,0x3D,0x2E,0x3A,0x21,0xF3,0x1C,0xD9,0x38,0x86,0x2C,0xC6,0x05,0xB6,0x7B,0x9A,0x8F,0x0F,0x97,0x1B,0x72,0x6F,0x1C,0xEB,0xAE,0xFF,0xDA,0x97,0x0D,0xBA,0x43,0x32,0xCA,0x66,0x34,0x3D,0x54,0xCB,0x24,0x9B,0x43,0xF2,0x70,0x3E,0x42,0xBB,0xA0,0x95,0x11,0x37,0x46,0xE1,0x4F,0x49,0xC5,0x1B,0xFC,0x3C,0x3A,0x3E,0xD1,0x65,0x0E,0x6F,0x58,0xF8,0x9E,0x5B,0xDB,0x55,0xB6,0x41,0x34,0xCB,0xBE,0xDB,0x87,0x5F,0xA9,0xD1,0x85,0x6B,0xB3,0x17,0x9C,0x61,0x0C,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xA2,0x5D,0x61,0x10,0xED,0x2A,0x9B,0xED,0xC9,0xFC,0xDF,0x14,0x54,0x8F,0x80,0x7A,0x06,0xF5,0x23,0xA0,0x9F,0x41,0xF3,0x10,0x30,0x4F,0x41,0xF3,0x18,0x30,0xCF,0xCA,0xFC,0xFF,0x35,0xC9,0x79,0xC9,0x89,0xFA,0x33,0xD7,0x1D,0xF6,0x5E,0x84,0x5C,0x56,0x6E,0xA7,0xDA,0x1E,0xF9,0xFA,0xAB,0xF5,0x97,0xFF,0x2F,0xED,0x89,0x7E,0x29,0x9E,0xB4,0x9F,0x74,0x1E,0x69,0xDA,0xA4,0x9F,0x81,0x94,0xEF,0x4F,0xF6,0xF9,0x0B,0xF4,0x65,0x51,0x08};



static void cs(ft_uint8_t i)
{
  switch (i)
  {
    case  0:  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(200,255,200)); break;
    case 60: Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,0)); break;
    case 80: Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,0,0)); break;
  }
}

ft_void_t home_setup()
{
  Ft_Gpu_Hal_WrCmd32(phost,CMD_INFLATE);
  Ft_Gpu_Hal_WrCmd32(phost,250*1024L);
  Ft_Gpu_Hal_WrCmdBuf(phost,home_star_icon,sizeof(home_star_icon));
  
  Ft_Gpu_CoCmd_Dlstart(phost);        // start
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255, 255, 255));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(13));    // handle for background stars
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(250*1024L));      // Starting address in gram
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 16, 32));  // format 
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, REPEAT, REPEAT, 512, 512  ));
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_HANDLE(14));    // handle for background stars
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SOURCE(250*1024L));      // Starting address in gram
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_LAYOUT(L4, 16, 32));  // format 
  Ft_App_WrCoCmd_Buffer(phost,BITMAP_SIZE(NEAREST, BORDER, BORDER, 32, 32  ));
  Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
  Ft_Gpu_CoCmd_Swap(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
}


static ft_uint8_t sk=0;
ft_uint8_t Read_Keys()
{
  static ft_uint8_t Read_tag=0,temp_tag=0,ret_tag=0;	
  Read_tag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
  ret_tag = NULL;
  if(Read_tag!=NULL)								// Allow if the Key is released
  {
    if(temp_tag!=Read_tag)
    {
      temp_tag = Read_tag;	
      sk = Read_tag;										// Load the Read tag to temp variable	
    }  
  }
  else
  {
    if(temp_tag!=0)
    {
      ret_tag = temp_tag;
    }  
    sk = 0;
  }
  return ret_tag;
}



char* const info[] PROGMEM = {  "FT800 Gradient Application",
                          "APP to demonstrate interactive Gradient,",
                          "using String, Gradient",
                          "& Slider."
                       }; 

void Ft_Play_Sound(ft_uint8_t sound,ft_uint8_t vol,ft_uint8_t midi)
{
  ft_uint16_t val = (midi << 8) | sound; 
  Ft_Gpu_Hal_Wr16(phost,REG_SOUND,val);
  Ft_Gpu_Hal_Wr8(phost,REG_PLAY,1); 
}

ft_void_t Info()
{
  ft_uint16_t dloffset = 0,z;
  Ft_CmdBuffer_Index = 0;
  

  
  Ft_Gpu_CoCmd_Dlstart(phost); 
  Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
  Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight/2,26,OPT_CENTERX|OPT_CENTERY,"Please tap on a dot");
  Ft_Gpu_CoCmd_Calibrate(phost,0);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  
  Ft_Gpu_CoCmd_Logo(phost);
  Ft_App_Flush_Co_Buffer(phost);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  while(0!=Ft_Gpu_Hal_Rd16(phost,REG_CMD_READ)); 
  dloffset = Ft_Gpu_Hal_Rd16(phost,REG_CMD_DL);
  dloffset -=4;
#ifdef FT_81X_ENABLE
  dloffset -= 2*4;//remove two more instructions in case of 81x
#endif
  
  Ft_Gpu_Hal_WrCmd32(phost,CMD_MEMCPY);
  Ft_Gpu_Hal_WrCmd32(phost,100000L);
  Ft_Gpu_Hal_WrCmd32(phost,RAM_DL);
  Ft_Gpu_Hal_WrCmd32(phost,dloffset);
  //Enter into Info Screen
  do
  {
    Ft_Gpu_CoCmd_Dlstart(phost);   
    Ft_Gpu_CoCmd_Append(phost,100000L,dloffset);
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_A(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_B(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_C(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_D(0));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_E(256));
    Ft_App_WrCoCmd_Buffer(phost,BITMAP_TRANSFORM_F(0));  
    Ft_App_WrCoCmd_Buffer(phost,SAVE_CONTEXT());	
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(219,180,150));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(220));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(EDGE_STRIP_A));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(0,FT_DispHeight*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(FT_DispWidth*16,FT_DispHeight*16));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_A(255));
    Ft_App_WrCoCmd_Buffer(phost,RESTORE_CONTEXT());	
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
   // INFORMATION 
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,20,28,OPT_CENTERX|OPT_CENTERY,(char*)pgm_read_word(&info[0]));
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,60,26,OPT_CENTERX|OPT_CENTERY,(char*)pgm_read_word(&info[1]));
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,90,26,OPT_CENTERX|OPT_CENTERY,(char*)pgm_read_word(&info[2]));  
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,120,26,OPT_CENTERX|OPT_CENTERY,(char*)pgm_read_word(&info[3]));  
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth/2,FT_DispHeight-30,26,OPT_CENTERX|OPT_CENTERY,"Click to play");
    //Check the Play key and change the color if play key is detect
    if(sk!='P')
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
    else
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(100,100,100));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));   
    Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(20*16));
    Ft_App_WrCoCmd_Buffer(phost,TAG('P'));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((FT_DispWidth/2)*16,(FT_DispHeight-60)*16));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(180,35,35));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II((FT_DispWidth/2)-14,(FT_DispHeight-75),14,4));
    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);
  }while(Read_Keys()!='P');
   Ft_Play_Sound(0x50,255,0xc0);
}

#ifdef FT900_PLATFORM
	ft_void_t FT900_Config()
{
		sys_enable(sys_device_uart0);
	    gpio_function(48, pad_uart0_txd); /* UART0 TXD */
	    gpio_function(49, pad_uart0_rxd); /* UART0 RXD */
	    uart_open(UART0,                    /* Device */
	              1,                        /* Prescaler = 1 */
	              UART_DIVIDER_115200_BAUD,  /* Divider = 1302 */
	              uart_data_bits_8,         /* No. Data Bits */
	              uart_parity_none,         /* Parity */
	              uart_stop_bits_1);        /* No. Stop Bits */

	    /* Print out a welcome message... */
	    uart_puts(UART0,

	        "(C) Copyright 2014-2015, Future Technology Devices International Ltd. \r\n"
	        "--------------------------------------------------------------------- \r\n"
	        "Welcome to Gradient Example ... \r\n"
	        "\r\n"
	        "--------------------------------------------------------------------- \r\n"
	        );

#ifdef ENABLE_ILI9488_HVGA_PORTRAIT
	/* asign all the respective pins to gpio and set them to default values */
	gpio_function(34, pad_gpio34);
	gpio_dir(34, pad_dir_output);
    gpio_write(34,1);

	gpio_function(27, pad_gpio27);
	gpio_dir(27, pad_dir_output);
    gpio_write(27,1);

	gpio_function(29, pad_gpio29);
	gpio_dir(29, pad_dir_output);
    gpio_write(29,1);

    gpio_function(33, pad_gpio33);
    gpio_dir(33, pad_dir_output);
    gpio_write(33,1);


    gpio_function(30, pad_gpio30);
    gpio_dir(30, pad_dir_output);
    gpio_write(30,1);

	gpio_function(28, pad_gpio28);
	gpio_dir(28, pad_dir_output);
    gpio_write(28,1);


  	gpio_function(43, pad_gpio43);
  	gpio_dir(43, pad_dir_output);
    gpio_write(43,1);
	gpio_write(34,1);
	gpio_write(28,1);
	gpio_write(43,1);
	gpio_write(33,1);
	gpio_write(33,1);

#endif
	/* useful for timer */
	//ft_millis_init();
	//interrupt_enable_globally();
	//printf("ft900 config done \n");
}
#endif


ft_void_t Gradient()
{
  ft_uint32_t Read_xy = 0,
              tracker;
  ft_uint16_t x1,x2,y1,y2,
              Read_x = 0,Read_y = 0,
  	      val1=48568,  val2=32765,
	      tx = 48,ty = 20,tval;
					
  ft_uint8_t		
             Read_Tag  = 0,
	     drag,buff[512];
  ft_int32_t TPsize;

  x1 = 50;y1 = 50;

  TPsize = FT_DispWidth*5/100;

  x2 = FT_DispWidth-60;
  y2 = FT_DispHeight-40;
  drag = 0;
  // compute the background gradient effect
  for(tval=0;tval<(FT_DispHeight/2);tval++)	
  {
    buff[FT_DispHeight-1-tval] = buff[tval] = (tval*0.9);		
  }
  Ft_Gpu_Hal_WrMem(phost,4096L,buff,FT_DispHeight);
  
  // Set the bitmap properties for the background
  Ft_DlBuffer_Index = 0;
  Ft_App_WrDlCmd_Buffer(phost,CLEAR(1,1,1));
  Ft_App_WrDlCmd_Buffer(phost,COLOR_A(255));
  Ft_App_WrDlCmd_Buffer(phost,COLOR_RGB(255,255,255));	
  Ft_App_WrDlCmd_Buffer(phost,BITMAP_HANDLE(2));	
  Ft_App_WrDlCmd_Buffer(phost,BITMAP_SOURCE(4096L));	
  Ft_App_WrDlCmd_Buffer(phost,BITMAP_LAYOUT(L8,1,FT_DispHeight));	
  Ft_App_WrDlCmd_Buffer(phost,BITMAP_SIZE(NEAREST, REPEAT, BORDER, FT_DispWidth, FT_DispHeight));
  Ft_Gpu_CoCmd_Track(phost,(FT_DispWidth-38),40,8,FT_DispHeight-65,3);	
  Ft_Gpu_CoCmd_Track(phost,(FT_DispWidth-15),40,8,FT_DispHeight-65,4);	
  Ft_App_WrDlCmd_Buffer(phost,DISPLAY());
  Ft_App_Flush_DL_Buffer(phost);
  Ft_Gpu_Hal_Wr8(phost,REG_DLSWAP,DLSWAP_FRAME);
  Ft_Gpu_Hal_WaitCmdfifo_empty(phost);  
  do
  {
    /* Read the Touch\drag*/
    Read_xy = Ft_Gpu_Hal_Rd32(phost,REG_TOUCH_SCREEN_XY); 
    Read_Tag = Ft_Gpu_Hal_Rd8(phost,REG_TOUCH_TAG);
    // pick the x& y cordinates of the points  
    Read_y = Read_xy & 0xffff;
    Read_x = (Read_xy >> 16) & 0xffff;
    if(Read_xy == 0x80008000)  drag = 0;
    else if(Read_Tag==1 || Read_Tag==2)drag = Read_Tag; 
    /* compute the coordintes of two points x&y*/
    if(drag == 1)
    {
	if(Read_x>=(FT_DispWidth-60))0; else	x1 = Read_x; 
	if(Read_y>=(FT_DispHeight-20))0; else	y1 = Read_y;
    }
    if(drag == 2)
    {
	if(Read_x>=(FT_DispWidth-60))0; else	x2 = Read_x; 
	if(Read_y>=(FT_DispHeight-20))0; else	y2 = Read_y;
    }
    /* read the tracker*/
    tracker = Ft_Gpu_Hal_Rd32(phost,REG_TRACKER);	
    /* compute the color for the gradient */
    if((tracker&0xff) > 2)
    {
      if((tracker&0xff)==3)
      {
	val1 = (tracker>>16);		
      }
      else if((tracker&0xff)==4)
      {
	val2 = (tracker>>16);		
      }
    }
// start the new displaylist
    Ft_Gpu_CoCmd_Dlstart(phost); 
    Ft_App_WrCoCmd_Buffer(phost,CLEAR_COLOR_RGB(55,55,55));
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1) );
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(BITMAPS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2II(0,0,2,0));
    Ft_Gpu_CoCmd_FgColor(phost,0xffffff);
  
    Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));	
  
    Ft_Gpu_CoCmd_BgColor(phost,val2*255);	
    Ft_App_WrCoCmd_Buffer(phost,TAG(4));
    Ft_Gpu_CoCmd_Slider(phost,(FT_DispWidth-15),40,8,(FT_DispHeight-65),0,val2,65535);	
  
    Ft_Gpu_CoCmd_BgColor(phost,val1*255);
    Ft_App_WrCoCmd_Buffer(phost,TAG(3));
    Ft_Gpu_CoCmd_Slider(phost,(FT_DispWidth-38),40,8,(FT_DispHeight-65),0,val1,65535);	
    Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
  
  
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));

    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth-45,10,26,0,"C1");
    Ft_Gpu_CoCmd_Text(phost,FT_DispWidth-20,10,26,0,"C2");
    Ft_App_WrCoCmd_Buffer(phost,SCISSOR_XY(0,10));
    Ft_App_WrCoCmd_Buffer(phost,SCISSOR_SIZE(FT_DispWidth-50,FT_DispHeight-30));
    Ft_App_WrCoCmd_Buffer(phost,CLEAR(1,1,1));
    Ft_Gpu_CoCmd_Gradient(phost,x1,y1,val1*255,x2,y2,val2*255);
  		
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(0,0,0));
  // Place a dot for drag the color
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(0,0,0,0));
    Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(TPsize*16));
    Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(1));
    Ft_App_WrCoCmd_Buffer(phost,TAG(1));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x1*16,y1*16));
    Ft_App_WrCoCmd_Buffer(phost,TAG(2));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x2*16,y2*16));
    Ft_App_WrCoCmd_Buffer(phost,TAG_MASK(0));
  
    /* calculation of touch point size */

    Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));
    Ft_App_WrCoCmd_Buffer(phost,POINT_SIZE(TPsize*16));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(FTPOINTS));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x1*16,y1*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F(x2*16,y2*16));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_MASK(1,1,1,1));
    Ft_App_WrCoCmd_Buffer(phost,COLOR_RGB(255,255,255));
    Ft_App_WrCoCmd_Buffer(phost,BEGIN(LINES));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((x1-TPsize)*16,(y1)*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((x1+TPsize)*16,(y1)*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((x1)*16,(y1-TPsize)*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((x1)*16,(y1+TPsize)*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((x2-TPsize)*16,(y2)*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((x2+TPsize)*16,(y2)*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((x2)*16,(y2-TPsize)*16));
    Ft_App_WrCoCmd_Buffer(phost,VERTEX2F((x2)*16,(y2+TPsize)*16));
    Ft_App_WrCoCmd_Buffer(phost,END());
    Ft_App_WrCoCmd_Buffer(phost,DISPLAY());
    Ft_Gpu_CoCmd_Swap(phost);
    Ft_App_Flush_Co_Buffer(phost);
    Ft_Gpu_Hal_WaitCmdfifo_empty(phost);	
  }while(1);
}



#if defined MSVC_PLATFORM | defined FT900_PLATFORM
/* Main entry point */
ft_int32_t main(ft_int32_t argc,ft_char8_t *argv[])
#endif
#if defined(ARDUINO_PLATFORM)||defined(MSVC_FT800EMU)
ft_void_t setup()
#endif
{

	 ft_uint8_t chipid;
#ifdef FT900_PLATFORM
	FT900_Config();
#endif
	Ft_Gpu_HalInit_t halinit;
	
	halinit.TotalChannelNum = 1;

              
	Ft_Gpu_Hal_Init(&halinit);
	host.hal_config.channel_no = 0;
	host.hal_config.pdn_pin_no = FT800_PD_N;
	host.hal_config.spi_cs_pin_no = FT800_SEL_PIN;
#ifdef MSVC_PLATFORM_SPI
	host.hal_config.spi_clockrate_khz = 12000; //in KHz
#endif
#ifdef ARDUINO_PLATFORM_SPI
	host.hal_config.spi_clockrate_khz = 4000; //in KHz
#endif
    Ft_Gpu_Hal_Open(&host);

    //printf("Ft_Gpu_Hal_Open done \n");
	phost = &host;

    Ft_BootupConfig();

#if ((defined FT900_PLATFORM) || defined(MSVC_PLATFORM))
	printf("\n reg_touch_rz =0x%x ", Ft_Gpu_Hal_Rd16(phost, REG_TOUCH_RZ));
	printf("\n reg_touch_rzthresh =0x%x ", Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_RZTHRESH));
    printf("\n reg_touch_tag_xy=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG_XY));
	printf("\n reg_touch_tag=0x%x",Ft_Gpu_Hal_Rd32(phost, REG_TOUCH_TAG));
#endif

    /*It is optional to clear the screen here*/	
    Ft_Gpu_Hal_WrMem(phost, RAM_DL,(ft_uint8_t *)FT_DLCODE_BOOTUP,sizeof(FT_DLCODE_BOOTUP));
    Ft_Gpu_Hal_Wr8(phost, REG_DLSWAP,DLSWAP_FRAME);
    
    Ft_Gpu_Hal_Sleep(1000);//Show the booting up screen.

    home_setup();
    Info();
    Gradient();   
	/* Close all the opened handles */
    Ft_Gpu_Hal_Close(phost);
    Ft_Gpu_Hal_DeInit();
#ifdef MSVC_PLATFORM
	return 0;
#endif
}

void loop()
{
}



/* Nothing beyond this */













