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
/** \file led_analyzer.c

	 \brief led_analyzer handles all functionality on device level and provides the functions needed by the GUI application.
	 
The led_analyzer library will handle functionality to work with severeal color controller devices. It scans
for connected color controller devices and opens them. All devices that have been connected to will have handles. 
These handles will be stored in an array and can be used by all underlying color related functions. Furthermore this library
provides the functions which are required for the application CoCo App.
 
 */
 
#include "led_analyzer.h"

/** Vendor ID for Hilscher Gesellschaft für Systemautomation mbH */
#define VID 0x1939
/** Product ID for the Color Controller "COLOR-CTRL" */
#define PID 0x0024

/** \brief scans for connected color controller devices and stores their serial numbers in an array.

Functions scans for all color controller devices that are connected via USB. A device which has "COLOR-CTRL" as description
will be counted as a color controller. Function prints manufacturer, description and serialnumber.
Furthermore the serialnumber(s) will be stored in an array and can be used for later functions
that open a connected device by a serialnumber. 
	@param asSerial stores the serial numbers of all connected color controller devices
	@param asLength	number of elements the serial number array can contain 
	
	@return 0  : no color controller device detected
	@return <0 : error with ftdi functions or insufficient length of asSerial
	@return >0 : number of connected color controller devices
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

/** \brief connects to all USB devices with a given serial number.

Function opens all USB devices which have a serial number that equals one of the serial numbers given in asSerial.
Furthermore it initializes the devices by configuring the right channels with the right modes. After having successfully opened
a device, the handle of the opened device will be stored in apHandles. As each (ftdi2232h) color controller device has 2 channels
each connected color controller will get 2 handles in apHandles.
	@param apHandles 	stores the handles of all opened USB color controller devices
	@param apHlength	maximum number of handles apHandles can store
	@param asSerial 	stores the serial numbers of all connected color controller devices
	
	@return 0  : opened no color controller device
	@return <0 : error with ftdi functions or insufficient length of apHandles
	@return >0 : number of opened color controller devices
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
	
	return devCounter;
	
	
}

/** \brief returns number of serial numbers from the serial number array.
	@param 	asSerial array that stores the serial numbers
	
	@return number elements in the serial number array
*/
int get_number_of_serials(char** asSerial)
{
	int counter = 0;

	while(asSerial[counter]!=NULL) counter ++;
	
	return counter;
}


/** \brief returns number of handles that are stored in the handle array.
	@param apHandles array that stores the handles
	
	@return number elements in the handle array
*/

int get_number_of_handles(void ** apHandles)
{
	int counter = 0;
	
	while(apHandles[counter] != NULL) counter++;
		
	return counter;
}


/** \brief returns the device number corresponding to a certain handleIndex.

As each device has 2 handles, the device index and the handle index are not the same. Following functions
provides an easy way to get the device number if a handle index is given 
	@param handle	index of the handle
	
	@return 		device index 
*/
int handleToDevice(int handle)
{	
	return (int)(handle/2);
}


/** \brief sets the integration time of one sensor.

Function sets the integration time of one sensor. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a longer integration time, the integration time for bright LEDs can be low. Refer to 
the sensor's datasheet for calculating the content of the integration time register. A few common values have already
been calculated and saved in enum tcs3472Integration_t.
	@param apHandles	 		array that stores ftdi2232h handles
	@param devIndex				device index of current color controller device 
	@param integrationtime		integration time to be sent to the sensor
	@param uiX					sensor which will get the new integration time ( 0 ... 15 )
	
	@return  0 : everything OK - Identification successful
	@return  1 : i2c-functions failed
*/
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

/** \brief sets the gain of one sensor.

Function sets the gain of 1 sensors This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a greater gain factor, gain factor for bright LEDs can be low. Refer to 
the sensor's datasheet for further information about gain.
	@param apHandles	 		array that stores ftdi2232h handles
	@param devIndex				device index of current color controller device 
	@param gain					gain to be sent to the sensor
	@param uiX					sensor which will get the new gain ( 0 ... 15 )
	
	@return  0 : everything OK - Identification successful
	@return  1 : i2c-functions failed
*/
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


