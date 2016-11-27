#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
//#include <GDBStub.h>
#include <climits>

const uint16_t CLICK_DELAY = 500;
const uint16_t HEADER_BYTES = 551;
const uint16_t HOLD_LENGTH = 1000;
const uint8_t DOUBLE_CLICK = 1;
const uint8_t HOLD_CLICK = 2;
const uint8_t INVALID_CLICK = 3;
const uint8_t SINGLE_CLICK = 0;

char groupMeToken[41] = {'\0'};
char groupMessages[3][141] = {'\0'};
uint32_t currentTime;
uint32_t groupIds[3] = {0};
uint32_t prevStartTime = 0;
uint32_t startTime = 0;
uint8_t clickType = INVALID_CLICK;

bool writeConfigFile();
bool readConfigFile();

void setup()
{
	Serial.begin(115200);
	SPIFFS.begin();
	readConfigFile();
	Serial.println();
	// Check if the GroupMe token was in the config file

	// Check if any of the required fields need to be added to the user for the
	// device to function correctly
	// TODO: Should we check each of the entries in the variables to make sure
	// they are valid?
	// TODO: (Part 2) Possibly add some javascript to try and valid the inputs.
	if (WiFi.SSID() == "" || groupMeToken[0] == '\0' || groupIds[0] == 0 || groupMessages[0][0] == '\0')
	{
		char groupUrl[48] = {'\0'};
		WiFiManager wifiManager;
		// Add the groups ids
		WiFiManagerParameter paramGroupToken("g_toke", "GroupMe Token", groupMeToken, 41);
		wifiManager.addParameter(&paramGroupToken);
		std::vector<WiFiManagerParameter> paramGroupIds;
		std::vector<WiFiManagerParameter> paramGroupMsgs;
		paramGroupIds.reserve(3);
		paramGroupMsgs.reserve(3);
		char groupIdStr[3][48] = {'\0'};
		char htmlId[6][4] = {'\0'};

		//WiFiManagerParameter id1("g_0", "GroupMe URL", groupIdStr, 48);
		//WiFiManagerParameter id2("g_1",
		for (uint8_t i = 0; i < 3; i++)
		{
			//Serial.print("Group ID: ");
			//Serial.println(groupIds[i]);
			//Serial.print("Group Message: ");
			//Serial.println(groupMessages[i]);
			sprintf(groupIdStr[i], "%u", groupIds[i]);
			sprintf(htmlId[i], "g_%u", i);
			paramGroupIds.push_back(WiFiManagerParameter(htmlId[i], "GroupMe URL", groupIdStr[i], 48));
			wifiManager.addParameter(&paramGroupIds[i]);
			printf("%p: \n", &paramGroupIds[i]);
			sprintf(htmlId[i + 3], "m_%u", i);
			paramGroupMsgs.push_back(WiFiManagerParameter(htmlId[i + 3], "Message", groupMessages[i], 141));
			printf("%p: \n", &paramGroupMsgs[i]);
			wifiManager.addParameter(&paramGroupMsgs[i]);
		}

		for (uint8_t i = 0; i < 3; i++)
		{
			//wifiManager.addParameter()
		}

		wifiManager.startConfigPortal();
		strcpy(groupMeToken, paramGroupToken.getValue());

		for (uint8_t i = 0; i < 3; i++)
		{
			strcpy(groupUrl, paramGroupIds[i].getValue());
			strcpy(groupMessages[i], paramGroupMsgs[i].getValue());
			Serial.println(groupUrl);
			Serial.println(groupMessages[i]);

			for (uint8_t j = 0; j < 141; j++)
			{
				if ((groupIds[i] = strtol(groupUrl + j, NULL, 10)) ||
					(groupIds[i] = strtol(groupUrl + j, NULL, 10)))
				{
					Serial.println(groupIds[i]);
					break;
				}
			}
		}

		writeConfigFile();
	}

	SPIFFS.end();
	attachInterrupt(digitalPinToInterrupt(0), readPin, CHANGE);
}

void loop()
{
	currentTime = millis();

	// TODO: I could probably combine these if statements into one
	if (clickType == DOUBLE_CLICK)
	{
		sendMessage(clickType);
		clickType = INVALID_CLICK;
	}
	else if (clickType == SINGLE_CLICK && currentTime - startTime > CLICK_DELAY)
	{
		sendMessage(clickType);
		clickType = INVALID_CLICK;
	}
	else if (clickType == HOLD_CLICK && currentTime - startTime > HOLD_LENGTH)
	{
		sendMessage(clickType);
		clickType = INVALID_CLICK;
	}
}

