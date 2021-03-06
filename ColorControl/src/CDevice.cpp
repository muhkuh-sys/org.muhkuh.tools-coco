#include "CDevice.h"

CColorController::CColorController()
{
    m_isConnected = false;

}

CColorController::~CColorController()
{

}


void CColorController::SetWavelength(int iSensorIndex, int iWavelength)
{
	m_sensorData[iSensorIndex].SetWavelength(iWavelength);
}

void CColorController::SetSaturation(int iSensorIndex, int iSaturation)
{
	m_sensorData[iSensorIndex].SetSaturation(iSaturation);
}

void CColorController::SetIllumination(int iSensorIndex, int iIllumination)
{
	m_sensorData[iSensorIndex].SetIllumination(iIllumination);
}

void CColorController::SetColour(int iSensorIndex, int r, int g, int b)
{
    m_sensorData[iSensorIndex].SetColor(r, g, b);
}

void CColorController::SetGain(int iSensorIndex, tcs3472_gain_t tGain)
{
	m_sensorData[iSensorIndex].SetGain(tGain);
}

void CColorController::SetIntTime(int iSensorIndex, tcs3472_intTime_t tIntTime)
{
	m_sensorData[iSensorIndex].SetIntTime(tIntTime);
}

void CColorController::SetClearRatio(int iSensorIndex, int iClearRatio)
{
    m_sensorData[iSensorIndex].SetClearRatio(iClearRatio);
}

void CColorController::SetTolNm(int tolNm)
{
    if(tolNm > 0)
    {
        m_tolnm = tolNm;
    }

    else m_tolnm = 10; // Default wavelength tolerance
}

void CColorController::SetTolSat(int tolSat)
{
    if(tolSat > 0)
    {
        m_tolsat = tolSat;
    }

    else m_tolsat = 10; // Default saturation tolerance
}

void CColorController::SetTolIllu(int tolIllu)
{
    if(tolIllu > 0)
    {
        m_tolillu = tolIllu;
    }

    else m_tolillu = 50;
}

void CColorController::SetState(int iErrorCode)
{
    /* Everything went well */
    if( iErrorCode == SENSOR_OK)
    {
        /* Validate the Errorcode and set the sensor states accordingly */
        for(int i = 0; i < 16; i++)
        {
            m_sensorData[i].SetState(SENSOR_OK);
        }

    }

    /* An Id Error occured */
    else if( iErrorCode & SENSOR_ID_ERROR )
    {
        /* Validate the Errorcode and set the sensor states accordingly */
        for(int i = 0; i < 16; i++)
        {
            if(iErrorCode & (1<<i))
            {
                m_sensorData[i].SetState(SENSOR_ID_ERROR);
            }
            else m_sensorData[i].SetState(SENSOR_OK);
        }
    }

    /* The conversions were not complete when the registers were read */
    else if( iErrorCode & SENSOR_INCOMPLETE_CONVERSION)
    {
        /* Validate the Errorcode and set the sensor states accordingly */
        for ( int i = 0; i < 16; i++ )
        {
            if(iErrorCode & (1<<i))
            {
                m_sensorData[i].SetState(SENSOR_INCOMPLETE_CONVERSION);
            }
            else m_sensorData[i].SetState(SENSOR_OK);
        }
    }

    /* Maximum clear levels got exceed, thus the readings are not valid */
    else if( iErrorCode & SENSOR_EXCEEDED_CLEAR )
    {
        /* Validate the Errorcode and set the sensor states accordingly */
        for( int i = 0; i < 16; i++ )
        {
            if(iErrorCode & (1<<i))
            {
                m_sensorData[i].SetState(SENSOR_EXCEEDED_CLEAR);
            }
            else m_sensorData[i].SetState(SENSOR_OK);
        }
    }

    /* Fatal device error , reading / writing to a ftdi channel failed */
    else if (iErrorCode == DEVICE_ERROR_FATAL )
    {
        /* Validate the Errorcode and set the sensor states accordingly */
        for(int i = 0; i < 16; i++)
        {
            m_sensorData[i].SetState(DEVICE_ERROR_FATAL);
        }
    }

    /* an usb error occoured - we read a different number of bytes than we expected to read */
    else if (iErrorCode == USB_ERROR )
    {
        /* Validate the Errorcode and set the sensor states accordingly */
        for(int i = 0; i < 16; i++)
        {
            m_sensorData[i].SetState(USB_ERROR);
        }
    }

}
