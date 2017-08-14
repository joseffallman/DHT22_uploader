/*
 Name:    SensorData.h
 Created: 2017-08-13
 Author:  Daniel
*/

#include "SensorData.h"

public class SensorData
{
    // Creates a SensorData object.
    // temperature (float): The temperature read from the sensor.
    // humidity (float): The humidity read from the sensor.
    SensorData::SensorData(float temperature, float humidity)
    {
        _temperature = temperature;
        _humidity = humidity;
    }

    // Gets the temperature read from the sensor.
    float SensorData::GetTemperature()
    {
        return _temperature;
    }

    // Gets the humidity read from the sensor.
    float SensorData::GetHumidity()
    {
        return _humidity;
    }
}
