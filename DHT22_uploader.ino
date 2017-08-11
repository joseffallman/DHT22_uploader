/*
 Name:		DHT22_uploader.ino
 Created:	8/4/2017 1:05:17 PM
 Author:	Josef
*/
#include <FS.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
//#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include "WiFiManager-NEW.h"
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <Ticker.h>
Ticker ticker;
#include <DHT.h>
#define DHTPIN 4     // what digital pin we're connected to
#define DHTTYPE DHT22
#include <TimeLib.h> 
//#include <WiFiUdp.h>  // This is only for ntp-time
#include <string.h>


float temperature = 0.0;
float humidity = 0.0;
int battery;
float level_voltage = 0.0;
unsigned int offset_sec = 0;
bool success_save = false;
bool enter_setup = false;
bool shouldSaveConfig = false;
int httpPort = 80; //to your server
//IPAddress timeServer(213, 21, 116, 142); // 0.se.pool.ntp.org
//const int timeZone = 1;     // Central European Time
//WiFiUDP Udp;
//unsigned int localPort = 8888;  // local port to listen for UDP packets
//const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
//byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets


//define your default values here, if there are different values in config.json, they are overwritten.
char url_server[50] = "www.mywebserver.com";
char url_page[50] = "/upload/upload.php";
char sensor_name[40] = "My Cool Thing";
char sleeptime[8] = "3600";
char set_Batt_Max[5] = "5.29";
char set_Batt_Min[5] = "4.4";
//char blynk_token[34] = "YOUR_BLYNK_TOKEN";

// ------------------------------------------------
// ---------- Functions declaration ---------------
// ------------------------------------------------
String printDigits(int digits);
String tid(unsigned int unix_timestamp);
void tick();
void saveConfigCallback();
void configModeCallback(WiFiManager *myWiFiManager);
String URLEncode(const char* msg);
boolean read_dht();
int battery_level();
void trigger_pressed();
//boolean check_sleep();
//void sendNTPpacket(IPAddress &address);
//time_t getNtpTime();
int adjustDstEurope(time_t f);
time_t FromDateHeader(WiFiClient client);
boolean time_from_file();
void WiFiMangager();
unsigned int calc_wakeup();
void deep_sleep(unsigned int sleep = 4294); //sleeps in sec, max = 4294
void turn_off();
void save_url(String url);
void upload_url_to_page(WiFiClient client, String url, boolean set_clock = false);


// ------------------------------------------------
// ------------------------------------------------
// ------------------------------------------------

