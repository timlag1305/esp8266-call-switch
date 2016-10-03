#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <WiFiClient.h>

ESP8266WebServer server(80);
File htmlFile;
File cssFile;

void handleRoot()
{
    server.streamFile(htmlFile, "text/html");
}

void handleRequest()
{
    Serial.println("Request received!");
    server.send(200, "text/html", "");
    if (server.args() > 0)
    {
        Serial.println("We have data!");
        for (uint8_t i = 0; i < server.args(); i++)
        {
            Serial.printf("Name: %s\n", server.argName(i).c_str());
        }
    }
}

void setup()
{
    delay(1000);
    Serial.begin(115200);
    SPIFFS.begin();
    htmlFile = SPIFFS.open("/index.html", "r");

    if (!htmlFile)
    {
        Serial.println("File open failed");
    }

    Serial.println();

    Serial.println("Configuring access point...");
    Serial.println("Setting soft-AP ... ");
    WiFi.softAP("test");

    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(ip);
    server.on("/", handleRoot);
    server.on("/request", handleRequest);
    server.begin();
    Serial.println("HTTP server started");
}

void loop()
{
    server.handleClient();
}