/** \brief initializes the sensors of a color controller device.

Function initializes the 16 sensors of a color controller device. Initializing includes turning the sensors on
clearing their interrupt flags and identifying them to be sure that the i2c-protocol works and following color readings
are valid.
	@param apHandles	 		array that stores ftdi2232h handles
	@param devIndex				device index of current color controller device
	
	@return 					0  : everything ok
	@return 					-1 : i2c-functions failed
	@return 					>0 : one or more sensor(s) failed
	@return	    				if the return code is 0b101100, identification failed with sensor 3, sensor 4 and sensor 6

*/
int init_sensors(void** apHandles, int devIndex)
{
	/* 2 handles per device, device 0 has handles 0,1 .. device 1 has handles 2,3 and so on*/
	int handleIndex = devIndex * 2;
	int errorcode = 0;
	unsigned char aucTempbuffer[16];
	
	int iHandleLength = get_number_of_handles(apHandles);
	
	
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



/** \brief reads the RGBC colors of all sensors under a device and checks if the colors are valid

Function read the colors red, green, blue and clear of all 16 sensors under a device and stores them in adequate buffers.
Furthermore the function will check if maximum clear levels have been exceeded. If so, the color reading won't be right and
color readings should be redone. If maximum clear levels are beeing exceeded too often, one should consider turning out
gain and/or integration time settings. The function will return a returncode which can be determined to check
which of the color sensors have exceeded maximum clear levels. Furthermore the function will store the sensors' measured
brightness in an array. This brightness is calculated by the ratio of the current clear level measured by the sensor to
the maximum clear value the sensor can reach. If the sensor has reached maximum clear level this ratio will be 1.0.
	@param apHandles	 		array that stores ftdi2232h handles
	@param devIndex				device index of current color controller device
	@param ausClear				stores all 16 clear colors
	@param ausRed				stores all 16 red colors
	@param ausGreen				stores all 16 green colors
	@param ausBlue				stores all 16 blue colors
	
	@return 					0  : everything ok
	@return 					-1 : i2c-functions failed
	@return 					>0 : one or more sensor(s) failed
	@return	    				if the return code is 0b101100, sensor 3, 4 and 6 failed

*/
int read_colors(void** apHandles, int devIndex, unsigned short* ausClear, unsigned short* ausRed,
				unsigned short* ausGreen, unsigned short* ausBlue)
{
	int iHandleLength = get_number_of_handles(apHandles);
	
	unsigned char aucTempbuffer[16];
	unsigned char aucIntegrationtime[16];
	unsigned char aucGain[16];
	
	float fLUX[16];
	unsigned short CCT[16];
	
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
	
	tcs_readColor(apHandles[handleIndex], apHandles[handleIndex+1], ausClear, CLEAR);
	tcs_readColor(apHandles[handleIndex], apHandles[handleIndex+1], ausRed, RED);
	tcs_readColor(apHandles[handleIndex], apHandles[handleIndex+1], ausGreen, GREEN);
	tcs_readColor(apHandles[handleIndex], apHandles[handleIndex+1], ausBlue, BLUE);

	if((errorcode = tcs_exClear(apHandles[handleIndex], apHandles[handleIndex+1], ausClear, aucIntegrationtime)) != 0)
	{		
		return errorcode;
	}
	
	if((errorcode = tcs_rgbcInvalid(apHandles[handleIndex], apHandles[handleIndex+1])) != 0)
	{
		return errorcode;
	}
	
	tcs_calculate_CCT_Lux(aucGain, aucIntegrationtime, ausClear, ausRed, ausGreen, ausBlue, CCT, fLUX);
	
	return errorcode;
	
}														 

/** \brief checks if clear level has been exceeded and if rgbc datasets are valid.

Function checks if clear levels of 16 sensors under a color controller device have been exceeded and it checks
if the colors are valid by checking certain bits in the sensors' status register. This functioin has been included
in the read color function as well. The returned errorcode can be used to determine which of the sensor(s) failed.
	@param apHandles	 		array that stores ftdi2232h handles
	@param devIndex				device index of current color controller device
	
	@return 					0  : everything ok
	@return 					-1 : i2c-functions failed
	@return 					>0 : one or more sensor(s) failed
	@return	    				if the return code is 0b101100, identification failed with sensor 3, sensor 4 and sensor 6

*/
int check_validity(void** apHandles, int devIndex, unsigned short* ausClear, unsigned char* aucIntegrationtime)
{
	int iHandleLength = get_number_of_handles(apHandles);
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


/** frees the memory of all connected and opened color controller devices.

Function iterates over all handle elements in apHandles and frees the memory. Freeing includes closing
the usb_device and freeing the memory allocated by the device handle. 
	@param apHandles	 		array that stores ftdi2232h handles
*/

int free_devices(void** apHandles)
{
	int iResult = 0;
	int index = 0;
	int iHandleLength = get_number_of_handles(apHandles);
	
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

/** \brief sets the gain of 16 sensors under a device.

Function sets the gain of 16 sensors of a device. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a greater gain factor, gain factor for bright LEDs can be low. Refer to 
the sensor's datasheet for further information about gain.
	@param apHandles	 		array that stores ftdi2232h handles
	@param gain					gain to be sent to the sensors
	
	@return  0 : everything OK - Identification successful
	@return  1 : i2c-functions failed
*/
int set_gain(void** apHandles, int devIndex, unsigned char gain)
{
	int iHandleLength = get_number_of_handles(apHandles);
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

/** \brief reads the current gain setting of 16 sensors under a device and stores them in an adequate buffer.

The function reads back the gain settings of 16 sensors. Refer to sensors' datasheet for further information about
gain settings.
	@param apHandles	 		array that stores ftdi2232h handles
	@param aucGains	pointer to buffer which will contain the gain settings of the 16 sensors
	
	@return 0 :				everything OK
	@return	1 : 			i2c-functions failed
*/
int get_gain(void** apHandles, int devIndex, unsigned char* aucGains)
{
	
	int iHandleLength = get_number_of_handles(apHandles);
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

/** \brief sets the integration time of 16 sensors under a device at once.

Function sets the integration time of 16 sensors. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a longer integration time, the integration time for bright LEDs can be low. Refer to 
the sensor's datasheet for calculating the content of the integration time register. A few common values have already
been calculated and saved in enum tcs3472Integration_t.
	@param apHandles	 		array that stores ftdi2232h handles
	@param integrationtime		integration time to be sent to the sensors
	
	@return  0 : everything OK - Identification successful
	@return  1 : i2c-functions failed
	*/

int set_intTime(void** apHandles, int devIndex, unsigned char integrationtime)
{
	int iHandleLength = get_number_of_handles(apHandles);
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


/** \brief reads the integration time setting of 16 sensors under a device and stores them in an adequate buffer.

The function reads back the integration time of 16 sensors. Refer to sensors' datasheet for further information about
integration time settings.
	@param apHandles	 		array that stores ftdi2232h handles
	@param aucIntegrationtime	pointer to buffer which will store the integration time settings of the 16 sensors
	
	@return 0 :					everything OK
	@return	1 : 				i2c-functions failed
*/

int get_intTime(void** apHandles, int devIndex, unsigned char* aucIntegrationtime)
{
	int iHandleLength = get_number_of_handles(apHandles);
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

/** waits for a time specified in uiWaitTime (ms)
	@param uiWaitTime	time to wait in order to let the sensors complete their ADC measurements
*/

void wait4Conversion(unsigned int uiWaitTime)
{

	if((uiWaitTime > 0) && (uiWaitTime <= 700))
		Sleep(uiWaitTime);
	
	else Sleep(200);
}


/** swapts the position of two serial numbers located in an array of serial numbers.

Function swaps the location of two serial numbers which are located in an array of serial numbers. This function
will be used for having control over the opening order of usb devices, as the current algorithm iterates over
the serial number array and opens the devices with those serial numbers. Thus a color controller device with a serial
number  at location 0 will be opened before a device with a serial number located at index 1. The handles which correspond
to the opened color controller devices will be stored in the same order, as the order of opening. If the function
has finished successfully the serial number which was in pos1 will be in position 2 and
	@param asSerial	array which contains serial numbers of color controller devices
	@param pos1		current position of one swap operand
	@param pos2		target position of the swap operand
	
	@return -1 :	reaching out of the serial number array
	@return  0 :	OK - swapping successful
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

/** \brief returns the index of the serial number described by curSerial.

Function returns the serial number that curSerial currently has in the serial number array asSerial.
	@param asSerial	 array which contains serial numbers of color controller devices
	@param curSerial current serial number
	
	@return 		 index / position of current serial number in asSerial
*/
int getSerialIndex(char** asSerial, char* curSerial)
{
	int numbOfDevs = get_number_of_serials(asSerial);
	int i;
	
	
	for(i = 0; i < numbOfDevs; i++)
	{
		if(strcmp(curSerial, asSerial[i]) == 0) return i;
	}
	
	printf("... serial not found - cannot return an index\n");
	return -1;

}


/** \brief swaps the serial number described by curSerial up by one position.

Function swaps the serial number described by curSerial up by one position. The serial number
that got pushed away will be in [oldPosition + 1].

*/
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

/** \brief swaps the serial number described by curSerial down by one position.

Function swaps the serial number described by curSerial up by one position. The serial number
that got pushed away will be in [oldPosition + 1].

*/
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