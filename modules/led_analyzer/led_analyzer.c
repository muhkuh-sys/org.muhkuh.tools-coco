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
#define VID 0x1939
#define PID 0x0024



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
	int i;
	int f;
	int numbOfDevs = 0;
	int numbOfSerials = 0;
	
	const char sMatch[] = "COLOR-CTRL";
	
	char manufacturer[128], description[128], serial[128];
	struct ftdi_device_list *devlist, *curdev;
	struct ftdi_context *ftdi;
	
	
	if(asLength <1)
	{
		printf("error - length of serialnumber array too small ... \n");
		return -1;
	}


	 if ((ftdi = ftdi_new()) == 0)
		{
			fprintf(stderr, "... ftdi_new failed\n");
			ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -1;
		}

	numbOfDevs = ftdi_usb_find_all(ftdi, &devlist, VID, PID);
		
	if(numbOfDevs == 0)
		{
			printf("... no ftdi device detected ... quitting.\n");
			ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -1;
		}

	printf("\n\n");
	
	i = 0;
	for (curdev = devlist; curdev != NULL; i++)
    {
        printf("Scanning device %d\n", i);
        if ((f = ftdi_usb_get_strings(ftdi, curdev->dev, manufacturer, 128, description, 128, serial, 128)) < 0)
        {
            fprintf(stderr, "... ftdi_usb_get_strings failed: %d (%s)\n", f, ftdi_get_error_string(ftdi));
            ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -1;
        }
        printf("Manufacturer: %s, Description: %s, Serial: %s\n\n", manufacturer, description, serial);
        
		
		
		if(strcmp(sMatch, description) == 0)
		{
			numbOfSerials++;
			asSerial[i] = (char*) malloc(sizeof(serial));
			strcpy(asSerial[i], serial);			
		}
		
		curdev = curdev->next;
    }
	
	ftdi_list_free(&devlist);
	ftdi_free(ftdi);
	
	if(numbOfSerials == 0)
	{
			printf("... no color controller(s) detected.\n");
			ftdi_list_free(&devlist);
			ftdi_free(ftdi);
	}
	
	return numbOfSerials;
}

