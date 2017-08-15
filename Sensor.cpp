/*
Name:    Sensor.h
Created: 2017-08-15
Author:  Josef
*/

#include "Sensor.h"
#include <DHT.h>

Sensor::Sensor(int pin, int type) {
	_pin = pin;
	_type = type;
};

// Gets the temperature and humidity measured by the sensor.
// Returns (SensorData)
SensorData* Sensor::Read() {
	Serial.println(F("DHT22 readings!"));
	// Initialize DHT sensor.
	DHT dht(_pin, _type);

	dht.begin();

	// Reading temperature or humidity takes about 250 milliseconds!
	// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
	float h = dht.readHumidity();
	// Read temperature as Celsius (the default)
	float t = dht.readTemperature();

	// Check if any reads failed and exit early (to try again).
	if (isnan(h) || isnan(t)) {
		Serial.println(F("Failed to read from DHT sensor!"));
		return NULL;
	}

	// Compute heat index in Celsius (isFahreheit = false)
	float hic = dht.computeHeatIndex(t, h, false);

	Serial.print(F("Humidity: "));
	Serial.print(h);
	Serial.print(F(" %\t"));
	Serial.print(F("Temperature: "));
	Serial.print(t);
	Serial.print(F(" *C "));
	Serial.print(F("Heat index: "));
	Serial.print(hic);
	Serial.println(F(" *C"));

	return new SensorData(t, h);
};