/**
 ******************************************************************************
 * @file    usbh_usr.c
 * @author  MCD Application Team
 * @version V2.1.0
 * @date    19-March-2012
 * @brief   This file includes the usb host library user callbacks
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "usbh_usr.h"
#include "display/display.h"
#include "ff.h"       /* FATFS */
#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bot.h"
#include "main.h"

/** @addtogroup USBH_USER
 * @{
 */

/** @addtogroup USBH_MSC_DEMO_USER_CALLBACKS
 * @{
 */

/** @defgroup USBH_USR 
 * @brief    This file includes the usb host stack user callbacks
 * @{
 */

/** @defgroup USBH_USR_Private_TypesDefinitions
 * @{
 */
/**
 * @}
 */

/** @defgroup USBH_USR_Private_Defines
 * @{
 */
#define IMAGE_BUFFER_SIZE    512
/**
 * @}
 */

/** @defgroup USBH_USR_Private_Macros
 * @{
 */
extern USB_OTG_CORE_HANDLE USB_OTG_Core;
/**
 * @}
 */

/** @defgroup USBH_USR_Private_Variables
 * @{
 */
uint8_t USBH_USR_ApplicationState = USH_USR_FS_INIT;
uint8_t filenameString[15] = {
		0 };

FATFS fatfs;
FIL file;
uint8_t Image_Buf[IMAGE_BUFFER_SIZE];
uint8_t line_idx = 0;

/*  Points to the DEVICE_PROP structure of current device */
/*  The purpose of this register is to speed up the execution */

USBH_Usr_cb_TypeDef USR_cb = {
		USBH_USR_Init, USBH_USR_DeInit, USBH_USR_DeviceAttached, USBH_USR_ResetDevice,
		USBH_USR_DeviceDisconnected, USBH_USR_OverCurrentDetected, USBH_USR_DeviceSpeedDetected,
		USBH_USR_Device_DescAvailable, USBH_USR_DeviceAddressAssigned,
		USBH_USR_Configuration_DescAvailable, USBH_USR_Manufacturer_String, USBH_USR_Product_String,
		USBH_USR_SerialNum_String, USBH_USR_EnumerationDone, USBH_USR_UserInput,
		USBH_USR_MSC_Application, USBH_USR_DeviceNotSupported, USBH_USR_UnrecoveredError

};

/**
 * @}
 */

/** @defgroup USBH_USR_Private_Constants
 * @{
 */
/*--------------- LCD Messages ---------------*/
const char MSG_HOST_INIT[] = "> Host Library Initialized";
const char MSG_DEV_ATTACHED[] = "Dev Att.";
const char MSG_DEV_DISCONNECTED[] = "> Device Disconnected";
const char MSG_DEV_ENUMERATED[] = "Enum";
const char MSG_DEV_HIGHSPEED[] = "Hspeed device";
const char MSG_DEV_FULLSPEED[] = "Fspeed device";
const char MSG_DEV_LOWSPEED[] = "Lspeed device";
const char MSG_DEV_ERROR[] = "> Device fault ";

const char MSG_MSC_CLASS[] = "MSC";
const char MSG_HID_CLASS[] = "HID";
const char MSG_DISK_SIZE[] = "> Size of the disk in MBytes: ";
const char MSG_LUN[] = "> LUN Available in the device:";
const char MSG_ROOT_CONT[] = "> Exploring disk flash ...";
const char MSG_WR_PROTECT[] = "> The disk is write protected";
const char MSG_UNREC_ERROR[] = "> UNRECOVERED ERROR STATE";

/**
 * @}
 */

/** @defgroup USBH_USR_Private_FunctionPrototypes
 * @{
 */
static uint8_t Explore_Disk(char* path, uint8_t recu_level);
static void WriteOutFile(void);
/**
 * @}
 */

/** @defgroup USBH_USR_Private_Functions
 * @{
 */

/**
 * @brief  USBH_USR_Init
 *         Displays the message on LCD for host lib initialization
 * @param  None
 * @retval None
 */
