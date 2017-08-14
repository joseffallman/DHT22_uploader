/*
 Name:    Sensor.h
 Created: 2017-08-12
 Author:  Daniel
*/

#include "SensorData.h"

public class Sensor
{
    public:
        Sensor();

        // Gets the temperature and humidity measured by the sensor.
        SensorData Read();
}