// the setup function runs once when you press reset or power the board
void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
	Serial.println();


	//Starts wifi as client and try to connect to last ap.
	WiFi.mode(WIFI_STA);
	WiFi.begin();

	//clean FS, for testing
	//SPIFFS.format();

	//read configuration from FS json
	Serial.println("mounting FS...");

	if (SPIFFS.begin()) {
		Serial.println("mounted file system");
		if (SPIFFS.exists("/config.json")) {
			//file exists, reading and loading
			Serial.println("reading config file");
			File configFile = SPIFFS.open("/config.json", "r");
			if (configFile) {
				Serial.println("opened config file");
				size_t size = configFile.size();
				// Allocate a buffer to store contents of the file.
				std::unique_ptr<char[]> buf(new char[size]);

				configFile.readBytes(buf.get(), size);
				DynamicJsonBuffer jsonBuffer;
				JsonObject& json = jsonBuffer.parseObject(buf.get());
				json.printTo(Serial);
				if (json.success()) {
					Serial.println("\nparsed json");

					strcpy(url_server, json["url_server"]);
					strcpy(url_page, json["url_page"]);
					strcpy(sensor_name, json["sensor_name"]);
					strcpy(sleeptime, json["sleeptime"]);
					strcpy(set_Batt_Max, json["set_Batt_Max"]);
					strcpy(set_Batt_Min, json["set_Batt_Min"]);
					success_save = true;

				}
				else {
					Serial.println("failed to load json config, will enter setup");
					enter_setup = true;
				}
			}
		}
		else {
			Serial.println("No config file, will enter setup");
			enter_setup = true;
		}
	}
	else {
		Serial.println("Failed to mount FS, restart");
		ESP.reset();
		delay(5000);
	}
  //--------------------------------------------------------------------------
  //                      end configfile readings
  //--------------------------------------------------------------------------

  // Get battery status and if we're low, turn off device
  battery = battery_level();
  Serial.println("Battery is: ");
  if ( battery <= 0 ) {
    Serial.println("ESP8266 shutdown");
    turn_off();
  }

  
  //--------------------------------------------------------------------------
  //                      Check for setup-portal
  //--------------------------------------------------------------------------
	if (WiFi.SSID() == "") {
		enter_setup = true;
	}

	//Start blue-led to tell thats its time to trigger the setup-portal
	if (!enter_setup) {
		Serial.println("Would you like to enter setup-portal?");
		pinMode(0, INPUT);
		pinMode(2, OUTPUT);
		digitalWrite(2, LOW);
		attachInterrupt(0, trigger_pressed, FALLING);
		delay(2000);
		detachInterrupt(0);
		digitalWrite(2, HIGH);
	}

	// is configuration portal requested?
	if (digitalRead(0) == LOW || enter_setup == true) {
		Serial.println("Setup-portal initialize");
		WiFiMangager();
		// digitalWrite(0, HIGH);
		// delay(1000);
		// Serial.println("Restarts");
		// ESP.reset();
		// delay(5000);
	}
	else {
		Serial.println("Setup-portal not trigged");
	}
  //--------------------------------------------------------------------------
  //                      All settings is now loaded and ready
  //--------------------------------------------------------------------------

  unsigned int sleep = atoi(sleeptime);
  if (strchr(url_server, ':') != NULL) {
    Serial.println("Will split ip and port to different variables");
    char *splited_url = strtok(url_server, ":");
    Serial.print("URL: ");Serial.print(splited_url);
    char *splited_port = strtok(NULL, ":");
    Serial.print(" Port: ");Serial.println(splited_port);
    
    strcpy(url_server, splited_url);
    httpPort = atoi(splited_port);
    
  };


  //--------------------------------------------------------------------------
  //                      If we are connected to wifi or not
  //--------------------------------------------------------------------------
	int wifi_x = 0;
	while (WiFi.status() != WL_CONNECTED && wifi_x < 10) {
		delay(500);
		wifi_x += 1;
		Serial.print(".");
	}
	Serial.println();
	if (WiFi.status() != WL_CONNECTED && wifi_x == 10) {
		Serial.println("Your not connected, restart");
		//Make sure that pin 0 is HIGH before restarts/sleep
		digitalWrite(0, HIGH);
		delay(1000);
		//ESP.reset();
		//delay(5000);
		//deep_sleep(60);

    if (read_dht() && success_save == true) {
      // Set time from file
      time_from_file();
     
      // Create the URL
      String the_time = tid(now());
      the_time.replace(" ", "%20");
      String url = String(String(url_page) + "?sensor=" + URLEncode(sensor_name) + "&tid=" + String(the_time) + "&temperature=" + String(temperature) + "&humidity=" + String(humidity) + "&batt=" + String(battery) + "&batt_volt=" + String(level_voltage) /* + "&sleeptime=" + String(sleeptime) */  + "&connection_status=no_internet" );
      save_url(url);
      sleep = calc_wakeup();
    }
    deep_sleep(sleep);
   
	}
	if (WiFi.status() == WL_CONNECTED) {
		Serial.println();
		Serial.println("WiFi connected");
		Serial.print("IP address: "); Serial.println(WiFi.localIP());
	}
	

	//Serial.println("Starting UDP");
	//Udp.begin(localPort);
	//setSyncProvider(getNtpTime);
	//String the_time = tid(now());
	//Serial.print("The time is now: "); Serial.println(the_time);
	//boolean sleep_diff = check_sleep();


	// If its a successfull read from sensor
  // and all values loaded from config.json
	if (read_dht() && success_save == true) {

		

		// ---------------------
		// -- Upload code ------
		// ---------------------



		Serial.print("Connecting to: "); Serial.println(url_server); 
		Serial.print("Port: "); Serial.println(httpPort);

		// Use WiFiClient class to create TCP connections
		WiFiClient client;
		if (!client.connect(url_server, httpPort)) {
			Serial.print("connection to your server failed. ");
     
      if (client.connect("www.google.com", 80)) {
				Serial.println("connected to google");
				client.print(String("GET ") + "index.html HTTP/1.1\r\n" +
					"Host: www.google.com\r\n" +
					"Connection: close\r\n\r\n");
				
        // Sets the clock
        int timeout = 5 * 10; // 5 seconds             
        while (!client.available() && (timeout-- > 0)) {
          delay(100);
        }
        time_t header_time = FromDateHeader(client);
        setTime(header_time);
        Serial.print("Local Time: "); Serial.println(tid(now()));

        // Create the URL
        String the_time = tid(now());
        the_time.replace(" ", "%20");
        String url = String(String(url_page) + "?sensor=" + URLEncode(sensor_name) + "&tid=" + String(the_time) + "&temperature=" + String(temperature) + "&humidity=" + String(humidity) + "&batt=" + String(battery) + "&batt_volt=" + String(level_voltage) /* + "&sleeptime=" + String(sleeptime) */  + "&connection_status=no_server" );
        save_url(url);
			}
			else {
				Serial.println("Google failed. No internet connection!");
        // Set time from file
        time_from_file();
       
        // Create the URL
        String the_time = tid(now());
        the_time.replace(" ", "%20");
        String url = String(String(url_page) + "?sensor=" + URLEncode(sensor_name) + "&tid=" + String(the_time) + "&temperature=" + String(temperature) + "&humidity=" + String(humidity) + "&batt=" + String(battery) + "&batt_volt=" + String(level_voltage) /* + "&sleeptime=" + String(sleeptime) */  + "&connection_status=no_internet" );
        save_url(url);
        sleep = calc_wakeup();
				deep_sleep(sleep);
				
			}
		}
		else {
      Serial.print("successfully connected to your server.");
      File f;
  
      if (SPIFFS.exists("/upload_later.txt")) {
        boolean set_clock = true;
        // Set time from file
        time_from_file();
       
        // Create the URL
        String the_time = tid(now());
        the_time.replace(" ", "%20");
		
        String file_url; 
        f = SPIFFS.open("/upload_later.txt", "r");
        while (f.available()) {
          //Read line by line from the file
          file_url = f.readStringUntil('\n');
          upload_url_to_page(client, file_url + "&tidnu=" + String(the_time), set_clock);
          set_clock = false;
        }
        f.close();
        SPIFFS.remove("/upload_later.txt");
        
        // Create the URL
        String the_time = tid(now());
        the_time.replace(" ", "%20");
        String url = String(String(url_page) + "?sensor=" + URLEncode(sensor_name) + "&tid=" + String(the_time) + "&temperature=" + String(temperature) + "&humidity=" + String(humidity) + "&batt=" + String(battery) + "&batt_volt=" + String(level_voltage) /* + "&sleeptime=" + String(sleeptime) */  + "&connection_status=internet" );
        upload_url_to_page(client, url);
      }
      else {
        // We now create a URI for the request
        // The clock is still NOT set so we can't use it
        String url = String(String(url_page) + "?sensor=" + URLEncode(sensor_name) + /* "&tid=" + String(the_time) + */ "&temperature=" + String(temperature) + "&humidity=" + String(humidity) + "&batt=" + String(battery) + "&batt_volt=" + String(level_voltage) /* + "&sleeptime=" + String(sleeptime) */  + "&connection_status=internet" );
        upload_url_to_page(client, url, true);
      }
       
  
  		// ---------------------
  		// -- Upload code end --
  		// ---------------------
  
  		sleep = calc_wakeup();
	  }
	}


	// ----------------------
	// -- DeepSleep in us ---
	// ----------------------
  deep_sleep(sleep);




}

