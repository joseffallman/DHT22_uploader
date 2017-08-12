/*
 Name:    Device.h
 Created: 2017-08-12
 Author:  Daniel
*/

#include "Sensor.h"

public class Device
{
    public:
        // Creates a Device object.
        // id (int): The ID of the current device.
        // updateIntervalSec (int): The update interval in seconds.
        Device(int id, int updateIntervalSec);

        // Frees up resources used by the Device.
        ~Device();

        // Reads current data (temperature and humidity) and sends it to the server.
        bool MeasureAndSendData();

        // Puts the device to deep sleep.
        void Sleep();

    private:
        /* Variables */

        // The update interval in seconds.
        int _updateIntervalSec;
        Sensor* _sensor;

        /* Methods */

        // Gets whether stored data exists.
        bool HasStoredData();

        // Sends stored data to the server.
        // Returns (bool): true if there are no more stored data to send to the server. Otherwise false.
        bool SendStoredData();

        // Gets the battery status in percent.
        // out volt (float): Current battery current.
        // out percent (int): Current calculated battery status in percent.
        // Returns (bool): true if the battery status could be read, otherwise false.
        bool GetBatteryStatus(float& volt, int& percent);

        // Gets the current time as presumed by the device. May differ aggressively from real time.
        String GetCurrentBoardTime();
}

/* USE LIKE THIS

void setup()
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
