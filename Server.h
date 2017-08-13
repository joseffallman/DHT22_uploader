/*
 Name:    Server.h
 Created: 2017-08-12
 Author:  Daniel
*/

#include <string.h>
#include "SensorData.h"

public class Server
{
    public:
        Server(String url);

        bool SendReading(SensorData reading);

    private:
        String _url;
}
