#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <SPI.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>     
#include <climits>
#include <string>

#define SINGLE_CLICK 1
#define DOUBLE_CLICK 2
#define HOLD_CLICK 3
#define CLICK_DELAY 500		// Max delay between double clicks in ms
#define HOLD_LENGTH 1000	// Button hold length in ms
#define KEY_LEN 41
#define SERVER "api.groupme.com"
#define HTTPS_PORT 443
// Fingerprint found using https://www.grc.com/fingerprints.htm
#define FINGERPRINT "ED:22:CB:A5:30:A8:BB:B0:C2:27:93:90:65:CD:64:EA:EA:18:3F:0E"

using namespace std;

WiFiClientSecure client;
uint32_t startTime = 0;
uint32_t prevStartTime = 0;
uint32_t currentTime;
uint8_t clickType = 0;
uint32_t randNumber;
char singleClickMessage[141];
char doubleClickMessage[141];
char holdClickMessage[141];
char key[41];

// We could revert this to return a status but since I don't have any idea on
// how to gracefully handle a failure, I think I'm going to keep it as void
void sendMessage(char message[])
{
	char uid[11];
	client.stop();

	if (client.connect(SERVER, HTTPS_PORT))
	{
		Serial.println("Connected!");
		// Don't send information if the certificates don't match!
		if (client.verify(FINGERPRINT, SERVER)) {
			sprintf(uid, "%d", random(UINT_MAX));
			Serial.println("certificate matches");
			string postData(string("{") +
				"\"message\": {" +
				"\"source_guid\": \"" + uid + "\"," +
				"\"text\": \"" + message + "\"" +
				"}" +
				"}");
			string request("POST /v3/groups/24907887/messages?token=");   //send HTTP POST request
			request += " HTTP/1.1";

			Serial.println(request.c_str());    
			Serial.println(message);
			client.println(request.c_str());
			client.println("Host: api.groupme.com");
			client.println("Content-Type: application/json");
			client.print("Content-Length: ");
			client.println(postData.length());
			client.println();
			client.println(postData.c_str());
			client.println();
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
		} else {
			Serial.println("certificate doesn't match");
		}

	}
	else
	{
		Serial.println("Connection Failed");
	}

	Serial.println();
	Serial.println("Request Complete!!");
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
	delay(1000);
	WiFiManager wifiManager;
	string ssid("");
	char id[10];
	WiFiManagerParameter apiKey("api_key", "Access Token", "", KEY_LEN);
	WiFiManagerParameter singleClick("s_click", "Single Click Message", "", 140);
	WiFiManagerParameter doubleClick("d_click", "Double Click Message", "", 140);
	WiFiManagerParameter holdClick("h_click", "Hold Click Message", "", 140);

	randomSeed(analogRead(0));

	// Add custom inputs for the user
	wifiManager.addParameter(&apiKey);
	wifiManager.addParameter(&singleClick);
	wifiManager.addParameter(&doubleClick);
	wifiManager.addParameter(&holdClick);

	// Add custom JavaScript to get user's GroupMe groups
	wifiManager.setCustomHeadElement(
			"<script>"
			"var groupsSelect;"
			"var groupsMap = [];"
			"function listener() {"
			"	var groups = JSON.parse(this.responseText);"
			"	var groupName;"
			"	var groupId;"
				// Remove the old groups
			"	for (var i = 0; i < groupsSelect.length; i++) {"
			"		groupsSelect.remove(i);"
			"	}"
				// Add the new groups
			"	for (var i = 0; i < groups.length; i++) {"
			"		groupName = groups[i].name;"
			"		groupId = parseInt(groups[i].id);"
			"		groupsSelect.add(groups[i].name);"
			"		groupsMap[groupName] = groupId;"
			"	}"
			"}"
			"document.addEventListener('DOMContentLoaded', function(event) {"
			"	groupsSelect = document.createElement('select');"
			"	var apiKey = document.getElementById('api_key');"
			"	document.body.appendChild(groupsSelect);"
			"	apiKey.addEventListener('input', function() {"
			"		if (apiKey.value.length === 40) {"
			"			var xhr = new XMLHttpRequest();"
			"			xhr.addEventListener('load', listener);"
			"			xhr.open('GET', 'https://api.groupme.com/v3/groups?token=' + apiKey.value);"
			"			xhr.send();"
			"		}"
			"	});"
			"});"
			"</script>"
	);

	// XXX: Remember to change this when done debugging!
	wifiManager.autoConnect();
	//if (digitalRead(0) == LOW)
	//{
	//	wifiManager.autoConnect();
	//}
	// This should allow the user to force the device to go into setup mode but
	// I haven't tested this
	//else
	//{
	//	sprintf(id, "%d", ESP.getChipId());
	//	ssid += string("ESP") + id;
	//	wifiManager.startConfigPortal(ssid.c_str(), "");
	//}

	strcpy(singleClickMessage, singleClick.getValue());
	strcpy(doubleClickMessage, doubleClick.getValue());
	strcpy(holdClickMessage, holdClick.getValue());
	strcpy(key, apiKey.getValue());
	Serial.println(singleClickMessage);
	Serial.println(doubleClickMessage);
	Serial.println(holdClickMessage);

	Serial.println("Push the Button!");
	//pinMode(0, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(0), readPin, CHANGE);
}

void loop() {
	// put your main code here, to run repeatedly:
	currentTime = millis();
	if (clickType == DOUBLE_CLICK)
	{
		sendMessage(doubleClickMessage);
		Serial.println("Double press");
		clickType = 0;
	}
	else if (clickType == SINGLE_CLICK && currentTime - startTime > CLICK_DELAY)
	{
		sendMessage(singleClickMessage);
		Serial.println("Single press");
		clickType = 0;
	}
	else if (clickType == HOLD_CLICK && currentTime - startTime > HOLD_LENGTH)
	{
		sendMessage(holdClickMessage);
		Serial.println("Button Hold");
		clickType = 0;
	}
}
