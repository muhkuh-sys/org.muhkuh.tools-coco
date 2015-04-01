/***************************************************************************
 *   Copyright (C) 2014 by Subhan Waizi                           		   *
 *                                     									   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "led_analyzer.h"
#include "tcs3472.h"
#include "ftdi.h"

#include <stdio.h>
#define VID 0x0403
#define PID 0x6010






/* Scans for connected ftdi devices and prints their Manufacturer, Description and Serialnumber
Serialnumber(s) of found devices will be stored in asSerial 

 * Return = 0: no device detected
 *        > 0: number of detected ftdi devices (different serial numbers)
 *		  < 0: error with ftdi functions or arraylength
 *
 *
*/


int scan_devices(char** asSerial, unsigned int asLength)
{
	int retVal = 0;
	int i = 0;
	int f;
	int numbOfDevs = 0;
	int numbOfSerials = 0;
	
	
	char manufacturer[128], description[128], serial[128];
	struct ftdi_device_list *devlist, *curdev;
	struct ftdi_context *ftdi;
	
	
	
	//memset(asSerial, 0, sizeof(char*) * asLength);
	
	
	
	if(asLength <1)
	{
		printf("error - length of serialnumber array too small ... \n");
		return -1;
	}


	 if ((ftdi = ftdi_new()) == 0)
		{
			fprintf(stderr, "ftdi_new failed\n");
			ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -1;
		}

	numbOfDevs = ftdi_usb_find_all(ftdi, &devlist, VID, PID);
		
	if(numbOfDevs == 0)
		{
			printf("no ftdi device detected ... quitting.\n");
			ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -1;
		}

	printf("Number of ftdi device(s) found: %d\n\n", numbOfDevs);	
	
	
	
	i = 0;
	for (curdev = devlist; curdev != NULL; i++)
    {
        printf("Scanning device: %d\n", i);
        if ((f = ftdi_usb_get_strings(ftdi, curdev->dev, manufacturer, 128, description, 128, serial, 128)) < 0)
        {
            fprintf(stderr, "ftdi_usb_get_strings failed: %d (%s)\n", f, ftdi_get_error_string(ftdi));
            ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -1;
        }
        printf("Manufacturer: %s, Description: %s, Serial: %s\n\n", manufacturer, description, serial);
        
		
		numbOfSerials++;
		
		asSerial[i] = (char*) malloc(strlen(serial)+1);
		strcpy(asSerial[i], serial);
		curdev = curdev->next;
    }
	
	ftdi_list_free(&devlist);
	ftdi_free(ftdi);
	
	return numbOfSerials;
}

