/*
 Name:    Sensor.h
 Created: 2017-08-12
 Author:  Daniel
*/

#include "SensorData.h"

class Sensor
{
	public:
		// Define to what pin (4) your sensor is connected and what type of DHT sensor it is (22)
        Sensor(int pin, int type);

        // Gets the temperature and humidity measured by the sensor.
        SensorData* Read();
	private:
		int _pin;
		int _type;
};
