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

Author : FTDI 

Revision History: 
0.1 - date 2015.08.08 - Initial Version


 
 */
#ifndef FT_ILI9488_H
#define FT_ILI9488_H

#ifdef ENABLE_ILI9488_HVGA_PORTRAIT

/* Macros for ILI driver */

#define ILI9488_CS_LOW (gpio_write(33, 0))
#define ILI9488_CS_HIGH (gpio_write(33, 1))
#define ILI9488_CLK_LOW (gpio_write(27, 0))
#define ILI9488_CLK_HIGH (gpio_write(27, 1))
#define ILI9488_MOSI_LOW (gpio_write(29, 0))
#define ILI9488_MOSI_HIGH (gpio_write(29, 1))
#define ILI9488_DCX_LOW (gpio_write(34, 0))
#define ILI9488_DCX_HIGH (gpio_write(34, 1))

#define ILI9488_MISO_LOWHIGH (gpio_read(30))

//ILI9488 commands
#define ILI9488_CMD_SOFTWARE_RESET				(0x01)//next new command only after 5ms and sleep commands only after 120ms
#define ILI9488_CMD_READ_DISPLAY_IDENTIFICATION	(0x04)
#define ILI9488_CMD_READ_DISPLAY_POWERMODE		(0x0A)
#define ILI9488_CMD_READ_MADCTRL				(0x0B)
#define ILI9488_CMD_READ_PIXEL_FORMAT			(0x0C)

#define ILI9488_READ_DISPLAY_SIGNALMODE			(0x0E)

#define ILI9488_CMD_SLEEP_IN					(0x10)
#define ILI9488_CMD_SLEEP_OUT					(0x11)
#define ILI9488_CMD_NORMAL_DISPLAY_ON			(0x13)
#define ILI9488_CMD_ALLPIXELOFF					(0x22)
#define ILI9488_CMD_ALLPIXELON					(0x23)
#define ILI9488_CMD_DISPLAYOFF					(0x28)
#define ILI9488_CMD_DISPLAYON					(0x29)//need to do this
#define ILI9488_CMD_COLOMNADDR					(0x2A)
#define ILI9488_CMD_ROWADDR						(0x2B)
#define ILI9488_CMD_TEARINGEFFECT_LINEOFF		(0x34)
#define ILI9488_CMD_TEARINGEFFECT_LINEON		(0x35)

#define ILI9488_CMD_MADCTRL						(0x36)
#define ILI9488_CMD_IDLE_OFF					(0x38)
#define ILI9488_CMD_IDLE_ON						(0x39)

#define ILI9488_CMD_INTERFACE_PIXEL_FORMAT		(0x3A)//default is 06 and DBI 18 bits/pixel
#define ILI9488_INTERFACE_PIXEL_FORMAT_18BIT_DPI	(0x66)
#define ILI9488_INTERFACE_PIXEL_FORMAT_24BIT_DPI	(0x70)

#define ILI9488_CMD_READ_ID1					(0xDA)
#define ILI9488_CMD_READ_ID2					(0xDB)
#define ILI9488_CMD_READ_ID3					(0xDC)

#define ILI9488_ID1								(0x54)
#define ILI9488_ID2								(0x80)
#define ILI9488_ID3								(0x66)

#define ILI9488_CMD_WRITE_CONTROL_DISPLAY		(0x53)
#define ILI9488_CMD_WRITE_BRIGHTNESSVALUE		(0x51)
#define ILI9488_CMD_READ_BRIGHTNESSVALUE		(0x52)
#define ILI9488_CMD_WRITE_CTRLDISPLAYVALUE		(0x53)
#define ILI9488_CMD_READ_CTRLDISPLAYVALUE		(0x54)

//extended functions
#define ILI9488_CMD_INTERFACE_MODE_CONTROL		(0xB0)
#define ILI9488_CMD_FRAME_RATE_CONTROL			(0xB1)
#define ILI9488_CMD_BLANCKING_PORCH_CONTROL		(0xB5)
#define ILI9488_CMD_DISPLAY_FUNCTION_CONTROL 	(0xB6)
#define ILI9488_CMD_READ_ID4					(0xD3)
#define ILI9488_CMD_IMAGEFUNCTION				(0xE9)

#define ILI9488_ID4								(0x00009488L)

#define ILI9488_CMD_SPI_READCMDSETTINGS			(0xFB)


ft_int16_t ILI9488_SPI_WriteCmd(ft_uint8_t cmd, ft_uint8_t data);
ft_int16_t ILI9488_SPI_WriteCmdN(ft_uint8_t cmd, ft_uint8_t bytecount,ft_uint8_t *pbuff);
ft_uint8_t ILI9488_SPI_Read(ft_uint8_t cmd);
ft_uint8_t ILI9488_SPI_ReadN(ft_uint8_t cmd,ft_uint8_t numbytes,ft_uint8_t *pbuffer);
ft_uint32_t ILI9488_SPI_ReadRDDID(ft_uint8_t cmd);
ft_uint32_t ILI9488_SPI_ReadRDDST(ft_uint8_t cmd);
ft_void_t ILI9488_Bootup();

#endif /* ENABLE_ILI9488_HVGA_PORTRAIT */

#endif  /*FT_ILI9488_H*/