void loop() {
	// put your main code here, to run repeatedly:

}

// ------------------------------------------------
// ---------- Functions ---------------------------
// ------------------------------------------------

void tick()
{
	//toggle state
	int state = digitalRead(0);  // get the current state of GPIO1 pin
	digitalWrite(0, !state);     // set pin to the opposite state
}
void trigger_pressed() {
	enter_setup = true;
	Serial.println("Pressed");
}
void saveConfigCallback() {
	Serial.println("Should save config");
	shouldSaveConfig = true;
}
//gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager) {

	Serial.println("Entered config mode");
	Serial.println(WiFi.softAPIP());
	//if you used auto generated SSID, print it
	Serial.println(myWiFiManager->getConfigPortalSSID());

	//set led pin as output
	pinMode(0, OUTPUT);
	//entered config mode, make led toggle
	ticker.attach(0.6, tick);
}
String URLEncode(const char* msg) {
	const char *hex = "0123456789abcdef";
	String encodedMsg = "";

	while (*msg != '\0') {
		if (('a' <= *msg && *msg <= 'z')
			|| ('A' <= *msg && *msg <= 'Z')
			|| ('0' <= *msg && *msg <= '9')) {
			encodedMsg += *msg;
		}
		else {
			encodedMsg += '%';
			encodedMsg += hex[*msg >> 4];
			encodedMsg += hex[*msg & 15];
		}
		msg++;
	}
	return encodedMsg;
}
boolean read_dht() {
	Serial.println("DHT22 readings!");
	// Initialize DHT sensor.
	DHT dht(DHTPIN, DHTTYPE);

	dht.begin();

	// Reading temperature or humidity takes about 250 milliseconds!
	// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
	float h = dht.readHumidity();
	// Read temperature as Celsius (the default)
	float t = dht.readTemperature();

	// Check if any reads failed and exit early (to try again).
	if (isnan(h) || isnan(t)) {
		Serial.println("Failed to read from DHT sensor!");
		return false;
	}

	// Compute heat index in Celsius (isFahreheit = false)
	float hic = dht.computeHeatIndex(t, h, false);

	Serial.print("Humidity: ");
	Serial.print(h);
	Serial.print(" %\t");
	Serial.print("Temperature: ");
	Serial.print(t);
	Serial.print(" *C ");
	Serial.print("Heat index: ");
	Serial.print(hic);
	Serial.print(" *C \n");

	temperature = t;
	humidity = h;
	return true;
}
int battery_level() {

	// read the battery level from the ESP8266 analog in pin.
	// analog read level is 10 bit 0-1023 (0V-1V).
	// Voltage divider formula is (Uin * R1)/(R2 + R1) = Uout
	// That will be (8.2 * 100 000) / (1 000 000 + 100 000) = 0,745V
	// And that is 0,745 * 1023 = 762,135 = 762max value
	int level = analogRead(A0);
	level_voltage = ((level * (1000000 + 100000)) / 100000);
	level_voltage = level_voltage / 1023;

	// ((MaxSp�nnig / 11) * 1000) * 1.023 = Umax 762
	// ((MinSp�nnig / 11) * 1000) * 1.023 = Umin 595,2
	float BattMax = atof(set_Batt_Max);
	float BattMin = atof(set_Batt_Min);

	int Umax = ((BattMax * 100000) / (1000000 + 100000)) * 1023;
	int Umin = ((BattMin * 100000) / (1000000 + 100000)) * 1023;
	// convert battery level to percent
	level = map(level, Umin, Umax, 0, 100);
	Serial.print("Battery level: "); Serial.print(level_voltage); Serial.print("Volt. That is "); Serial.print(level); Serial.println("%");

	if (level < 0) {
		level = -1;
	}

	return level;

}
/*
boolean check_sleep() {
	String filereads;
	unsigned int this_wakeup_was = now();
	// this opens the file "clock.txt" in read-mode
	File f = SPIFFS.open("/clock.txt", "r");
	if (f) {
		// we could open the file
		while (f.available()) {
			//Lets read line by line from the file
			filereads = f.readStringUntil('\n');
		}
		f.close();

		unsigned int realsleep = now() - (millis() / 1000) - filereads.toInt();

		//if realsleep is less than sleeptime-1%
		if (realsleep < atoi(sleeptime) - (atoi(sleeptime) * 0.01)) {
			Serial.println("Did not sleep long enough");
			//Serial.print("Slept for ");Serial.print(realsleep); Serial.println(" seconds");
			//Serial.print("Should slept in "); Serial.print(sleeptime); Serial.println(" seconds");

			Serial.print("Time in file: "); Serial.println(tid(filereads.toInt()));

			//unsigned int sleep = ((atoi(sleeptime) - realsleep) * 1000000U);
			unsigned int sleep = (filereads.toInt() + atoi(sleeptime)) - now();
			Serial.print("This round took: "); Serial.print(millis()); Serial.println(" milliseconds.");
			Serial.print("Going to sleep for some extra "); Serial.print(sleep); Serial.println(" seconds...ZzzZzz");
			Serial.println();
			ESP.deepSleep(sleep * 1000000U, WAKE_RF_DEFAULT);
			delay(1000);
		}
		else {
			Serial.print("Slept for: "); Serial.print(realsleep); Serial.println(" seconds");
			if (realsleep < atoi(sleeptime) + (atoi(sleeptime) * 0.01)) {
				this_wakeup_was = filereads.toInt() + atoi(sleeptime);
			}

		}
	}
	// -----------------
	// open the file in write mode
	f = SPIFFS.open("/clock.txt", "w");
	if (!f) {
		Serial.println("file creation failed");
	}
	f.println(this_wakeup_was);
	f.close();
	//--------------
	return true;
}
*/
String printDigits(int digits) {
	// utility for digital clock display: prints preceding colon and leading 0
	String x = String(digits);
	if (digits < 10)
		//Serial.print('0');
		x = "0" + String(digits);
	return x;
}
String tid(unsigned int unix_timestamp) {
	time_t f = unix_timestamp;
	String format = String(year(f)) + "-" + String(printDigits(month(f))) + "-" + String(printDigits(day(f))) + " " + String(printDigits(hour(f))) + ":" + String(printDigits(minute(f))) + ":" + String(printDigits(second(f)));
	//Serial.println(format);
	return format;
}
/*
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
// set all bytes in the buffer to 0
memset(packetBuffer, 0, NTP_PACKET_SIZE);
// Initialize values needed to form NTP request
// (see URL above for details on the packets)
packetBuffer[0] = 0b11100011;   // LI, Version, Mode
packetBuffer[1] = 0;     // Stratum, or type of clock
packetBuffer[2] = 6;     // Polling Interval
packetBuffer[3] = 0xEC;  // Peer Clock Precision
// 8 bytes of zero for Root Delay & Root Dispersion
packetBuffer[12]  = 49;
packetBuffer[13]  = 0x4E;
packetBuffer[14]  = 49;
packetBuffer[15]  = 52;
// all NTP fields have been given values, now
// you can send a packet requesting a timestamp:
Udp.beginPacket(address, 123); //NTP requests are to port 123
Udp.write(packetBuffer, NTP_PACKET_SIZE);
Udp.endPacket();
}
time_t getNtpTime() {
while (Udp.parsePacket() > 0) ; // discard any previously received packets
Serial.println("Transmit NTP Request");
sendNTPpacket(timeServer);
uint32_t beginWait = millis();
while (millis() - beginWait < 1500) {
int size = Udp.parsePacket();
if (size >= NTP_PACKET_SIZE) {
Serial.println("Receive NTP Response");
Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
unsigned long secsSince1900;
// convert four bytes starting at location 40 to a long integer
secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
secsSince1900 |= (unsigned long)packetBuffer[43];
return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
}
}
Serial.println("No NTP Response :-(");
return 0; // return 0 if unable to get the time
}
*/
int adjustDstEurope(time_t f) {
	// last sunday of march
	int beginDSTDate = (31 - (5 * year(f) / 4 + 4) % 7);
	//Serial.print("Winter to summer:");Serial.print(beginDSTDate);Serial.println(" march.");
	int beginDSTMonth = 3;
	//last sunday of october
	int endDSTDate = (31 - (5 * year(f) / 4 + 1) % 7);
	//Serial.print("Summer to winter:");Serial.print(endDSTDate);Serial.println(" october.");
	int endDSTMonth = 10;
	// DST is valid as:
	Serial.print("Now its ");
	if (((month(f) > beginDSTMonth) && (month(f) < endDSTMonth))
		|| ((month(f) == beginDSTMonth) && (day(f) >= beginDSTDate))
		|| ((month(f) == endDSTMonth) && (day(f) <= endDSTDate))) {
		Serial.println("summertime.");
		return 7200;  // DST europe = utc +2 hour
	}
	else {
		Serial.println("wintertime.");
		return 3600; // nonDST europe = utc +1 hour
	}
}
time_t FromDateHeader(WiFiClient client) {
  /* 
   *  Finds out string 'Mon, 07 Aug 2017 12:41:21 GMT' in Header response
   */
  
  while (client.available()) {
    if (client.read() == '\n' && client.read() == 'D' && client.read() == 'a' && client.read() == 't' && client.read() == 'e' && client.read() == ':') {
      //String timestring = client.readStringUntil('\n');
      //Serial.println(timestring);
      
      client.readStringUntil(' ');
      int days = client.parseInt();
      client.read();
      String months1 = client.readStringUntil(' ');
      int years = client.parseInt(); 
      int hours = client.parseInt();
      int minutes = client.parseInt();
      int seconds = client.parseInt();
      
      char s_month[5];
      months1.toCharArray(s_month, 5);
      
      static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
      int months = (strstr(month_names, s_month) - month_names) / 3 + 1;
      Serial.print("Current UTC-Date "); Serial.print(years); Serial.print("-"); Serial.print(printDigits(months)); Serial.print("-"); Serial.print(printDigits(days));
      Serial.print(" "); Serial.print(printDigits(hours)); Serial.print(":"); Serial.print(printDigits(minutes)); Serial.print(":"); Serial.println(printDigits(seconds));
      if (days == 0 || months == 0 || years == 0) return 0; // likely something went wrong
      
      TimeElements tm;
      tm.Hour = hours;
      tm.Minute = minutes;
      tm.Second = seconds;
      tm.Day = days;
      tm.Month = months;
      tm.Year = years - 1970;
      time_t header_time = makeTime(tm);
      //setTime(hours, minutes, seconds, days, months, years);
      //adjustTime(adjustDstEurope(header_time));
      header_time = header_time + adjustDstEurope(header_time);
      //setTime(header_time);
      return header_time;
    }
  }
}
boolean time_from_file() {
  File f;
  time_t file_time;

  if (SPIFFS.exists("/clock.txt")) {
    String filereads;
    // this opens the file "clock.txt" in read-mode
    f = SPIFFS.open("/clock.txt", "r");
    while (f.available()) {
      //Lets read line by line from the file
      filereads = f.readStringUntil('\n');
    }
    f.close();
    file_time = filereads.toInt();
    setTime(file_time);
    Serial.print("Estimated Local Time: "); Serial.println(tid(now()));
    return true;
  }
  return false;
}

