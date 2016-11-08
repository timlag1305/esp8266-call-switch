#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <SPI.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>     
#include <climits>
#include <string>
#include <algorithm>
#include <cctype>

// Hardware button press definitions
#define SINGLE_CLICK 1
#define DOUBLE_CLICK 2
#define HOLD_CLICK 3
#define CLICK_DELAY 500		// Max delay between double clicks in ms
#define HOLD_LENGTH 1000	// Button hold length in ms

// GroupMe definitions
#define KEY_LEN 41
#define SERVER "api.groupme.com"
#define HTTPS_PORT 443
#define GROUPME_INDEX 20
#define GROUPME_CREATE 21
#define GROUPME_ADD 22
#define GROUPME_MESSAGE 23
#define CONTACT_ID 24
#define CONTACT_NUM 25
#define CONTACT_EMAIL 26
// Fingerprint found using https://www.grc.com/fingerprints.htm
#define FINGERPRINT "ED:22:CB:A5:30:A8:BB:B0:C2:27:93:90:65:CD:64:EA:EA:18:3F:0E"

using namespace std;

WiFiClientSecure client;
uint32_t startTime = 0;
uint32_t prevStartTime = 0;
uint32_t currentTime;
uint8_t clickType = 0;
uint8_t userIdType;
uint32_t randNumber;
char singleClickMessage[141];
char doubleClickMessage[141];
char holdClickMessage[141];
char key[41];
string groupmeKey;
string groupName;
string groupDescription;
string groupUID;
string nickname;
string message;
string userId;
bool hasDescription;
bool shareGroup = false;

struct group
{
	char *group_name;
	uint32_t group_id;
	group *next;
};

struct group *groupHead = (struct group *) malloc(sizeof(struct group));

