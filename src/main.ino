#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <SPI.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>     
#include <climits>
#include <ctype.h>
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
//uint8_t userIdType;
//char singleClickMessage[141];
//char doubleClickMessage[141];
//char holdClickMessage[141];
char key[41];
//string groupName;
//string groupDescription;
//string groupUID;
//string nickname;
//string message;
//string userId;
bool hasDescription;
bool shareGroup = false;

// We could revert this to return a status but since I don't have any idea on
// how to gracefully handle a failure, I think I'm going to keep it as void
// XXX: Add support for various requests starting with a request to get all of the user's current groups!!!
// TODO: Refactor so that the user can just include a number parameter
void groupmeRequest(uint8_t requestType, char groupNames[][256], char groupIds[][9])
{
	uint8_t numGroups = 10;
	char uid[11];
	client.stop();
	// TODO: Change requests to char arrays
	string request;
	string requestBody;
	bool getGroups = false;

	if (client.connect(SERVER, HTTPS_PORT))
	{
		Serial.println("Connected!");

		// Don't send information if the certificates don't match!
		if (client.verify(FINGERPRINT, SERVER)) {
			switch(requestType)
			{
				case GROUPME_INDEX:
					request = request + "GET /v3/groups?per_page=1";
					getGroups = true;
					break;

				// Create a new group:
				// Required values: "name" - The name of this group
				// Optional values: "description" - The description of this group
				//					"share" - Returns share link if true
				// Need to save the group uid
				//case GROUPME_CREATE:
				//	request = request + "POST /v3/groups";
				//	requestBody = requestBody +
				//		"{" +
				//		"	\"name\":\"" + groupName + "\"";

				//	if (hasDescription)
				//	{
				//		requestBody = requestBody + "," +
				//		"	\"description\":\"" + groupDescription + "\"";
				//	}

				//	if (shareGroup)
				//	{
				//		requestBody = requestBody + "," +
				//		"	\"share\":true";
				//	}
				//	requestBody = requestBody +
				//		"}";
				//	client.println(requestBody.c_str());

				//	break;
				//case GROUPME_ADD:
				//	request = request + "POST /v3/groups/" + groupUID + "/members/add/";
				//	requestBody = requestBody +
				//		"{" +
				//		"	\"nickname\":\"" + nickname +"\"";
				//		switch(userIdType)
				//		{
				//			case CONTACT_ID:
				//				requestBody = requestBody + "\"user_id\":\"" + userId + "\"";
				//				break;
				//			case CONTACT_NUM:
				//				requestBody = requestBody + "\"phone_number\":\"" + userId + "\"";
				//				break;
				//			case CONTACT_EMAIL:
				//				requestBody = requestBody + "\"email\":\"" + userId + "\"";
				//				break;
				//		}
				//	break;
				//case GROUPME_MESSAGE:
				//	request = request + "POST /v3/groups/" + groupUID + "/messages";
				//	sprintf(uid, "%d", random(UINT_MAX));
				//	requestBody = requestBody +
				//		"{" +
				//		"	\"message\": {\"" +
				//		"		\"source_guid\":\"" + uid + "\"" +
				//		"		\"text\": " + message + "\"" +
				//		"	}" +
				//		"}";
				//	// Remove all whitespace
				//	//postData.erase(remove_if(postData.begin(), postData.end(), (int(*)(int))isspace), postData.end());
				//	// Insert the message
				//	//postData.replace(postData.rfind("%s"), 2, message);
				//	// Insert the uid
				//	//postData.replace(postData.find("%s"), 2, uid);
				//	break;
			}

			uint8_t pageNum = 1;
			char pageNumStr[3];
			long timeOut = 4000; //capture response from the server
			long lastTime = millis();
			uint32_t bytesAvailable;
			char response[1025];
			// By default, the request gets 1 "page" of results with 10 groups
			// on the page. If we want more groups, we can either have multiple
			// requests or increase the number of groups per page.
			char *pos;
			uint8_t idIdx = 0;
			uint8_t nameIdx = 0;
			uint8_t nameLen;
			Serial.println("certificate matches");
			while (getGroups)
			{
				//request = request + "&token=" + groupmeKey + " HTTP/1.1";
				if (getGroups)
				{
					yield();
					sprintf(pageNumStr, "%d", pageNum);
					Serial.println((request + "&page=" + pageNumStr + "&token=" + key + " HTTP/1.1").c_str());
					client.println((request + "&page=" + pageNumStr + "&token=" + key + " HTTP/1.1").c_str());
					if (pageNum == 10) {
						break;
					}
					pageNum++;
				}

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
				yield();
				lastTime = millis();

				// I inserted a bunch of yields in receiving the response
				// because it takes a while and I believe that inserting the
				// yields will reduce malloc based errors.
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
						//Serial.print(response);
						if (strstr(response, "Content-Length: 36"))
						{
							getGroups = false;
						}
						else
						{
							// TODO: I am really going to need to audit this loop
							// because it is really error prone
							//
							// Search the current string chunk for
							// "group_id":"xxxxxxxx","name":"xxxxxxxxxxx"
							// The problem we need to worry about is that this chunk
							// might split up some of the needed data

							// Find the pointer to the beginning of the group id key.
							if (pageNum - 1 > numGroups)
							{
								//numGroups += 5;
								//// Add another 5 groups if they have all been maxed out
								//groupIds = (char *) realloc(groupIds, numGroups * sizeof(*groupIds));
								//groupNames = (char *) realloc(groupNames, numGroups * sizeof(*groupNames));

								//for (uint8_t i = 5; i > 0; i--)
								//{
								//	groupIds[numGroups - i] = (char *) calloc(9, 1);
								//	groupNames[numGroups - i] = (char *) calloc(256, 1);
								//}
							}

							pos = strstr(response, "\"group_id\":\"");

							if (pos != NULL)// && pos + 12 + 8 < response + bytesAvailable)
							{
								strncpy(groupIds[idIdx], pos + 12, 8);
								// Since the group ids may be less than 8 digits,
								// remove any extraneous characters
								for (uint8_t i = 0; i < 8; i++)
								{
									// Replace non digits with null terminators
									if (!isdigit(groupIds[idIdx][i]))
									{
										groupIds[idIdx][i] = '\0';
										break;
									}
								}

								Serial.println(groupIds[idIdx]);
								idIdx++;
							}

							// Find the pointer to the beginning of the group name
							pos = strstr(response, "\"name\":\"");

							if (pos != NULL && pos + 9 < response + bytesAvailable)
							{
								if (strstr(pos + 8, "\"") != NULL)
								{
									nameLen = (uint8_t) (strstr(pos + 8, "\"") - (pos + 8));
									if (pos + 8 + nameLen < response + bytesAvailable)
									{
										strncpy(groupNames[nameIdx], pos + 8, nameLen);
										Serial.println(groupNames[nameIdx]);
									}
									nameIdx++;
								}
							}
						}
					}
				}
				numGroups = pageNum - 1;
			}

			//if (requestType == GROUPME_INDEX)
			//{
			//	SPIFFS.begin();
			//	File groupMeGroups = SPIFFS.open("/groups", "w");

			//	for (int i = 0; i < pageNum; i++)
			//	{
			//		groupMeGroups.println((string("\"") + groupIds[i] + "\",\"" + groupNames[i] + "\"").c_str());
			//	}

			//	groupMeGroups.close();
			//	SPIFFS.end();
			//}
		} else {
			Serial.println("certificate doesn't match");
		}
	}
	else
	{
		Serial.println("Connection Failed");
	}

	client.stop();
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
	WiFi.disconnect(true);
	//It seems like group IDs can be 8 digits in length. I will change this to a uint32_t though.
	char groupIds[10][9];
	// Group names can be up to 255 characters in length
	char groupNames[10][256];
	//char id[10];

	//for (int i = 0; i < 10; i++)
	//{
	//	groupIds[i] = (char *) calloc(9, 1);
	//	groupNames[i] = (char *) calloc(256, 1);
	//}

	if (SPIFFS.begin()) {
		File config = SPIFFS.open("/groupme", "r");
		File js;

		if (config)
		{
			Serial.println(config.size());
			strcpy(key, config.readStringUntil('\n').c_str());
			Serial.print("GroupMe API Key: ");
			Serial.println(key);

			/*if (groupmeKey.size() == 0)
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
			{*/
				wifiManager.autoConnect();
				groupmeRequest(GROUPME_INDEX, groupNames, groupIds);
				//wifiManager.setConnected(true);
				//wifiManager.setGroups(groupNames, groupIds, 10);
				//WiFi.disconnect(true);
				//wifiManager.startConfigPortal();
			//}
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


		//Serial.println("Push the Button!");
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