void sendMessage(uint8_t clickType)
{
	const char FINGERPRINT[] = "ED:22:CB:A5:30:A8:BB:B0:C2:27:93:90:65:CD:64:EA:EA:18:3F:0E";
	const char SERVER[] = "api.groupme.com";
	const uint16_t HTTPS_PORT = 443;
	char uid[11];
	char groupIdStr[48] = {'\0'};
	//char postData[190] = {'\0'};
	//char request[90] = {'\0'};
	WiFiClientSecure client;

	if (client.connect(SERVER, HTTPS_PORT))
	{
		if (client.verify(FINGERPRINT, SERVER))
		{
			sprintf(groupIdStr, "%u", groupIds[clickType]);
			sprintf(uid, "%u", random(UINT_MAX));
			// Length of POST data (I believe):
			// 11 +
			// 15 + 10 + 2 +
			// 8 + 140 + 1 +
			// 2 + 1
			//strcpy(postData, "{\"message\":{\"source_guid\":\"");
			//strcat(postData, uid);
			//strcat(postData, "\",\"text\":\"");
			//strcat(postData, groupMessages[clickType]);
			//strcat(postData, "\"}}");
			//std::string postData(std::string("{") +
			//		"\"message\":{" +
			//		"\"source_guid\":\"" + uid + "\"," +
			//		"\"text\":\"" + groupMessages[clickType] + "\"" +
			//		"}" +
			//		"}");
			// Length of request:
			// 16 + 8 + 16 + 40 + 9 + 1
			//strcpy(request, "POST /v3/groups/");
			//strcat(request, groupIdStr);
			//strcat(request, "/messages?token=");
			//strcat(request, groupMeToken);
			//strcat(request, " HTTP/1.1");
			//std::string request(std::string("POST /v3/groups/") + groupIdStr +
			//		"/messages?token=" + groupMeToken + " HTTP/1.1");

			//Serial.println(request);
			//Serial.println(groupMessages[clickType]);
			//client.println(request);
			// Send the request
			Serial.print("POST /v3/groups/");
			Serial.print(groupIdStr);
			Serial.print("/messages?token=");
			Serial.print(groupMeToken);
			Serial.println(" HTTP/1.1");
			Serial.println("Host: api.groupme.com");
			Serial.println("Content-Type: application/json");
			Serial.print("Content-Length: ");
			Serial.println(40 + strlen(uid) +
					strlen(groupMessages[clickType]));
			Serial.println();
			Serial.print("{\"message\":{\"source_guid\":\"");
			Serial.print(uid);
			Serial.print("\",\"text\":\"");
			Serial.print(groupMessages[clickType]);
			Serial.println("\"}}");
			//client.println(postData);
			Serial.println();

			client.print("POST /v3/groups/");
			client.print(groupIdStr);
			client.print("/messages?token=");
			client.print(groupMeToken);
			client.println(F(" HTTP/1.1"));

			client.println("Host: api.groupme.com");
			client.println("Content-Type: application/json");
			client.print("Content-Length: ");
			client.println(40 + strlen(uid) +
					strlen(groupMessages[clickType]));
			client.println();
			client.print("{\"message\":{\"source_guid\":\"");
			client.print(uid);
			client.print("\",\"text\":\"");
			client.print(groupMessages[clickType]);
			client.println("\"}}");
			//client.println(postData);
			client.println();
			long timeOut = 4000;
			long lastTime = millis();

			while ((millis() - lastTime) < timeOut)
			{
				while (client.available())
				{
					char c = client.read();
					Serial.write(c);
				}
			}
		}
		else
		{
			Serial.println("certificate doesn't match");
		}
	}
	else
	{
		Serial.println("Connection Failed");
	}

	Serial.println();
	Serial.println("Request Complete!!");
	ESP.deepSleep(UINT_MAX);
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

bool readConfigFile()
{
	File f = SPIFFS.open("/token", "r");
	if (f)
	{
		const size_t size = f.size();
		std::unique_ptr<char[]> buf(new char[size]);
		f.readBytes(buf.get(), size);
		f.close();
		DynamicJsonBuffer jsonBuffer;
		JsonObject& root = jsonBuffer.parseObject(buf.get());

		if (!root.success())
		{
			Serial.println("JSON parseObject() failed");
			return false;
		}
		else
		{
			root.prettyPrintTo(Serial);

			if (root.containsKey("token"))
			{
				strcpy(groupMeToken, root["token"]);
			}
			if (root.containsKey("ids"))
			{
				root["ids"].asArray().copyTo(groupIds);
			}
			if (root.containsKey("messages"))
			{
				// Must not be possible to get strings in an array
				//root["messages"].asArray().copyTo(groupMessages);
				for (uint8_t i = 0; i < 3; i++)
				{
					strcpy(groupMessages[i], root["messages"][i]);
					Serial.println(groupIds[i]);
					Serial.println(groupMessages[i]);
				}
			}
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool writeConfigFile()
{
	Serial.println("Saving config file");
	File f = SPIFFS.open("/token", "w+");
	StaticJsonBuffer<16 + 3 * (140 + 8)> jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();

	root["token"] = groupMeToken;
	JsonArray &arrIds = root.createNestedArray("ids");

	Serial.println("Write file");
	for (uint8_t i = 0; i < 3; i++)
	{
		Serial.println(groupIds[i]);
		arrIds.add(groupIds[i]);
	}

	JsonArray &arrMsgs = root.createNestedArray("messages");

	for (uint8_t i = 0; i < 3; i++)
	{
		Serial.println(groupMessages[i]);
		arrMsgs.add(groupMessages[i]);
	}

	// Add other options to the config file
	if (f)
	{
		root.printTo(f);
		root.prettyPrintTo(Serial);
		f.close();
	}
}