void WiFiMangager() {

  char *html = "<div><span class='info'>{p}</span><br/><input id='{i}' name='{n}' length={l} placeholder='{p}' value='{v}'></div>";
	WiFiManagerParameter custom_url_server("server", "Upload to url server", url_server, 50, html);
	WiFiManagerParameter custom_url_page("page", "Upload to url page", url_page, 50, html);
	WiFiManagerParameter custom_sensor_name("name", "Give me a name", sensor_name, 40, html);
	WiFiManagerParameter custom_sleeptime("sleeptime", "seconds to sleep", sleeptime, 8, html);
	WiFiManagerParameter custom_set_Batt_Max("set_Batt_Max", "Max Battery voltage", set_Batt_Max, 5, html);
	WiFiManagerParameter custom_set_Batt_Min("set_Batt_Min", "Min Battery voltage", set_Batt_Min, 5, html);

	//WiFiManager
	//Local intialization. Once its business is done, there is no need to keep it around
	WiFiManager wifiManager;

	//set config save notify callback
	wifiManager.setSaveConfigCallback(saveConfigCallback);

	//set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
	wifiManager.setAPCallback(configModeCallback);


	//add all your parameters here
	wifiManager.addParameter(&custom_url_server);
	wifiManager.addParameter(&custom_url_page);
	wifiManager.addParameter(&custom_sensor_name);
	wifiManager.addParameter(&custom_sleeptime);
	wifiManager.addParameter(&custom_set_Batt_Max);
	wifiManager.addParameter(&custom_set_Batt_Min);

	//reset settings - for testing
	//wifiManager.resetSettings();

	//set minimu quality of signal so it ignores AP's under that quality
	//defaults to 8%
	//wifiManager.setMinimumSignalQuality();

	//sets timeout until configuration portal gets turned off
	//useful to make it all retry or go to sleep
	//in seconds
	wifiManager.setTimeout(180);

	// Serial debug mode
	wifiManager.setDebugOutput(false);

	if (!wifiManager.startConfigPortal("SetupGadget")) {
		Serial.println("failed to connect and hit timeout");
		ticker.detach();
		digitalWrite(0, HIGH);
		delay(3000);
		ESP.reset();
		delay(5000);
	}
	//if you get here you have connected to the WiFi
	Serial.println("connected...");
	ticker.detach();
	digitalWrite(0, HIGH);

	//read updated parameters
	strcpy(url_server, custom_url_server.getValue());
	strcpy(url_page, custom_url_page.getValue());
	strcpy(sensor_name, custom_sensor_name.getValue());
	strcpy(sleeptime, custom_sleeptime.getValue());
	strcpy(set_Batt_Max, custom_set_Batt_Max.getValue());
	strcpy(set_Batt_Min, custom_set_Batt_Min.getValue());

	//save the custom parameters to FS
	if (shouldSaveConfig) {
		Serial.println("saving config");
		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.createObject();
		json["url_server"] = url_server;
		json["url_page"] = url_page;
		json["sensor_name"] = sensor_name;
		json["sleeptime"] = sleeptime;
		json["set_Batt_Max"] = set_Batt_Max;
		json["set_Batt_Min"] = set_Batt_Min;
		success_save = true;

		File configFile = SPIFFS.open("/config.json", "w");
		if (!configFile) {
			Serial.println("Failed to open config file for writing");
		}

		json.printTo(Serial);
		json.printTo(configFile);
		configFile.close();
		Serial.println();

		//Remove old clock-file
		if (SPIFFS.exists("/clock.txt")) {
			Serial.println("Remove old clockfile");
			SPIFFS.remove("/clock.txt");
		}
	}

	/*
	// Use WiFiClient class to create TCP connections
	WiFiClient client;
	const int httpPort = 80;
	if (!client.connect(url_server, httpPort)) {
	Serial.println("connection failed");
	Serial.println("Restart");
	ESP.reset();
	delay(5000);
	}

	// Get and set the clock from url-page header.
	// This will send the request to the server
	client.print(String("GET ") + String(url_page) + " HTTP/1.1\r\n" +
	"Host: " + String(url_server) + "\r\n" +
	"Connection: close\r\n\r\n");
	int timeout = 5 * 10; // 5 seconds
	while(!!!client.available() && (timeout-- > 0)){
	delay(100);
	}
	time_t header_time = FromDateHeader(client);
	setTime(header_time);
	*/

}
unsigned int calc_wakeup() {
	unsigned int next_wakeup = 0;
	time_t this_time = now(); // Returns the current time as seconds since Jan 1 1970
	unsigned int this_sleeptime = atoi(sleeptime);
	File f;

	if (SPIFFS.exists("/clock.txt")) {
		String filereads;
		int i = 1;
		// this opens the file "clock.txt" in read-mode
		f = SPIFFS.open("/clock.txt", "r");
		while (f.available()) {
			//Lets read line by line from the file
			filereads = f.readStringUntil('\n');
		}
		f.close();
		next_wakeup = filereads.toInt();
		Serial.print("This time: "); Serial.print(this_time); Serial.print(" and time in file: "); Serial.print(next_wakeup); Serial.print(" = "); Serial.println(tid(next_wakeup));
    
		if ((this_time + (this_sleeptime * 24)) > next_wakeup) {
			while ((this_time + this_sleeptime / 2) > next_wakeup) {
				next_wakeup = next_wakeup + this_sleeptime;
			}
		}
		else {
			next_wakeup = this_time + this_sleeptime;
		}
   
	}
	else {
		next_wakeup = this_time + this_sleeptime;
	}

	// open the file in write mode
	f = SPIFFS.open("/clock.txt", "w");
	if (!f) {
		Serial.println("file creation failed");
		return 0;
	}
	f.println(next_wakeup);
	f.close();

	return next_wakeup - this_time;
}