void USBH_USR_Init(void)
{
	static uint8_t startup = 0;

	if(startup == 0)
	{
		startup = 1;
		DISP_StringWrite(0, 0, " USB OTG FS MSC Host");
		DISP_StringWrite(1, 0, "> USB Host library started.");
		DISP_StringWrite(2, 0, "     USB Host Library v2.1.0");
	}
}

/**
 * @brief  USBH_USR_DeviceAttached
 *         Displays the message on LCD on device attached
 * @param  None
 * @retval None
 */
extern xTaskHandle * aDisplayTasks[];
void USBH_USR_DeviceAttached(void)
{
	uint8_t i=0;
	for(i=0;i<DISP_TASKS_COUNT;i++)
		vTaskSuspend(*aDisplayTasks[i]);
	DISP_Clear();
	DISP_StringWrite(0, 0, MSG_DEV_ATTACHED);
}

/**
 * @brief  USBH_USR_UnrecoveredError
 * @param  None
 * @retval None
 */
void USBH_USR_UnrecoveredError(void)
{
	DISP_StringWrite(4, 0, MSG_UNREC_ERROR);
}

/**
 * @brief  USBH_DisconnectEvent
 *         Device disconnect event
 * @param  None
 * @retval Staus
 */
void USBH_USR_DeviceDisconnected(void)
{
	DISP_Clear();
	vTaskResume(**(aDisplayTasks+1));
}
/**
 * @brief  USBH_USR_ResetUSBDevice
 * @param  None
 * @retval None
 */
void USBH_USR_ResetDevice(void)
{
	/* callback for USB-Reset */
}

/**
 * @brief  USBH_USR_DeviceSpeedDetected
 *         Displays the message on LCD for device speed
 * @param  Device speed
 * @retval None
 */
void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed)
{
	if(DeviceSpeed == HPRT0_PRTSPD_HIGH_SPEED)
	{
		DISP_StringWrite(1, 0, MSG_DEV_HIGHSPEED);
	}
	else if(DeviceSpeed == HPRT0_PRTSPD_FULL_SPEED)
	{
		DISP_StringWrite(1, 0, MSG_DEV_FULLSPEED);
	}
	else if(DeviceSpeed == HPRT0_PRTSPD_LOW_SPEED)
	{
		DISP_StringWrite(1, 0, MSG_DEV_LOWSPEED);
	}
	else
	{
		DISP_StringWrite(1, 0, MSG_DEV_ERROR);
	}
}

/**
 * @brief  USBH_USR_Device_DescAvailable
 *         Displays the message on LCD for device descriptor
 * @param  device descriptor
 * @retval None
 */
void USBH_USR_Device_DescAvailable(void *DeviceDesc)
{
	USBH_DevDesc_TypeDef *hs;
	hs = DeviceDesc;
}

/**
 * @brief  USBH_USR_DeviceAddressAssigned
 *         USB device is successfully assigned the Address
 * @param  None
 * @retval None
 */
void USBH_USR_DeviceAddressAssigned(void)
{

}

/**
 * @brief  USBH_USR_Conf_Desc
 *         Displays the message on LCD for configuration descriptor
 * @param  Configuration descriptor
 * @retval None
 */
void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef * cfgDesc,
		USBH_InterfaceDesc_TypeDef *itfDesc, USBH_EpDesc_TypeDef *epDesc)
{
	USBH_InterfaceDesc_TypeDef *id;

	id = itfDesc;

	if((*id).bInterfaceClass == 0x08)
	{
		DISP_StringWrite(2, 0, MSG_MSC_CLASS);
	}
	else if((*id).bInterfaceClass == 0x03)
	{
		DISP_StringWrite(2, 0, MSG_HID_CLASS);
	}
}

/**
 * @brief  USBH_USR_Manufacturer_String
 *         Displays the message on LCD for Manufacturer String
 * @param  Manufacturer String
 * @retval None
 */
void USBH_USR_Manufacturer_String(void *ManufacturerString)
{
//  LCD_UsrLog("Manufacturer : %s", (char *)ManufacturerString);
}

/**
 * @brief  USBH_USR_Product_String
 *         Displays the message on LCD for Product String
 * @param  Product String
 * @retval None
 */
