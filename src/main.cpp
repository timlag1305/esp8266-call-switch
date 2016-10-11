#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>     

void setup()
{
    Serial.begin(115200);
    WiFiManager wifiManager;
    WiFiManagerParameter roomNum("room_num", "Room Number", "", 4);
    wifiManager.addParameter(&roomNum);
    wifiManager.startConfigPortal("ESP_AP");
    int room = (int) roomNum.getValue();
}

void loop()
{
}
