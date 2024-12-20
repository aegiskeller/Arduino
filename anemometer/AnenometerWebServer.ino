// This is the curent working version 
// do not know why I can not get temperature to show on html
// Import required libraries
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "DHT.h"

#include "arduino_secrets.h" 

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)
IPAddress staticIP(192, 168, 1, 75);  // Replace with your desired static IP address
IPAddress gateway(192, 168, 1, 1);     // Replace with your gateway IP address
IPAddress subnet(255, 255, 0, 0);    // Replace with your subnet mask

#define DHT22_PIN 4
DHT dht22(DHT22_PIN, DHT22);

int ANENOM = A0;

// wind speed, updated in loop()
int wind = 0;
float tempC = 0;
float humi = 0;
float windavg = 0;
// int windmax = 0;
// int currentwind = 0;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time was updated
unsigned long previousMillisAvg = 0;    // will store last time was updated

unsigned int numsamp = 0;
unsigned long windsamps = 0;

// Updates readings every 10 seconds
const long interval = 10000;  

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ws-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP8266 Wind Speed Server</h2>
  <p>
    <span class="ws-labels">Wind Speed</span> 
    <span id="windspeed">%WINDSPEED%</span>
    <sup class="units">ADU</sup>
  </p>
  <p>
    <span class="ws-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">p.c.</sup>
  </p>
  <p>
    <span class="ws-labels">Temperature</span>
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">C</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("windspeed").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/windspeed", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

</script>
</html>)rawliteral";

// Replaces placeholder with WS values
String processor(const String& var){
  //Serial.println(var);
  if(var == "WINDSPEED"){
    return String(wind);
  }
  // else if(var == "AVGWINDSPEED"){
  //   return String(windavg);
  // }
  else if(var == "HUMIDITY"){
    return String(humi);
  }
  else if(var == "TEMPERATURE"){
    return String(tempC);
  }
  return String();
}

void ServerNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht22.begin(); // initialize the DHT22 sensor
 
// Connect to Wi-Fi
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, pass);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(tempC).c_str());
    //request->send_P(200, "text/plain", tempC);
  });
  server.on("/windspeed", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(wind).c_str());
  });
  // server.on("/avgwindspeed", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", String(windavg).c_str());
  // });
  // server.on("/windmaxspeed", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", String(windmax).c_str());
  // });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(humi).c_str());
  });

  // Start server
  server.begin();
}
 
void loop(){  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the values
    previousMillis = currentMillis;
    // Read wind speed
    float newWind = analogRead(ANENOM);
    // if wind read failed, don't change wind value
    if (isnan(newWind)) {
      Serial.println("Failed to read from anenometer sensor!");
    }
    else {
      wind = newWind;
      Serial.println(wind);
    }
    // read humidity
    humi  = dht22.readHumidity();
    // read temperature as Celsius
    tempC = dht22.readTemperature();
      // check if any reads failed
    if (isnan(humi) || isnan(tempC)) {
      Serial.println("Failed to read from DHT22 sensor!");
    } else {
      Serial.print("DHT22# Humidity: ");
      Serial.print(humi);
      Serial.print("%");

      Serial.print("  |  "); 

      Serial.print("Temperature: ");
      Serial.print(tempC);
      Serial.print("°C");
      // Serial.print(" Wind Avg: ");
      // windavg = windsamps;
      // Serial.print(windavg);
      // Serial.print(" Wind Max: ");
      // Serial.println(windmax);
    }
    numsamp=0;
    windsamps = 0;
    // windmax =0;
  }
  // if (currentMillis - previousMillisAvg >= interval/100) {
  //   // save the last time you updated the values
  //   previousMillisAvg = currentMillis;
  //   // Read wind speed
  //   float newWind = analogRead(ANENOM);
  //   numsamp += 1;
  //   windsamps += newWind;
  //   if (currentwind > windmax) {
  //     windmax = currentwind;
  //   }
  // }
}