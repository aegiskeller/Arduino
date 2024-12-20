#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"
#include "arduino_secrets.h"


// Uncomment one of the lines below for whatever DHT sensor type you're using!
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

char ssid[] = SECRET_SSID;        // your network SSID (name)
char password[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)c:\Users\Admin\Documents\Arduino\Arduino\anemometer\arduino_secrets.h
IPAddress staticIP(192, 168, 1, 75);  // Replace with your desired static IP address
IPAddress gateway(192, 168, 1, 1);     // Replace with your gateway IP address
IPAddress subnet(255, 255, 255, 0);    // Replace with your subnet mask

ESP8266WebServer server(80);
int ANENOM = A0;
unsigned long previousMillis = 0;    // will store last time was updated
const long interval = 1000;  

// DHT Sensor
uint8_t DHTPin = 4; 
               
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);                

float Temperature;
float Humidity;
float avgwindspeed=0.0;
int Windspeed;
int cumulWS=0;
int icount=0;
int numsamps = 10;
 
void setup() {
  Serial.begin(115200);
  delay(100);
  
  pinMode(DHTPin, INPUT);

  dht.begin();              

  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.on("/temperature", handle_temp);
  server.on("/humidity", handle_hum);
  server.on("/windspeed", handle_wind);
  server.on("/avgwindspeed", handle_avgwind);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

}
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the values
    previousMillis = currentMillis;
    // Read wind speed
    cumulWS += analogRead(ANENOM);
    icount+=1;
  }
  if (icount > numsamps){
    avgwindspeed = cumulWS/numsamps;
    icount = 0;
    cumulWS = 0;
    Serial.print("avgws: ");
    Serial.println(avgwindspeed);
  }

  server.handleClient();
  
}

void handle_OnConnect() {

  Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity 
  Windspeed = analogRead(ANENOM); // Gets the values of the windspeed 
  server.send(200, "text/html", SendHTML(Temperature,Humidity,Windspeed,avgwindspeed)); 
}

void handle_temp() {
  Temperature = dht.readTemperature(); // Gets the values of the temperature
  server.send(200, "text/plain", String(Temperature).c_str());
}

void handle_hum() {
  Humidity = dht.readHumidity(); // Gets the values of the humidity
  server.send(200, "text/plain", String(Humidity).c_str());
}

void handle_wind() {
  Windspeed = analogRead(ANENOM);
  server.send(200, "text/plain", String(Windspeed).c_str());
}

void handle_avgwind() {
  server.send(200, "text/plain", String(avgwindspeed).c_str());
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float Temperaturestat,float Humiditystat, int Windspeedstat, float avgWindspeedstat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>ESP8266 Weather Report</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>ESP8266 Weather Report</h1>\n";
  
  ptr +="<p>Temperature: ";
  ptr +=Temperaturestat;
  ptr +="&deg;C</p>";
  ptr +="<p>Humidity: ";
  ptr +=Humiditystat;
  ptr +="&percnt;</p>";
  ptr +="<p>Windspeed: ";
  ptr +=Windspeedstat;
  ptr +="ADU</p>";
  ptr +="<p>Avg Windspeed: ";
  ptr +=avgWindspeedstat;
  ptr +="ADU</p>";
  ptr +="</div>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}