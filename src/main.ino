#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <GDBStub.h>

#define HEADER_BYTES 551
char groupMeToken[41];
uint32_t groupIds[3];
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
	if (groupMeToken[0] == '\0')
	{
		//strcpy(groupMeToken, "GroupMe Token");
	}


	if (WiFi.SSID() == "")
	{
		char groupUrls[3][48] = {'\0'};
		WiFiManager wifiManager;
		// Add the groups ids
		WiFiManagerParameter paramGroupOne("g_one", "GroupMe URL", groupUrls[0], 48);
		WiFiManagerParameter paramGroupTwo("g_two", "GroupMe URL", groupUrls[1], 48);
		WiFiManagerParameter paramGroupThree("g_three", "GroupMe URL", groupUrls[2], 48);
		// Add the messages to send to group
		WiFiManagerParameter paramMessageOne("m_one", "Message", groupMessages[0], 141);
		WiFiManagerParameter paramMessageTwo("m_two", "Message", groupMessages[1], 141);
		WiFiManagerParameter paramMessageThree("m_three", "Message", groupMessages[2], 141);

		wifiManager.addParameter(&paramGroupOne);
		wifiManager.addParameter(&paramMessageOne);

		wifiManager.addParameter(&paramGroupTwo);
		wifiManager.addParameter(&paramMessageTwo);

		wifiManager.addParameter(&paramGroupThree);
		wifiManager.addParameter(&paramMessageThree);

		wifiManager.startConfigPortal();

		strcpy(groupUrls[0], paramGroupOne.getValue());
		strcpy(groupUrls[1], paramGroupTwo.getValue());
		strcpy(groupUrls[2], paramGroupThree.getValue());

		for (uint8_t i = 0; i < 3; i++)
		{
			Serial.println("finding ids");
			for (uint8_t j = 0; j < 141; j++)
			{
				if ((groupIds[i] = strtol(groupUrls[i] + j, NULL, 10)) ||
					(groupIds[i] = strtol(groupUrls[i] + j, NULL, 10)))
				{
					Serial.println(groupIds[i]);
					break;
				}
			}
		}

		strcpy(groupMessages[0], paramMessageOne.getValue());
		strcpy(groupMessages[1], paramMessageTwo.getValue());
		strcpy(groupMessages[2], paramMessageThree.getValue());

	}

	for (uint8_t i = 0; i < 3; i++)
	{
		strcpy(groupMessages[i], "test1");
		groupIds[i] = 24907887;
	}

	writeConfigFile();

	SPIFFS.end();
	//groupMeGroups(groupIds, groupNames);
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

void groupMeGroups(uint32_t groupIds[], char groupNames[][256])
{
	WiFiClientSecure client;
	char response[HEADER_BYTES + 1];
	uint16_t respSize;
	char *startPos, *endPos;
	bool infoFound;
	// Note: Putting a yield in here seems to cause problems with correctly
	// getting the json data
	for (uint8_t i = 0; i < 5; i++)
	{
		infoFound = false;

		if (client.connect("api.groupme.com", 443))
		{
			Serial.println(ESP.getFreeHeap());
			client.print(String("GET ") + F("/v3/groups?per_page=1&page=") + (i + 1) +
					F("&token=") + groupMeToken +
					F(" HTTP/1.1\r\nHost: api.groupme.com\r\nConnection: close\r\n\r\n"));
			//Serial.print(String("GET ") + "/v3/groups?per_page=1&page=" + (i + 1) +
			//		"&token=" + groupMeToken +
			//		" HTTP/1.1\r\nHost: api.groupme.com\r\nConnection: close\r\n\r\n");
			//unsigned long timeout = millis();
			while (client.connected() && client.available() == 0)
			{
				//yield();
			}

			while (client.connected())
			{
				while (respSize = client.available())
				{
					// Apparently closing the connection is not enough. You need to
					// also read in all of the bytes to prevent memory leaks
					if (infoFound)
					{
						if (respSize >= HEADER_BYTES)
						{
							client.readBytes(response, HEADER_BYTES);
						}
						else
						{
							client.readBytes(response, respSize);
						}
					}
					else if (respSize >= HEADER_BYTES)
					{
						std::fill(response, response + HEADER_BYTES + 1, '\0');
						client.readBytes(response, HEADER_BYTES);
						//Serial.print(response + 21);
						startPos = strstr(response, "\"name\":\"");

						// Find the part of the response with the group name
						if (startPos != NULL &&
							(endPos = strstr(startPos + 8, "\"")) != NULL &&
							endPos - startPos < 256)
						{
							startPos += 8;
							Serial.println();
								//Serial.println((uint32_t) (endPos - startPos));
							for (uint8_t j = 0; j < endPos - startPos; j++)
							{
								groupNames[i][j] = startPos[j];
							}
							//strncpy(groupNames[i], startPos + 8, endPos - startPos);
							//Serial.println(groupNames[i]);
						}

						// Try and find a group ID. They can be at different
						// offsets so check multiple offsets
						if ((groupIds[i] = strtol(response + 20, NULL, 10)) ||
								(groupIds[i] = strtol(response + 21, NULL, 10)))
						{
							Serial.println(groupIds[i]);
							infoFound = true;
						}
					}
					else
					{
						std::fill(response, response + HEADER_BYTES + 1, '\0');
						client.readBytes(response, respSize);
					}
				}
			}
		}
		else
		{
			Serial.println("Could not connect.");
		}
	}
}
