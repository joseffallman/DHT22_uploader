/*
Name:    Server.h
Created: 2017-08-14
Author:  Josef
*/

#include "Server.h"

UploadServer::UploadServer() {
};

UploadServer::UploadServer(String url) {
	_url = url;
};

void UploadServer::AddUrl(String url) {
	_url = url;
};

bool UploadServer::Connect(String adress) {

}
bool UploadServer::SendReading(SensorData reading) {
	return true;
};