void USBH_USR_Product_String(void *ProductString)
{
//  LCD_UsrLog("Product : %s", (char *)ProductString);
}

/**
 * @brief  USBH_USR_SerialNum_String
 *         Displays the message on LCD for SerialNum_String
 * @param  SerialNum_String
 * @retval None
 */
void USBH_USR_SerialNum_String(void *SerialNumString)
{
//  LCD_UsrLog( "Serial Number : %s", (char *)SerialNumString);
}

/**
 * @brief  EnumerationDone
 *         User response request is displayed to ask application jump to class
 * @param  None
 * @retval None
 */
void USBH_USR_EnumerationDone(void)
{

	/* Enumeration complete */
	DISP_StringWrite(3, 0, MSG_DEV_ENUMERATED);

//  LCD_SetTextColor(Green);
//  LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 42, "To see the root content of the disk : " );
//  LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 30, "Press Key...                       ");
//  LCD_SetTextColor(LCD_LOG_DEFAULT_COLOR);

}

/**
 * @brief  USBH_USR_DeviceNotSupported
 *         Device is not supported
 * @param  None
 * @retval None
 */
void USBH_USR_DeviceNotSupported(void)
{
	DISP_Clear();
	DISP_StringWrite(0, 0, "> Device not supported.");
}

/**
 * @brief  USBH_USR_UserInput
 *         User Action for application state entry
 * @param  None
 * @retval USBH_USR_Status : User response for key button
 */
USBH_USR_Status USBH_USR_UserInput(void)
{
	USBH_USR_Status usbh_usr_status;

	usbh_usr_status = USBH_USR_NO_RESP;

	/*ROT PB is in polling mode to detect user action */
	if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0 ) == RESET)
	{

		usbh_usr_status = USBH_USR_RESP_OK;

	}
	return usbh_usr_status;
}

/**
 * @brief  USBH_USR_OverCurrentDetected
 *         Over Current Detected on VBUS
 * @param  None
 * @retval Staus
 */
void USBH_USR_OverCurrentDetected(void)
{
	DISP_Clear();
	DISP_StringWrite(0, 0, "Overcurr");
}

/**
 * @brief  USBH_USR_MSC_Application
 *         Demo application for mass storage
 * @param  None
 * @retval Staus
 */