/* function connects to devices with serial numbers given in asSerial 
   
   retVal >0 : number of devices connected to
   retVal <=0: 
   
*/
int connect_to_devices(void** apHandles, int apHlength, char** asSerial)
{

	struct ftdi_context* ftdi;
	int numbOfDevs = get_number_of_serials(asSerial);
	int iArrayPos = 0;
	int devCounter = 0;
	int f;
	
	
	
	printf("Number of LED-Analyzers found: %d\n\n", numbOfDevs);
	
	
	if(2*numbOfDevs > apHlength)
	{
		printf("handlearray too small for number of ftdi-chips found\n");
		printf("needed: %d, got: %d\n", numbOfDevs*2, apHlength);
		return -1;
	}
	
	memset(apHandles, 0, sizeof(void*) * apHlength);
	
	while(devCounter < numbOfDevs)
	{
		printf("Connecting to device %d - %s\n", devCounter, asSerial[devCounter]);
		
		if(iArrayPos+2 <= apHlength)
		{
			/* Ch A */
			
		    if ((apHandles[iArrayPos] = ftdi_new()) == 0)
				{
					fprintf(stderr, "ftdi_new failed!\n");
					return -1;
				}
			
			
			if((f = ftdi_set_interface(apHandles[iArrayPos], INTERFACE_A))<0)
				{
					fprintf(stderr, "Unable to attach to dev %d interface A: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
				
			
			
			if(asSerial[devCounter] == NULL) 
				{
					printf("Serial number non-existent ... please rescan your devices\n");
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
				
			if((f = ftdi_usb_open_desc(apHandles[iArrayPos], VID, PID, NULL, asSerial[devCounter])<0))
				{
					fprintf(stderr, "unable to open dev %d interface A: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else printf("ftdi dev %d Channel A - open succeeded\n", devCounter);
			
			if((f=ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE)) < 0)
				{
					fprintf(stderr, "unable to set the mode on dev %d Channel A: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else  printf("enabling bitbang mode on dev %d Channel A\n", devCounter);
			
			iArrayPos ++;
						
			/* Ch B */
			
		    if ((apHandles[iArrayPos] = ftdi_new()) == 0)
				{
					fprintf(stderr, "ftdi_new failed!\n");
					return -1;
				}
			
			if((f = ftdi_set_interface(apHandles[iArrayPos], INTERFACE_B))<0)
				{
					fprintf(stderr, "Unable to attach to dev %d interface B: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			
			if(asSerial[devCounter] == NULL) 
				{
					printf("Serial number non-existent ... please rescan your devices\n");
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			
			if((f = ftdi_usb_open_desc(apHandles[iArrayPos],VID, PID, NULL, asSerial[devCounter])<0))	
				{
					fprintf(stderr, "unable to open dev %d interface B: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else printf("ftdi dev %d Channel B - open succeeded\n", devCounter);
			
			if((f=ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE)) < 0)
				{
					fprintf(stderr, "unable to set the mode on dev %d Channel B: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			
			else  printf("enabling bitbang mode on dev %d Channel B\n", devCounter);
			
			iArrayPos ++;
			
		}
		/* Go to the next device found */
		devCounter ++;
		
		printf("\n");
		
	}
	
	printf("\n");
	return numbOfDevs;
	
	
}

int get_number_of_serials(char** asSerial)
{
	int counter = 0;
	int i = 0;
	
	while(asSerial[counter]!=NULL) counter ++;
	
	return counter;
}


/* Diese Funktion detektiert alle Farbsensor devices am PC und öffnet sie.
 * Die handles auf die geöffneten Devices werden in einer Liste zurückgegeben.
 *
 * Return = 0: no device detected
 *        > 0: number of detected ftdi devices
 *		  < 0: error with ftdi functions
 *
 *
 */
 
 
int detect_devices(void** apHandles, int apHlength)
{
	int iArrayPos;
	int iResult;
	int i, f;
	int numbOfDevs = 0;
	int devCounter = 0;
	char manufacturer[128], description[128], serial[10][128];
	struct ftdi_device_list *devlist, *curdev;
	struct ftdi_context *ftdi;
	//struct ftdi_context *ftdiA, *ftdiB;
	

	 if ((ftdi = ftdi_new()) == 0)
		{
			fprintf(stderr, "ftdi_new failed\n");
			ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -1;
		}
		
	
		
	
	
	numbOfDevs = ftdi_usb_find_all(ftdi, &devlist, VID, PID);
		
	if(numbOfDevs == 0)
		{
			printf("no ftdi device detected ... quitting.\n");
			ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -1;
		}
	
	

	printf("Number of device(s) found: %d\n\n", numbOfDevs);
	
	/* This ftdi_specific function was only needed in order to detect the number of devices */
	i = 0;
	for (curdev = devlist; curdev != NULL; i++)
    {
        printf("Checking device: %d\n", i);
        if ((f = ftdi_usb_get_strings(ftdi, curdev->dev, manufacturer, 128, description, 128, serial[i], 128)) < 0)
        {
            fprintf(stderr, "ftdi_usb_get_strings failed: %d (%s)\n", f, ftdi_get_error_string(ftdi));
            ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -1;
        }
        printf("Manufacturer: %s, Description: %s, Serial: %s\n\n", manufacturer, description, serial[i]);
        curdev = curdev->next;
    }
	
	ftdi_list_free(&devlist);
	ftdi_free(ftdi);
	
	if(2*numbOfDevs > apHlength)
		{
			printf("handlearray too small for number of ftdi-chips found\n");
			printf("needed: %d, got: %d\n", numbOfDevs*2, apHlength);
			return -1;
		}

	
	
	memset(apHandles, 0, sizeof(void*) * apHlength);
	/* Erstes Handle an Offset 0 im Array. */
	iArrayPos = 0;
	
	
	printf("--------------------------------------------------------\n");
    printf("USB - FUNCTIONS\n");
	printf("--------------------------------------------------------\n");

	while(devCounter < numbOfDevs)
	{
		
	
		if(iArrayPos+2 <= apHlength)
		{
			/* Ch A */
			


		    if ((apHandles[iArrayPos] = ftdi_new()) == 0)
				{
					fprintf(stderr, "ftdi_new failed!\n");
					return -1;
				}
			
			if((f = ftdi_set_interface(apHandles[iArrayPos], INTERFACE_A))<0)
				{
					fprintf(stderr, "Unable to attach to dev %d interface A: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
				
			//if((f = ftdi_usb_open_desc_index(apHandles[iArrayPos], VID, PID, NULL, NULL, devCounter)<0))
			if((f = ftdi_usb_open_desc(apHandles[iArrayPos], VID, PID, NULL, serial[devCounter])<0))
				{
					fprintf(stderr, "unable to open dev %d interface A: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else printf("ftdi dev %d Channel A - open succeeded\n", devCounter);
			
			if((f=ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE)) < 0)
				{
					fprintf(stderr, "unable to set the mode on dev %d Channel A: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else  printf("enabling bitbang mode on dev %d Channel A\n", devCounter);
			
			iArrayPos ++;
						
			/* Ch B */
			
		    if ((apHandles[iArrayPos] = ftdi_new()) == 0)
				{
					fprintf(stderr, "ftdi_new failed!\n");
					return -1;
				}
			
			if((f = ftdi_set_interface(apHandles[iArrayPos], INTERFACE_B))<0)
				{
					fprintf(stderr, "Unable to attach to dev %d interface B: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
				
			//if((f = ftdi_usb_open_desc_index(apHandles[iArrayPos],VID, PID, NULL, NULL, devCounter)<0))
			if((f = ftdi_usb_open_desc(apHandles[iArrayPos],VID, PID, NULL, serial[devCounter])<0))	
				{
					fprintf(stderr, "unable to open dev %d interface B: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else printf("ftdi dev %d Channel B - open succeeded\n", devCounter);
			
			if((f=ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE)) < 0)
				{
					fprintf(stderr, "unable to set the mode on dev %d Channel B: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			
			else  printf("enabling bitbang mode on dev %d Channel B\n", devCounter);
			
			iArrayPos ++;
			
		}
		/* Go to the next device found */
		devCounter ++;
		
		
	}
	
	printf("\n\n");
	return numbOfDevs;
}

/* Each handle controls 8 color sensors
*  Each device normally has 2 handles (if ft2232h)
*  Each color sensor returns 4 unsigned short values
*/


int get_handleLength(void ** apHandles)
{
	int iCounter = 0;
	while(apHandles[iCounter] != NULL)
		{
			iCounter++;
		}
		
	
	return iCounter;
}


/* This function returns the device number given to a certain handleindex */
int handleToDevice(int handle)
{	
	return (int)(handle/2);
}


/* Set the integration time of one sensor given in uiX which ranges from 0 ... 15 */
int set_intTime_x(void** apHandles, int devIndex, unsigned long integrationtime, unsigned int uiX)
{
	int handleIndex = devIndex * 2;
	
	if(tcs_setIntegrationTime_x(apHandles[handleIndex], apHandles[handleIndex+1], integrationtime, uiX) != 0)
	{
		printf("failed to set integration time for sensor %d on dev %d ...\n", uiX+1, devIndex);
		return 1;
	}
	return 0;
}

int set_gain_x(void** apHandles, int devIndex, unsigned long gain, unsigned int uiX)
{
	int handleIndex = devIndex * 2;
		
	if(tcs_setGain_x(apHandles[handleIndex], apHandles[handleIndex+1], gain, uiX) != 0)
	{
		printf("failed to set gain for sensor %d on dev %d ...\n", uiX+1, devIndex);
		return 1;		
	}
	return 0;
}


/* Initialize the sensors under a certain device# and handle#
	Initializing a sensor, sets up its gain and integration time, clears any priorly generated interrupt, turns the sensor on, and waits 
	an integration time cycle, so that data is already ready for the next color reading */
int init_sensors(void** apHandles, int devIndex, unsigned long integrationtime, unsigned long gain)
{
	int iResult = 0;
	/* 2 handles per device, dev 0 has handles 0,1 .. dev 1 has handles 2,3 and so on*/
	int handleIndex = devIndex * 2;
	
	int iHandleLength = get_handleLength(apHandles);
	
	printf("Initializing on devIndex: %d\n", devIndex);
	
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		return 1;
	}
	

	if(tcs_setIntegrationTime(apHandles[handleIndex], apHandles[handleIndex+1], integrationtime) != 0)
	{
		printf("failed to set integration time on dev %d ...\n", devIndex);
		return 1;
	}
			
	if(tcs_setGain(apHandles[handleIndex], apHandles[handleIndex+1], gain) != 0)
	{
		printf("failed to set integration time on dev %d...\n", devIndex);
		return 2;
	}
			
	if(tcs_clearInt(apHandles[handleIndex], apHandles[handleIndex+1]) != 0)
	{
		printf("failed to clear interrupt channel on dev %d...\n", devIndex);
		return 3;
	}
			
	if(tcs_ON(apHandles[handleIndex], apHandles[handleIndex+1]) != 0)
	{
		printf("failed to turn the sensors on on dev %d...\n", devIndex);
		return 4;
	}
			
	tcs_waitIntegrationtime(integrationtime);
	
	printf("initializing successful on devIndex: %d\n", devIndex);
	return iResult;
}


/* 
MISSING DUE TO LACK OF APPROPRIATE HARDWARE

RETURN ERRORCODE INSTEAD OF 0 (0 = all ok)

REPLACE IF HARDWARE ARRIVES
*/

/* Function reads out four colors (RGBC) of each sensor (16) under a device */
int read_colors(void** apHandles, int devIndex, unsigned short* ausClear, unsigned short* ausRed,
														  unsigned short* ausGreen, unsigned short* ausBlue)
{
	int iResult = 0;
	int iHandleLength = get_handleLength(apHandles);
	unsigned char aucTempbuffer[16];
	int handleIndex = devIndex * 2;
	
	unsigned short int errorcode = 0;
	
	// Transform device index into handle index, as each device has two handles */
	printf("Reading colors on devIndex: %d\n", devIndex);
	
	if(handleIndex >= iHandleLength)
		{
			printf("Exceeded maximum amount of handles ... \n");
			printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
			return 1;
		}
	
	// First perform the functions needed for assertion of correct color values */
	
	if((errorcode = tcs_identify(apHandles[handleIndex], apHandles[handleIndex+1], aucTempbuffer)) != 0)
		{
			printf(" errorcode identification: %d \n", errorcode);
			
			/* 
			MISSING DUE TO LACK OF APPROPRIATE HARDWARE

			RETURN ERRORCODE INSTEAD OF 0 (0 = all ok)

			REPLACE IF HARDWARE ARRIVES
			*/

		}
	
	if((errorcode = tcs_waitForData(apHandles[handleIndex], apHandles[handleIndex+1], aucTempbuffer)) != 0)
		{
			printf(" errorcode incomplete conversion: %d \n", errorcode);
			
			
		/* 
		MISSING DUE TO LACK OF APPROPRIATE HARDWARE

		RETURN ERRORCODE INSTEAD OF 0 (0 = all ok)

		REPLACE IF HARDWARE ARRIVES
		*/

			
			return 0;
		}
		
	
	tcs_readColour(apHandles[handleIndex], apHandles[handleIndex+1], ausClear, CLEAR);
	tcs_readColour(apHandles[handleIndex], apHandles[handleIndex+1], ausRed, RED);
	tcs_readColour(apHandles[handleIndex], apHandles[handleIndex+1], ausGreen, GREEN);
	tcs_readColour(apHandles[handleIndex], apHandles[handleIndex+1], ausBlue, BLUE);


	printf("reading colors successful\n");
	return iResult;
	
}														 

/* This function should be called after any color reading in order to find out of the colors of the sensor are valid
and can be used for led detection 
Colors are not valid if the gain/integration time setting was too high, which could result in a a color out of range
or the color sets are not valid due to any other reason */
int check_validity(void** apHandles, int devIndex, unsigned short* ausClear, unsigned long integrationtime)
{
	int iResult = 0;
	int iHandleLength = get_handleLength(apHandles);
	int handleIndex = devIndex * 2;
	unsigned char aucReadbuffer[16];

	unsigned short int errorcode = 0;
	
	
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		return 1;
	}
	
	
	if((errorcode = tcs_exClear(apHandles[handleIndex], apHandles[handleIndex+1], ausClear, integrationtime)) != 0)
	{
		printf(" errorcode exceeded clear: %d \n", errorcode);
		
		/* 
		MISSING DUE TO LACK OF APPROPRIATE HARDWARE

		RETURN ERRORCODE INSTEAD OF 0 (0 = all ok)

		REPLACE IF HARDWARE ARRIVES
		*/

	}
	if((errorcode = tcs_rgbcInvalid(apHandles[handleIndex], apHandles[handleIndex+1], aucReadbuffer)) != 0)
	{
		printf(" errorcode invalid rgbc: %d \n", errorcode);
		
		/* 
		MISSING DUE TO LACK OF APPROPRIATE HARDWARE

		RETURN ERRORCODE INSTEAD OF 0 (0 = all ok)

		REPLACE IF HARDWARE ARRIVES
		
		
		*/
		return 0;
	}
	
	return iResult;
}

int free_devices(void** apHandles)
{
	int iResult = 0;
	int index = 0;
	int iHandleLength = get_handleLength(apHandles);
	
	printf("Number of handles to delete: %d\n", iHandleLength);
	while(index < iHandleLength)
	{
		ftdi_usb_close(apHandles[index]);
		ftdi_free(apHandles[index]);
		printf("Freeing handle # %d on device # %d\n", index, handleToDevice(index));
		index ++;
	}
	/*
	Free was successful over all handles
	*/
		
	return iResult;
}

int get_gainSettings(void** apHandles, int devIndex, unsigned char* aucGains)
{
	
	int iHandleLength = get_handleLength(apHandles);
	int handleIndex = devIndex * 2;
	int iResult = 0;
	
	if(handleIndex >= iHandleLength)
	{
			printf("Exceeded maximum amount of handles ... \n");
			printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
			return 1;
	}
	
	iResult = tcs_getGain(apHandles[handleIndex], apHandles[handleIndex+1], aucGains);
	return iResult;
}

int get_intTimeSettings(void** apHandles, int devIndex, unsigned char* aucIntTimeSettings)
{

	int iHandleLength = get_handleLength(apHandles);
	int handleIndex = devIndex * 2;
	int iResult = 0;
	
	if(handleIndex >= iHandleLength)
	{
			printf("Exceeded maximum amount of handles ... \n");
			printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
			return 1;
	}

	
	iResult = tcs_getIntegrationtime(apHandles[handleIndex], apHandles[handleIndex+1], aucIntTimeSettings);
	return iResult;

}