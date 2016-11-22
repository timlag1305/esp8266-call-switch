#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#define HEADER_BYTES 551
char groupMeToken[41];
uint32_t groupIds[3];
// Do we need a global array of group names?

bool writeConfigFile();
bool readConfigFile();

void setup()
{
	Serial.begin(115200);
	SPIFFS.begin();
	readConfigFile();
	// Check if the GroupMe token was in the config file
	if (groupMeToken[0] == '\0')
	{
		strcpy(groupMeToken, "GroupMe Token");
	}

	if (WiFi.SSID() == "")
	{
		WiFiManager wifiManager;
		WiFiManagerParameter paramGroupMeToken("groupme_key", groupMeToken,
				groupMeToken, 41);
		wifiManager.addParameter(&paramGroupMeToken);
		wifiManager.startConfigPortal();
		strcpy(groupMeToken, paramGroupMeToken.getValue());
		writeConfigFile();
		Serial.println(paramGroupMeToken.getValue());
	}

	SPIFFS.end();

	WiFiClientSecure client;
	char response[HEADER_BYTES + 1];
	uint16_t respSize;
	//char *startPos, *endPos;
	uint32_t groupIds[10];
	char groupNames[5][256];
	for(uint8_t i = 0; i < 10; i++)
	{
		if (client.connect("api.groupme.com", 443))
		{
			client.print(String("GET ") + F("/v3/groups?per_page=1&page=") + (i + 1) +
					F("&token=") + groupMeToken +
					F(" HTTP/1.1\r\nHost: api.groupme.com\r\nConnection: close\r\n\r\n"));
			//Serial.print(String("GET ") + "/v3/groups?per_page=1&page=" + (i + 1) +
			//		"&token=" + groupMeToken +
			//		" HTTP/1.1\r\nHost: api.groupme.com\r\nConnection: close\r\n\r\n");
			unsigned long timeout = millis();
			while (client.available() == 0)
			{
				if (millis() - timeout > 5000)
				{
					break;
				}
				yield();
			}
			while ((respSize = client.available()))
			{
				if (respSize >= HEADER_BYTES)
				{
					std::fill(response, response + HEADER_BYTES + 1, '\0');
					client.readBytes(response, HEADER_BYTES);
					//Serial.print(response + 21);
					//startPos = strstr(response, "\"name\":\"");

					// Find the part of the response with the group name
					//if (startPos != NULL &&
					//	(endPos = strstr(startPos + 8, "\"")) != NULL &&
					//	endPos - startPos < 256)
					//{
					//	startPos += 8;
					//	for (uint8_t j = 0; j < endPos - startPos; j++)
					//	{
					//		groupNames[i][j] = startPos[j];
					//	}
					//	Serial.println(groupNames[i]);
					//	strncpy(groupNames[i], startPos + 8, 1);
					//}

					// Try and find a group ID. They can be at different
					// offsets so check multiple offsets
					if ((groupIds[i] = strtol(response + 20, NULL, 10)) ||
							(groupIds[i] = strtol(response + 21, NULL, 10)))
					{
						Serial.println(groupIds[i]);
						client.flush();
						break;
					}
				}

				yield();
			}
			Serial.println();
		}
		else
		{
			Serial.println("Could not connect.");
		}

		client.stop();
	}
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
		StaticJsonBuffer<200> jsonBuffer;
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
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	json["token"] = groupMeToken;
	// Add other options to the config file
	if (f)
	{
		json.printTo(f);
		f.close();
	}
}