int USBH_USR_MSC_Application(void)
{
	FRESULT res;
	uint8_t writeTextBuff[] = "STM32 Connectivity line Host Demo application using FAT_FS   ";
	uint16_t bytesWritten, bytesToWrite;

	switch(USBH_USR_ApplicationState)
	{
	case USH_USR_FS_INIT:

		/* Initialises the File System*/
		if(f_mount(0, &fatfs) != FR_OK)
		{
			/* efs initialisation fails*/
			DISP_Clear();
			DISP_StringWrite(0, 0, "> Cannot initialize File System.");
			return (-1);
		}
//    LCD_UsrLog("> File System initialized.");
//    LCD_UsrLog("> Disk capacity : %d Bytes", USBH_MSC_Param.MSCapacity *
//      USBH_MSC_Param.MSPageLength);

		if(USBH_MSC_Param.MSWriteProtect == DISK_WRITE_PROTECTED)
		{
			DISP_StringWrite(1, 0, MSG_WR_PROTECT);
		}

		USBH_USR_ApplicationState = USH_USR_FS_READLIST;
		break;

	case USH_USR_FS_READLIST:

		DISP_StringWrite(2, 0, MSG_ROOT_CONT);
		DISP_Clear();
		Explore_Disk("0:/", 1);
		line_idx = 0;
		USBH_USR_ApplicationState = USH_USR_FS_WRITEFILE;

		break;

	case USH_USR_FS_WRITEFILE:

		USB_OTG_BSP_mDelay(100);

		/*Key B3 in polling*/
		while((HCD_IsDeviceConnected(&USB_OTG_Core))
				&& (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0 ) == SET))
		{
		}
		/* Writes a text file, STM32.TXT in the disk*/
		DISP_Clear();
		DISP_StringWrite(0, 0, "Write...");
		if(USBH_MSC_Param.MSWriteProtect == DISK_WRITE_PROTECTED)
		{
			DISP_StringWrite(1, 0, "write protected");
			USBH_USR_ApplicationState = USH_USR_FS_DRAW;
			break;
		}

		/* Register work area for logical drives */
		f_mount(0, &fatfs);

		if(f_open(&file, "0:STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
		{
			/* Write buffer to file */
			bytesToWrite = sizeof(writeTextBuff);
			res = f_write(&file, writeTextBuff, bytesToWrite, (void *) &bytesWritten);

			if((bytesWritten == 0) || (res != FR_OK)) /*EOF or Error*/
			{
				DISP_StringWrite(2, 0, "File cannot written.");
			}
			else
			{
				DISP_StringWrite(2, 0, "File created.");
			}

			/*close file and filesystem*/
			f_close(&file);
			WriteOutFile();
			f_mount(0, NULL );
		}

		else
		{
			DISP_StringWrite(2, 0, "File already created.");
		}

		USBH_USR_ApplicationState = USH_USR_FS_DRAW;

		break;

	case USH_USR_FS_DRAW:

//    /*Key B3 in polling*/
//    while((HCD_IsDeviceConnected(&USB_OTG_Core)) &&
//      (STM_EVAL_PBGetState (BUTTON_KEY) == SET))
//    {
//      Toggle_Leds();
//    }
//
//    while(HCD_IsDeviceConnected(&USB_OTG_Core))
//    {
//      if ( f_mount( 0, &fatfs ) != FR_OK )
//      {
//        /* fat_fs initialisation fails*/
//        return(-1);
//      }
//      return Image_Browser("0:/");
//    }
		break;
	default:
		break;
	}
	return (0);
}

/**
 * @brief  Explore_Disk
 *         Displays disk content
 * @param  path: pointer to root path
 * @retval None
 */
static uint8_t Explore_Disk(char* path, uint8_t recu_level)
{

	FRESULT res;
	FILINFO fno;
	DIR dir;
	char *fn;
	char tmp[14];

	res = f_opendir(&dir, path);
	if(res == FR_OK)
	{
		while(HCD_IsDeviceConnected(&USB_OTG_Core))
		{
			res = f_readdir(&dir, &fno);
			if(res != FR_OK || fno.fname[0] == 0)
			{
				break;
			}
			if(fno.fname[0] == '.')
			{
				continue;
			}

			fn = fno.fname;
			strcpy(tmp, fn);

			line_idx++;
			if(line_idx > 6)
			{
				line_idx = 0;
				DISP_StringWrite(7, 0, "Press key to cont.");

				/*Key B3 in polling*/
				while((HCD_IsDeviceConnected(&USB_OTG_Core))
						&& (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0 ) == SET))
				{
				}
			}

			if(recu_level == 1)
			{
				DISP_StringWrite(line_idx, 0, "/-");
			}
			else if(recu_level == 2)
			{
				DISP_StringWrite(line_idx, 0, "//-");
			}
			if((fno.fattrib & AM_MASK) == AM_DIR)
			{
				strcat(tmp, "");
				DISP_StringWrite(line_idx, 18, tmp);
			}
			else
			{
				strcat(tmp, "");
				DISP_StringWrite(line_idx, 18, tmp);
			}

			if(((fno.fattrib & AM_MASK) == AM_DIR) && (recu_level == 1))
			{
				Explore_Disk(fn, 2);
			}
		}
	}
	return res;
}

static void WriteOutFile(void)
{
	FRESULT res;
	FIL fin;
	char str[32];
	char i = 0;

	DISP_Clear();
	res = f_open(&fin, "0:STM32.TXT", FA_OPEN_EXISTING | FA_READ);
	if(res == FR_OK)
	{
		i=0;
		while(!f_eof(&fin))
		{
			f_gets(str,32,&fin);
			DISP_StringWrite(i,0,str);
			i++;
			if(i>7)
				i=0;
		}
		f_close(&fin);
	}
}

/**
 * @brief  USBH_USR_DeInit
 *         Deint User state and associated variables
 * @param  None
 * @retval None
 */
void USBH_USR_DeInit(void)
{
	USBH_USR_ApplicationState = USH_USR_FS_INIT;
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

