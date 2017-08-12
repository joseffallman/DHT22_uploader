/*
 Name:    Device.h
 Created: 2017-08-12
 Author:  Daniel
*/

#include "Sensor.h"

public class Device
{
    public:
        Device(int id);

        bool HasStoredData();

        // Sends stored data to the server.
        bool SendStoredData();

        // Reads current data (temperature and humidity) and sends it to the server.
        bool MeasureAndSendData();

    private:
        // Gets the battery status in percent.
        // out volt (float): Current battery current.
        // out percent (int): Current calculated battery status in percent.
        // Returns (bool): true if the battery status could be read, otherwise false.
        bool GetBatteryStatus(float& volt, int& percent);

        // Gets the current time as presumed by the device.
        String GetCurrentTime();
}

/* USE LIKE THIS

void loop()
{
    theDevice = new Device(id);

    bool performMeasurement = true;

    if (theDevice.HasStoredData()) {
        performMeasurement = theDevice.SendStoredData();
    };

    if (performMeasurement) {
        theDevice.MeasureAndSendData();
    }
}

 */
