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
#define API_KEY "Your IFTTT API KEY"
#define SERVER "maker.ifttt.com"
#define EVENT "The name of the IFTTT event"

using namespace std;

WiFiClient client;
uint32_t startTime = 0;
uint32_t prevStartTime = 0;
uint32_t currentTime;
uint8_t clickType = 0;

String iftttTrigger(String message)
{
    String name = "";
    client.stop();
    if (client.connect(SERVER, 80))
    {
	String postData = "{\"value1\": \"" + message + "\"}";
	Serial.println("Connected to server... Getting name");
	String request = "POST /trigger/";   //send HTTP PUT request
	request += EVENT;
	request += "/with/key/";
	request += API_KEY;
	request += " HTTP/1.1";

	Serial.println(request);    
	client.println(request);
	client.println("Host: maker.ifttt.com");
	client.println("User-Agent: Energia/1.1");
	client.println("Connection: close");
	client.println("Content-Type: application/json");
	client.print("Content-Length: ");
	client.println(postData.length());
	client.println();
	client.println(postData);
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
    WiFi.mode(WIFI_STA);
    WiFiManager wifiManager;
    wifiManager.autoConnect();
    Serial.println("Push the Button!");
    pinMode(0, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(0), readPin, CHANGE);
}

void loop() {
    // put your main code here, to run repeatedly:
    currentTime = millis();
    // Check if the button is up and it has been pressed more than 500ms ago
    if (clickType == DOUBLE_CLICK)
    {
	iftttTrigger("You double clicked the button!");
	Serial.println("Double press");
	clickType = 0;
    }
    else if (clickType == SINGLE_CLICK && currentTime - startTime > CLICK_DELAY)
    {
	iftttTrigger("You single clicked the button!");
	Serial.println("Single press");
	clickType = 0;
    }
    else if (clickType == HOLD_CLICK && currentTime - startTime > HOLD_LENGTH)
    {
	iftttTrigger("You pressed and held the button!");
	Serial.println("Button Hold");
	clickType = 0;
    }
}
