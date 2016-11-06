#include <ESP8266WiFi.h>

String apiKey = "";
char ssid[] = "";
char password[] = "s";
const char* server = "";


int holdPin = 0;  // defines the gpio 0 as the hold pin- will hold the CH_PD high until we power down
int buttonPin = 2; //defines the GPIO 2 as the PIR read pin- reads the state of the button output)
int button = 1; //sets the button to 1 - it must have been woken up

WiFiClient client;

void setup() {

 pinMode(holdPin ,OUTPUT);
 digitalWrite(holdPin,HIGH); //holds the CH_PD to high even if the the button goes low
 pinMode(buttonPin , INPUT); //sets the GPIO 2 to an input so we can read the button output state

 WiFi.begin(ssid,password);
 while(WiFi.status() != WL_CONNECTED)
 {
  delay(300);
 }
 
}

void loop() {
  if(client.connect(server,80))
  {

   String postStr = apiKey;
          postStr += "";
   Client.print("POST /update HTTP\1.1\n")
   Client.print("Host:  "); 
   Client.print("Connection: close\n"); 
   Client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n"); 
   Client.print("Content-Type: application/x-www-form-urlencoded\n"); 
   Client.print("Content-Length: "); 
   Client.print(postStr.length()); 
   Client.print("\n\n"); 
   Client.print(postStr);
  }
  if((button) == 0)
  {
    Client.stop();
    delay(1000);
    digitalWrite(holdPin, LOW); // sets GPIO 0 low this takes the CH_PD & powers down the esp
      
  }
  else
  {
    while(digitalRead(buttonPin) == 1)
    {
       delay(500);  
    }
    button = 0;
    delay(2000); //wait 20 seconds , stops updating the server
  }
}
