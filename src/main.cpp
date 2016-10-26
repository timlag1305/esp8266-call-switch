#include <SPI.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <WiFiManager.h>     
#include <WiFiClient.h>
#include <string>

#define SINGLE_CLICK 1
#define DOUBLE_CLICK 2
#define HOLD_CLICK 3
#define CLICK_DELAY 500
#define HOLD_LENGTH 1000

using namespace std;

char ssid[] = "";
char password[] = "";
char server[] = "maker.ifttt.com";
String IFTTT_KEY  = "";
String ifttt_event = "button_pressed";   //IFTTT maker event name 

WiFiClient client;
int startTime = 0;
int prevStartTime = 0;
int endTime = 0;
int trigger = 0;
uint32_t currentTime;
uint8_t clickType = 0;

String iftttTrigger(String KEY, String EVENT)
{
    String name = "";
    client.stop();
    if (client.connect(server, 80))
    {
	String PostData = "{\"value1\" : \"testValue\", \"value2\" : \"Hello\", \"value3\" : \"World!\" }";
	Serial.println("Connected to server... Getting name");
	String request = "POST /trigger/";   //send HTTP PUT request
	request += EVENT;
	request += "/with/key/";
	request += KEY;
	request += " HTTP/1.1";

	Serial.println(request);    
	client.println(request);
	client.println("Host: maker.ifttt.com");
	client.println("User-Agent: Energia/1.1");
	client.println("Connection: close");
	client.println("Content-Type: application/json");
	client.print("Content-Length: ");
	client.println(PostData.length());
	client.println();
	client.println(PostData);
	client.println();
    }
    else
    {
	Serial.println("Connection Failed");
	return "FAIL";
    }

    long timeOut = 4000; //capture response from the server
    long lastTime = millis();

    while ((millis() - lastTime) < timeOut) //wait for incoming response 
    {
	while (client.available())           //characters incoming from server
	{
	    char c = client.read();          //read characters
	    Serial.write(c);
	}
    }
    Serial.println();
    Serial.println("Request Complete!!");
    //return name received from sever
    return "SUCCESS";

}

void readPin()
{
    // Button up
    if (digitalRead(0) == HIGH)
    {
	if (prevStartTime != 0 && startTime - prevStartTime <= CLICK_DELAY)
	{
	    clickType = DOUBLE_CLICK;
	}
	else if (millis() - startTime <= CLICK_DELAY)
	{
	    clickType = SINGLE_CLICK;
	}
    }
    // Button down
    else
    {
	clickType = HOLD_CLICK;
	prevStartTime = startTime;
	startTime = millis();
    }
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    /*WiFi.mode(WIFI_STA);
    WiFiManager wifiManager;
    WiFiManagerParameter message_one("message_one", "Single click message", "", 100);
    WiFiManagerParameter message_two("message_two", "Double click message", "", 100);

    wifiManager.addParameter(&message_one);
    wifiManager.addParameter(&message_two);

    wifiManager.startConfigPortal("ESP8266WiFi");
    Serial.println("Push the Button!");*/
    pinMode(0, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(0), readPin, CHANGE);
}

void loop() {
    // put your main code here, to run repeatedly:
    currentTime = millis();
    // Check if the button is up and it has been pressed more than 500ms ago
    if (clickType == DOUBLE_CLICK)
    {
	Serial.println("Double press");
	clickType = 0;
    }
    else if (clickType == SINGLE_CLICK && currentTime - startTime > CLICK_DELAY)
    {
	Serial.println("Single press");
	clickType = 0;
    }
    else if (clickType == HOLD_CLICK && currentTime - startTime > HOLD_LENGTH)
    {
	Serial.println("Button Hold");
	clickType = 0;
    }

    /*if (trigger != 0)
    {
        // This might not work
        if (trigger == SINGLE_CLICK)
        {
            delay(500);
        }

        iftttTrigger(IFTTT_KEY, ifttt_event);
        Serial.println("Push The Button");
        trigger = 0;
    }*/
}
