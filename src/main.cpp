#include <string>
#include <FS.h>
#include <SPI.h>
#include <WiFiManager.h>     

using namespace std;

void setup()
{
    WiFiManager wifiManager;
    Serial.begin(115200);

    wifiManager.autoConnect();
}

void loop()
{
}