// We could revert this to return a status but since I don't have any idea on
// how to gracefully handle a failure, I think I'm going to keep it as void
// XXX: Add support for various requests starting with a request to get all of the user's current groups!!!
// TODO: Refactor so that the user can just include a number parameter
void groupmeRequest(uint8_t requestType)
{
	char uid[11];
	client.stop();
	string request;
	string requestBody;

	if (client.connect(SERVER, HTTPS_PORT))
	{
		Serial.println("Connected!");

		// Don't send information if the certificates don't match!
		if (client.verify(FINGERPRINT, SERVER)) {
			switch(requestType)
			{
				case GROUPME_INDEX:
					request = request + "GET /v3/groups";
					break;

				// Create a new group:
				// Required values: "name" - The name of this group
				// Optional values: "description" - The description of this group
				//					"share" - Returns share link if true
				// Need to save the group uid
				case GROUPME_CREATE:
					request = request + "POST /v3/groups";
					requestBody = requestBody +
						"{" +
						"	\"name\":\"" + groupName + "\"";

					if (hasDescription)
					{
						requestBody = requestBody + "," +
						"	\"description\":\"" + groupDescription + "\"";
					}

					if (shareGroup)
					{
						requestBody = requestBody + "," +
						"	\"share\":true";
					}
					requestBody = requestBody +
						"}";
					client.println(requestBody.c_str());

					break;
				case GROUPME_ADD:
					request = request + "POST /v3/groups/" + groupUID + "/members/add/";
					requestBody = requestBody +
						"{" +
						"	\"nickname\":\"" + nickname +"\"";
						switch(userIdType)
						{
							case CONTACT_ID:
								requestBody = requestBody + "\"user_id\":\"" + userId + "\"";
								break;
							case CONTACT_NUM:
								requestBody = requestBody + "\"phone_number\":\"" + userId + "\"";
								break;
							case CONTACT_EMAIL:
								requestBody = requestBody + "\"email\":\"" + userId + "\"";
								break;
						}
					break;
				case GROUPME_MESSAGE:
					request = request + "POST /v3/groups/" + groupUID + "/messages";
					sprintf(uid, "%d", random(UINT_MAX));
					requestBody = requestBody +
						"{" +
						"	\"message\": {\"" +
						"		\"source_guid\":\"" + uid + "\"" +
						"		\"text\": " + message + "\"" +
						"	}" +
						"}";
					// Remove all whitespace
					//postData.erase(remove_if(postData.begin(), postData.end(), (int(*)(int))isspace), postData.end());
					// Insert the message
					//postData.replace(postData.rfind("%s"), 2, message);
					// Insert the uid
					//postData.replace(postData.find("%s"), 2, uid);
					break;
			}
			Serial.println("certificate matches");
			request = request + "?token=" + groupmeKey + " HTTP/1.1";
			Serial.println(request.c_str());
			client.println(request.c_str());
			client.println("Host: api.groupme.com");

			if (requestBody.size() > 0)
			{
				client.println("Content-Type: application/json");
				client.print("Content-Length: ");
				client.println(requestBody.size());
				client.println();
				client.println(requestBody.c_str());
			}

			client.println();
			long timeOut = 4000; //capture response from the server
			long lastTime = millis();
			int bytesAvailable;
			int readLen;
			char c;
			char response[1025];
			int pos = 0;
			response[1024] = '\0';
			group *curGroup;
			curGroup = groupHead;

			// There's some sort of dangling pointer or something...
			while ((millis() - lastTime) < timeOut)	 //wait for incoming response 
			{
				// Need to read in chunks because it's gonna be UUUUGGGEEEE
				while ((bytesAvailable = client.available()))           //characters incoming from server
				{
					if (bytesAvailable < 1024)
					{
						client.readBytes(response, bytesAvailable);
					}
					else
					{
						client.readBytes(response, 1024);          //read characters
					}

					// TODO: I am really going to need to audit this loop
					// because it is really error prone
					//
					// Search the current string chunk for
					// "group_id":"xxxxxxxx","name":"xxxxxxxxxxx"
					// The problem we need to worry about is that this chunk
					// might split up some of the needed data
					string respString(response);
					if (pos < respString.size())
					{
						Serial.println("legit!");
						pos = respString.find("\"group_id\":\"") + 12;

						// TODO: Rework this because technically group_id could be after group_name
						// The id is 8 digits
						if (pos + 8 < respString.size())
						{
							curGroup->group_id = (uint32_t) atol(respString.substr(pos, 8).c_str());
							Serial.println(curGroup->group_id);
							//pos += 8;
							//pos = respString.find("\"name\":\"", pos) + 7;

							//if (pos < respString.size())
							//{
							//	int endPos = respString.find("\"", pos + 1);
							//}
						}
					}
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
	if (SPIFFS.begin()) {
		File config = SPIFFS.open("/groupme", "r");
		File js;

		if (config)
		{
			Serial.println(config.size());
			//groupmeKey = string(config.readString().c_str());
			groupmeKey = config.readStringUntil('\n').c_str();
			Serial.print("GroupMe API Key: ");
			Serial.println(groupmeKey.c_str());

			if (groupmeKey.size() == 0)
			{
				WiFiManagerParameter apiKey("api_key", "Access Token", "MpgDBEjejSVNUa1kFrIqQGt0U57WYDg6M4qm1LdU", KEY_LEN);
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
				js = SPIFFS.open("/min.js", "r");

				if (js != 0) {
					Serial.println(js.size());
					Serial.println(js.readString());
					wifiManager.setCustomHeadElement(js.readString().c_str());
				}

				// XXX: Remember to change this when done debugging!
				wifiManager.autoConnect();
				strcpy(singleClickMessage, singleClick.getValue());
				strcpy(doubleClickMessage, doubleClick.getValue());
				strcpy(holdClickMessage, holdClick.getValue());
				strcpy(key, apiKey.getValue());
				Serial.println(singleClickMessage);
				Serial.println(doubleClickMessage);
				Serial.println(holdClickMessage);
				config.write((const uint8_t *) key, 40);
				config.write('\n');
			}
			else
			{
				wifiManager.autoConnect();
				groupmeRequest(GROUPME_INDEX);
			}
		}

		config.close();
		SPIFFS.end();
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


		Serial.println("Push the Button!");
		//pinMode(0, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(0), readPin, CHANGE);
	}
}

void loop() {
	// put your main code here, to run repeatedly:
	currentTime = millis();
	if (clickType == DOUBLE_CLICK)
	{
		//sendMessage(doubleClickMessage);
		Serial.println("Double press");
		clickType = 0;
	}
	else if (clickType == SINGLE_CLICK && currentTime - startTime > CLICK_DELAY)
	{
		//sendMessage(singleClickMessage);
		Serial.println("Single press");
		clickType = 0;
	}
	else if (clickType == HOLD_CLICK && currentTime - startTime > HOLD_LENGTH)
	{
		//sendMessage(holdClickMessage);
		Serial.println("Button Hold");
		clickType = 0;
	}

}
