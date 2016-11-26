#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <GDBStub.h>

#define HEADER_BYTES 551
char groupMeToken[41] = {'\0'};
uint32_t groupIds[3] = {0};
char groupMessages[3][141] = {'\0'};
// Do we need a global array of group names?

bool writeConfigFile();
bool readConfigFile();

void setup()
{
	Serial.begin(115200);
	SPIFFS.begin();
	readConfigFile();
	// Check if the GroupMe token was in the config file

	// Check if any of the required fields need to be added to the user for the
	// device to function correctly
	// TODO: Should we check each of the entries in the variables to make sure
	// they are valid?
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
		char groupIdStr[48] = {'\0'};
		char htmlId[4] = {'\0'};
		for (uint8_t i = 0; i < 3; i++)
		{
			sprintf(groupIdStr, "%u", groupIds[i]);
			sprintf(htmlId, "g_%u", i);
			paramGroupIds.push_back(WiFiManagerParameter(htmlId, "GroupMe URL", groupIdStr, 48));
			wifiManager.addParameter(&paramGroupIds[i]);
			sprintf(htmlId, "m_%u", i);
			paramGroupMsgs.push_back(WiFiManagerParameter(htmlId, "Message", groupMessages[i], 141));
			wifiManager.addParameter(&paramGroupMsgs[i]);
		}

		//wifiManager.addParameter(&paramGroupOne);
		//wifiManager.addParameter(&paramMessageOne);

		//wifiManager.addParameter(&paramGroupTwo);
		//wifiManager.addParameter(&paramMessageTwo);

		//wifiManager.addParameter(&paramGroupThree);
		//wifiManager.addParameter(&paramMessageThree);

		wifiManager.startConfigPortal();

		for (uint8_t i = 0; i < 3; i++)
		{
			strcpy(groupUrl, paramGroupIds[i].getValue());
			strcpy(groupMessages[i], paramGroupMsgs[i].getValue());

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
	}

	writeConfigFile();

	SPIFFS.end();
}

void loop()
{
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

		if (!root.success()) {
			Serial.println("JSON parseObject() failed");
			return false;
		}
		else
		{
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
	for (uint8_t i = 0; i < 3; i++)
	{
		arrIds.add(groupIds[i]);
	}
	JsonArray &arrMsgs = root.createNestedArray("messages");
	for (uint8_t i = 0; i < 3; i++)
	{
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