void deep_sleep(unsigned int sleep) {
  //sleeps in sec, max = 4294
  Serial.print("This round took: "); Serial.print(millis()); Serial.println(" milliseconds.");

  Serial.print("Going to long deep sleep for "); Serial.print(sleep); Serial.println(" seconds...ZzzZzz");
  Serial.println();
  ESP.deepSleep(sleep * 1000000U, WAKE_RF_DEFAULT);
  delay(1000);
}

void turn_off() {
  ESP.deepSleep(0);
}

void save_url(String url) {
  /* saves the url to a file for later upload */
  File f;

  if (SPIFFS.exists("/upload_later.txt")) {
      f = SPIFFS.open("/upload_later.txt", "a+");
  }
  else {
    f = SPIFFS.open("/upload_later.txt", "w+");
  }
  if (!f) {
    Serial.println("file creation failed");
    return;
  }

  
  f.println(url);
  f.close();
}

void upload_url_to_page(WiFiClient client, String url, boolean set_clock) {

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + String(url_server) + "\r\n" +
    "Connection: close\r\n\r\n");



  int timeout = 5 * 10; // 5 seconds             
  while (!client.available() && (timeout-- > 0)) {
    delay(100);
  }

  if (set_clock) {
    time_t header_time = FromDateHeader(client);
    setTime(header_time);
    Serial.print("Local Time: "); Serial.println(tid(now()));
  }

  // Read all the lines of the reply from server
  while (client.available()) {
    String line = client.readStringUntil('\r');
    // Searching html to find <b>Klar</b>
    if (line.length() > 5) {
      int klar = line.indexOf("<b>Klar</b>");
      if (klar != -1) {
        Serial.println("Successful to upload data ");
      }
    }
  }
  Serial.println();
}

