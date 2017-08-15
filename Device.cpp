/*
 Name:    Device.h
 Created: 2017-08-12
 Author:  Daniel
*/


#include "Device.h"

// Creates a Device object.
// id (int): The ID of the current device.
// updateIntervalSec (int): The update interval in seconds.
// Sensor (sensor): The sensor of the device
Device::Device(int id, int updateIntervalSec, Sensor *sensor)
{
	_id = id;
	_updateIntervalSec = updateIntervalSec;
	//_pSensor = new Sensor();
	_sensor = sensor;
	_server = new UploadServer();
};

// Frees up resources used by the Device.
Device::~Device()
{
	delete _pSensor;
	delete _server;
}

// Reads current data (temperature and humidity) and sends it to the server.
// If stored data exist, then it will be uploaded before the current measurement.
// If data can be read but not sent (due to network errors etc.), the data will be stored instead.
bool Device::MeasureAndSendData()
{
	bool allStoredReadingsSent = true;

	if (HasStoredReadings()) {
		allStoredReadingsSent = SendStoredReadings();
	}

	if (allStoredReadingsSent) {
		SensorData* reading = _sensor->Read();

		bool success = _server->SendReading(*reading);

		if (!success) {
			StoreReading(*reading);
		}
	}
}

// Puts the device into deep sleep.
void Device::Sleep()
{
}


/* Methods */

// Gets whether stored data exists.
bool Device::HasStoredReadings()
{
}

// Sends stored data to the server.
// Returns (bool): true if there are no more stored data to send to the server. Otherwise false.
bool Device::SendStoredReadings()
{
}

// Stores a reading in the filesystem.
// reading (SensorData): The reading to store.
void Device::StoreReading(SensorData reading)
{
}

// Gets the battery status in percent.
// out volt (float): Current battery current.
// out percent (int): Current calculated battery status in percent.
// Returns (bool): true if the battery status could be read, otherwise false.
bool Device::GetBatteryStatus(float& volt, int& percent)
{
}

// Gets the current time as presumed by the device. May differ aggressively from real time.
String Device::GetCurrentBoardTime()
{
}

// Sets the current time. This method should be called every time the exact time is known.
String Device::SetCurrentBoardTime(String currentTime)
{
}