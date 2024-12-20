/*
 */
#include <SPI.h>
#include <WiFiNINA.h>

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

// the IP address for the shield:
IPAddress ip(192, 168, 1, 49);   

int levelMeterHigh = 2;
int levelMeterLow = 3;
int relayPin = 5;
int lastTankState = 0;
int manualMode = 0;         // start in automated mode
int status = WL_IDLE_STATUS;      //connection status
WiFiServer server(80);            //server socket

WiFiClient client = server.available();

void setup() {
  Serial.begin(9600);      // initialize serial communication
  pinMode(25, OUTPUT);      // set the LED pin mode
  WiFi.config(ip);

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

/*  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }*/

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(3000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status

  pinMode(levelMeterHigh, INPUT);
  pinMode(levelMeterLow, INPUT);
  pinMode(relayPin, OUTPUT);
}


void loop() {
  int levelMeterHighState = digitalRead(levelMeterHigh);
  int levelMeterLowState = digitalRead(levelMeterLow);
  int relayPinState = 0;
  // print out the state of the water level meters:
  /* 
    With the float device hanging down when not submerged
    0 == NOT submerged
    1 == Submerged
  */
  Serial.print(levelMeterHighState);
  Serial.print("\t");
  Serial.print(levelMeterLowState);
  
  if (levelMeterHighState == 0 && levelMeterLowState == 0 && manualMode == 0) {
    lastTankState = 0; // empty
  }
  else if (levelMeterHighState == 1 && levelMeterLowState == 1 && manualMode == 0) {
    lastTankState = 1; //full
  }
  if (lastTankState == 0 && levelMeterHighState == 1 && levelMeterLowState == 1 && manualMode == 0){
    digitalWrite(relayPin, HIGH);
    relayPinState=1;
  }
  else if (lastTankState == 1 && levelMeterLowState == 1 && manualMode == 0){
    digitalWrite(relayPin, HIGH);
    relayPinState=1;
  }
  else if (manualMode == 0) {
    digitalWrite(relayPin, LOW);
    relayPinState=0;   
  } 
  Serial.print("\t");
  Serial.print(lastTankState); 
  Serial.print("\t");
  Serial.print(relayPinState); 
  Serial.println();
  
  delay(1000);
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got twuo newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            //client.println("HTTP/1.1 200 OK");
            client.println("<!DOCTYPE html>");
            client.println("<html>");
            /*client.println("<head> <meta http-equiv=\"refresh\" content=\"1\">\*/
            client.println("<head> \
            <script> \
            setTimeout(function () { \
            window.location.reload(); \
            }, 5000); \
            </script> \
            <style> \
              body { \
                background-color: linen;\
              } \
              h1 { \
                color: maroon;\
                margin-left: 40px; \
              } \
            </style> \
  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\" integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\"> \
            <title>Wombat Sump Pump</title> </head>");
            client.println("");
            client.println("<body> <h1> WOMBAT Sump Pump</h1> \
            <p> <i class=\"fas fa-poo\" style=\"color:#059e8a;\"></i></p> \
            <p> Currently the sump pump is ");
            if (relayPinState == 1) {
              client.println("<span style=\"color: green\">ON</span><br>");
            }
            else if (relayPinState == 0) {
              client.println("<span style=\"color: grey\">off</span><br>");
            }

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> to turn the pump on<br>");
            client.print("Click <a href=\"/L\">here</a> to turn the pump off<br>");
            if (levelMeterLowState == 0) {
              client.print("<br> Level switch LOW is <span style=\"color: green\">dry</span>");
            }
            else if (levelMeterLowState == 1) {
              client.print("<br> Level switch LOW is <span style=\"color: blue\">submerged</span>");
            }
            if (levelMeterHighState == 0) {
              client.print("<br> Level switch HIGH is <span style=\"color: green\">dry</span>");
            }
            else if (levelMeterHighState == 1) {
              client.print("<br> Level switch HIGH is <span style=\"color: blue\">submerged</span>");
            }
            client.print("<br> Operation is currently in ");
            if (manualMode == 0) {
              client.print("Automated mode<br>");
            }
            else if (manualMode == 1) {
              client.print("Manual Override mode<br>");
            }
            client.print("<a href=\"/A\"><large>A</h1></large> to operate pump in automated mode<br>");
            client.print("<br></body>");
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the Pump on
          digitalWrite(relayPin, HIGH);               // GET /H turns the Pump on
          manualMode = 1;
          relayPinState = 1;
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(relayPin, LOW);                // GET /L turns the Pump off
          manualMode = 1;
          relayPinState = 0;
        }
        if (currentLine.endsWith("GET /A")) {
          manualMode = 0;
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
