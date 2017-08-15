/*
 Name:    Server.h
 Created: 2017-08-12
 Author:  Daniel
*/

#include <WString.h>
#include "SensorData.h"

class UploadServer
{
    public:
		UploadServer();
        UploadServer(String url);

		void AddUrl(String url);
		bool Connect(String adress);
        bool SendReading(SensorData reading);

    private:
        String _url;
};