/* function connects to devices with serial numbers given in asSerial 
   
   retVal >0  : number of devices connected to
   retVal <=0 : error with ftdi functions or arraylength
   
*/
int connect_to_devices(void** apHandles, int apHlength, char** asSerial)
{

	int numbOfDevs = get_number_of_serials(asSerial);
	int iArrayPos = 0;
	int devCounter = 0;
	int f;
	
	printf("Number of Color Controllers found: %d\n\n", numbOfDevs);

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
					fprintf(stderr, "... ftdi_new failed!\n");
					return -1;
				}
			
			
			if((f = ftdi_set_interface(apHandles[iArrayPos], INTERFACE_A))<0)
				{
					fprintf(stderr, "... unable to attach to device %d interface A: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
				
			
			
			if(asSerial[devCounter] == NULL) 
				{
					printf("... serial number non-existent ... please rescan your devices\n");
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
				
			if((f = ftdi_usb_open_desc(apHandles[iArrayPos], VID, PID, NULL, asSerial[devCounter])<0))
				{
					fprintf(stderr, "... unable to open device %d interface A: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else printf("ftdi device %d Channel A - open succeeded\n", devCounter);
			
			if((f=ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE)) < 0)
				{
					fprintf(stderr, "... unable to set the mode on device %d Channel A: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else  printf("enabling bitbang mode on device %d Channel A\n", devCounter);
			
			iArrayPos ++;
						
			/* Ch B */
			
		    if ((apHandles[iArrayPos] = ftdi_new()) == 0)
				{
					fprintf(stderr, "... ftdi_new failed!\n");
					return -1;
				}
			
			if((f = ftdi_set_interface(apHandles[iArrayPos], INTERFACE_B))<0)
				{
					fprintf(stderr, "... unable to attach to device %d interface B: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			
			if(asSerial[devCounter] == NULL) 
				{
					printf("... serial number non-existent ... please rescan your devices\n");
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			
			if((f = ftdi_usb_open_desc(apHandles[iArrayPos],VID, PID, NULL, asSerial[devCounter])<0))	
				{
					fprintf(stderr, "... unable to open device %d interface B: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else printf("ftdi device %d Channel B - open succeeded\n", devCounter);
			
			if((f=ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE)) < 0)
				{
					fprintf(stderr, "unable to set the mode on device %d Channel B: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			
			else  printf("enabling bitbang mode on device %d Channel B\n", devCounter);
			
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

	while(asSerial[counter]!=NULL) counter ++;
	
	return counter;
}


/* function detects all devices with a given VID and PID and opens them 
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
	int i, f;
	int numbOfDevs = 0;
	int devCounter = 0;
	char manufacturer[128], description[128], serial[10][128];
	struct ftdi_device_list *devlist, *curdev;
	struct ftdi_context *ftdi;
	
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
					fprintf(stderr, "Unable to attach to device %d interface A: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
				
			//if((f = ftdi_usb_open_desc_index(apHandles[iArrayPos], VID, PID, NULL, NULL, devCounter)<0))
			if((f = ftdi_usb_open_desc(apHandles[iArrayPos], VID, PID, NULL, serial[devCounter])<0))
				{
					fprintf(stderr, "unable to open device %d interface A: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else printf("ftdi device %d Channel A - open succeeded\n", devCounter);
			
			if((f=ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE)) < 0)
				{
					fprintf(stderr, "unable to set the mode on device %d Channel A: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else  printf("enabling bitbang mode on device %d Channel A\n", devCounter);
			
			iArrayPos ++;
						
			/* Ch B */
			
		    if ((apHandles[iArrayPos] = ftdi_new()) == 0)
				{
					fprintf(stderr, "ftdi_new failed!\n");
					return -1;
				}
			
			if((f = ftdi_set_interface(apHandles[iArrayPos], INTERFACE_B))<0)
				{
					fprintf(stderr, "Unable to attach to device %d interface B: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
				
			//if((f = ftdi_usb_open_desc_index(apHandles[iArrayPos],VID, PID, NULL, NULL, devCounter)<0))
			if((f = ftdi_usb_open_desc(apHandles[iArrayPos],VID, PID, NULL, serial[devCounter])<0))	
				{
					fprintf(stderr, "unable to open device %d interface B: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_deinit(apHandles[iArrayPos]);
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			else printf("ftdi device %d Channel B - open succeeded\n", devCounter);
			
			if((f=ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE)) < 0)
				{
					fprintf(stderr, "unable to set the mode on device %d Channel B: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
					ftdi_free(apHandles[iArrayPos]);
					return -1;
				}
			
			else  printf("enabling bitbang mode on device %d Channel B\n", devCounter);
			
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
int set_intTime_x(void** apHandles, int devIndex, unsigned char integrationtime, unsigned int uiX)
{
	int handleIndex = devIndex * 2;
	
	if(tcs_setIntegrationTime_x(apHandles[handleIndex], apHandles[handleIndex+1], integrationtime, uiX) != 0)
	{
		printf("... failed to set integration time for sensor %d on device %d ...\n", uiX+1, devIndex);
		return 1;
	}
	return 0;
}

/* Set the gain of one sensor given in uiX which ranges from 0 ... 15 */
int set_gain_x(void** apHandles, int devIndex, unsigned char gain, unsigned int uiX)
{
	int handleIndex = devIndex * 2;
		
	if(tcs_setGain_x(apHandles[handleIndex], apHandles[handleIndex+1], gain, uiX) != 0)
	{
		printf("... failed to set gain for sensor %d on device %d ...\n", uiX+1, devIndex);
		return 1;		
	}
	return 0;
}


/* Initialize the sensors under a certain device# and handle#
	Initializing a sensor, sets up its gain and integration time, clears any priorly generated interrupt, turns the sensor on, and waits 
	an integration time cycle, so that data is already ready for the next color reading 

	
*/
int init_sensors(void** apHandles, int devIndex)
{
	/* 2 handles per device, device 0 has handles 0,1 .. device 1 has handles 2,3 and so on*/
	int handleIndex = devIndex * 2;
	int errorcode = 0;
	unsigned char aucTempbuffer[16];
	
	int iHandleLength = get_handleLength(apHandles);
	
	
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		return -1;
	}
	
			
	if(tcs_clearInt(apHandles[handleIndex], apHandles[handleIndex+1]) != 0)
	{
		printf("... failed to clear interrupt channel on device %d...\n", devIndex);
	}
			
	if(tcs_ON(apHandles[handleIndex], apHandles[handleIndex+1]) != 0)
	{
		printf("... failed to turn the sensors on on device %d...\n", devIndex);
	}
			
		
	if((errorcode = tcs_identify(apHandles[handleIndex], apHandles[handleIndex+1], aucTempbuffer)) != 0)
	{
		return errorcode;
	}
	
	
	printf("Initializing successful on device %d.\n", devIndex);
	return errorcode;
}



/* Function reads out four colors (RGBC) of each sensor (16) under a device */
int read_colors(void** apHandles, int devIndex, unsigned short* ausClear, unsigned short* ausRed,
				unsigned short* ausGreen, unsigned short* ausBlue, float* afBrightness)
{
	int iHandleLength = get_handleLength(apHandles);
	
	unsigned char aucTempbuffer[16];
	unsigned char aucIntegrationtime[16];
	unsigned char aucGain[16];
	
	int handleIndex = devIndex * 2;
	int gainDivisor = 0;
	
	int errorcode = 0;
	
	// Transform device index into handle index, as each device has two handles */
	
	if(handleIndex >= iHandleLength)
		{
			printf("Exceeded maximum amount of handles ... \n");
			printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
			return -1;
		}

	/* Check if sensors' ADCs have completed conversion */
	if((errorcode = tcs_waitForData(apHandles[handleIndex], apHandles[handleIndex+1])) != 0)
		{
			return errorcode;
		}
		
	tcs_getIntegrationtime(apHandles[handleIndex], apHandles[handleIndex+1], aucIntegrationtime);
	tcs_getGain(apHandles[handleIndex], apHandles[handleIndex+1], aucGain);
	
	tcs_readColour(apHandles[handleIndex], apHandles[handleIndex+1], ausClear, CLEAR);
	tcs_readColour(apHandles[handleIndex], apHandles[handleIndex+1], ausRed, RED);
	tcs_readColour(apHandles[handleIndex], apHandles[handleIndex+1], ausGreen, GREEN);
	tcs_readColour(apHandles[handleIndex], apHandles[handleIndex+1], ausBlue, BLUE);

	if((errorcode = tcs_exClear(apHandles[handleIndex], apHandles[handleIndex+1], ausClear, aucIntegrationtime)) != 0)
	{		
		return errorcode;
	}
	
	if((errorcode = tcs_rgbcInvalid(apHandles[handleIndex], apHandles[handleIndex+1])) != 0)
	{
		return errorcode;
	}
		
	int i;
	for(i=0; i<16; i++)
	{
		gainDivisor = getGainDivisor(aucGain[i]);
		
		switch(aucIntegrationtime[i])
		{
            case TCS3472_INTEGRATION_2_4ms:
                afBrightness[i] = (float)ausClear[i]/gainDivisor/1024;
                break;

            case TCS3472_INTEGRATION_24ms:
				afBrightness[i] = (float)ausClear[i]/gainDivisor/10240;
                break;

            case TCS3472_INTEGRATION_100ms:
                afBrightness[i] = (float)ausClear[i]/gainDivisor/43007;
                break;

            case TCS3472_INTEGRATION_154ms:
                afBrightness[i] = (float)ausClear[i]/gainDivisor/65535;
                break;

            case TCS3472_INTEGRATION_200ms:
				afBrightness[i] = (float)ausClear[i]/gainDivisor/65535;
                break;

            case TCS3472_INTEGRATION_700ms:
                afBrightness[i] = (float)ausClear[i]/gainDivisor/65535;
                break;
				
			default: 
				printf("Unknown integration time setting - please take a enum value\n");
				afBrightness[i] = 0.;
				break;
		}
		
	}

	printf("Reading colors successful.\n");
	return errorcode;
	
}														 

/* This function should be called after any color reading in order to find out of the colors of the sensor are valid
and can be used for led detection 
Colors are not valid if the gain/integration time setting was too high, which could result in a a color out of range
or the color sets are not valid due to any other reason */

int check_validity(void** apHandles, int devIndex, unsigned short* ausClear, unsigned char* aucIntegrationtime)
{
	int iHandleLength = get_handleLength(apHandles);
	int handleIndex = devIndex * 2;

	int errorcode = 0;
	
	
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		return -1;
	}
	
	
	if((errorcode = tcs_exClear(apHandles[handleIndex], apHandles[handleIndex+1], ausClear, aucIntegrationtime)) != 0)
	{		
		return errorcode;
	}
	
	if((errorcode = tcs_rgbcInvalid(apHandles[handleIndex], apHandles[handleIndex+1])) != 0)
	{
		return errorcode;
	}
	
	return errorcode;
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

int set_gain(void** apHandles, int devIndex, unsigned char gain)
{
	int iHandleLength = get_handleLength(apHandles);
	int handleIndex = devIndex * 2;
	int iResult = 0;
	
	if(handleIndex >= iHandleLength)
	{
			printf("Exceeded maximum amount of handles ... \n");
			printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
			return -1;
	}
	
	iResult = tcs_setGain(apHandles[handleIndex], apHandles[handleIndex+1], gain);
	return iResult;		
}
int get_gain(void** apHandles, int devIndex, unsigned char* aucGains)
{
	
	int iHandleLength = get_handleLength(apHandles);
	int handleIndex = devIndex * 2;
	int iResult = 0;
	
	if(handleIndex >= iHandleLength)
	{
			printf("Exceeded maximum amount of handles ... \n");
			printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
			return -1;
	}
	
	iResult = tcs_getGain(apHandles[handleIndex], apHandles[handleIndex+1], aucGains);
	return iResult;
}

int set_intTime(void** apHandles, int devIndex, unsigned char integrationtime)
{
	int iHandleLength = get_handleLength(apHandles);
	int handleIndex = devIndex * 2;
	int iResult = 0;
	
	if(handleIndex >= iHandleLength)
	{
			printf("Exceeded maximum amount of handles ... \n");
			printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
			return -1;
	}
	
	iResult = tcs_setIntegrationTime(apHandles[handleIndex], apHandles[handleIndex+1], integrationtime);
	return iResult;
		
}

int get_intTime(void** apHandles, int devIndex, unsigned char* aucIntegrationtime)
{
	int iHandleLength = get_handleLength(apHandles);
	int handleIndex = devIndex * 2;
	int iResult = 0;
	
	if(handleIndex >= iHandleLength)
	{
			printf("Exceeded maximum amount of handles ... \n");
			printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
			return -1;
	}
	
	iResult = tcs_getIntegrationtime(apHandles[handleIndex], apHandles[handleIndex+1], aucIntegrationtime);
	return iResult;

}

void wait4Conversion(unsigned int uiWaitTime)
{

	if((uiWaitTime > 0) && (uiWaitTime <= 700))
		Sleep(uiWaitTime);
	
	else Sleep(200);
}


/* Functions swaps two strings in the asSerial string array 

	retVal = -1: swapping didn't work due to overindexing the asSerial array 
	retVal = 0 : everything ok -- swapping worked 
*/


int swap_serialPos(char** asSerial, unsigned int pos1, unsigned int pos2)
{
	int numbOfDevs = get_number_of_serials(asSerial);
	char temp[128];
	
	if(pos1 >= numbOfDevs || pos1 < 0) 
	{
		printf("Reaching out of seralnumber array ... cannot swap\n");
		return -1;
	}
	
	if(pos2 >= numbOfDevs || pos2 < 0) 
	{
		printf("Reaching out of seralnumber array ... cannot swap\n");
		return -1;
	}
	
	
	/* Temporary store of Serials old position */
	strcpy(temp, asSerial[pos1]);
	/* store the serial number into the new position */
	strcpy(asSerial[pos1], asSerial[pos2]);
	/* Restore the old position */
	strcpy(asSerial[pos2], temp);
	
	return 0;

	
}


int getSerialIndex(char** asSerial, char* curSerial)
{
	int numbOfDevs = get_number_of_serials(asSerial);
	int i;
	int index = -1;
	
	
	for(i = 0; i < numbOfDevs; i++)
	{
		if(strcmp(curSerial, asSerial[i]) == 0) index = i;
	}
	
	printf("... serial not found - cannot return an index\n");
	return -1;

}


int swap_up(char** asSerial, char* curSerial)
{
	int curIndex;
	
	if((curIndex = getSerialIndex(asSerial, curSerial)) < 0)
	{
		printf("... cannot swap serial position up\n");
		return -1;
	}
	
	if(curIndex == 0)
	{
		printf("... cannot swap up, serial number already in first position\n");
		return 0;
	}

	return swap_serialPos(asSerial, curIndex, curIndex-1);
	
}


int swap_down(char** asSerial, char* curSerial)
{
	int curIndex;
	int numbOfDevs = get_number_of_serials(asSerial);
	
	if(curIndex = getSerialIndex(asSerial, curSerial) < 0)
	{
		printf("... cannot swap serial position down\n");
		return -1;
	}
	
	if(curIndex == (numbOfDevs-1))
	{
		printf("... cannot swap down, serial number already in last position\n");
		return 0;
	}
	
	return swap_serialPos(asSerial, curIndex, curIndex+1);
	
}