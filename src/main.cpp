#ifndef __CC3200R1M1RGC__
#include <SPI.h>
#endif
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

char ssid[] = "";
char password[] = "";
char server[] = "maker.ifttt.com";

WiFiClient client;
int trigger = 0;

String iftttTrigger(String KEY, String EVENT)
{
    String name = "";
    client.stop();
    if (client.connect(server,80))
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

void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println("dBm");
}

void sendRequest(){
    if (trigger == 0){
	trigger = 1;
    }
}
void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    delay(100);
    Serial.print("Attempting to connect to a network named: ");
    Serial.println(ssid);

    WiFi.begin(ssid,password);
    while (WiFi.status() != WL_CONNECTED)
    {
	Serial.print(".");
	delay(300);
    }
    Serial.println("\nYou are connected to the network");
    Serial.println("Waiting for an IP Address");

    while (WiFi.localIP() == INADDR_NONE)
    {
	Serial.print(".");
	delay(300); 

    }

    Serial.println("\nIP Address obtained");
    printWifiStatus();

    pinMode(0,INPUT_PULLUP);
    attachInterrupt(0,sendRequest,FALLING);
    Serial.println("Push the Button!");

}

void loop() {
    // put your main code here, to run repeatedly:
    String IFTTT_KEY  = "";
    String IFTTT_EVENT = "button_pressed";   //IFTTT maker event name 
    if (trigger == 1)
    {
	iftttTrigger(IFTTT_KEY,IFTTT_EVENT);
	Serial.println("Push The Button");
	trigger = 0;
    }
}
