/*
 Name:    SensorData.h
 Created: 2017-08-13
 Author:  Daniel
*/

// Class that models data read from a sensor.
public class SensorData
{
    public:
        // Creates a SensorData object.
        // temperature (float): The temperature read from the sensor.
        // humidity (float): The humidity read from the sensor.
        SensorData(float temperature, float humidity);

        // Gets the temperature read from the sensor.
        float GetTemperature();

        // Gets the humidity read from the sensor.
        float GetHumidity();

    private:
        float _temperature;
        float _humidity;
}
