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
        Device(int id, int updateIntervalSec, Server& server);

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
        bool HasStoredReadings();

        // Sends stored readings to the server.
        // Returns (bool): true if there are no more stored data to send to the server. Otherwise false.
        bool SendStoredReadings();

        // Stores a reading in the filesystem.
        // reading (SensorData): The reading to store.
        void StoreReading(SensorData reading);

        // Gets the battery status in percent.
        // out volt (float): Current battery current.
        // out percent (int): Current calculated battery status in percent.
        // Returns (bool): true if the battery status could be read, otherwise false.
        bool GetBatteryStatus(float& volt, int& percent);

        // Gets the current time as presumed by the device. May differ aggressively from real time.
        String GetCurrentBoardTime();

        // Sets the current time. This method should be called every time the exact time is known.
        String Device::SetCurrentBoardTime(String currentTime)
}

/* USE LIKE THIS

void setup()
{
    Server server(url);
    Device device(id, 3600, server);

    device.MeasureAndSendData();
    device.Sleep();
}

 */
