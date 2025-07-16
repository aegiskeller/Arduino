// ESP8266 Water Level Control System with WiFi Web Interface
// Compatible with Wemos D1 Mini and similar ESP8266 boards

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "arduino_secrets.h"

// Pin assignments for Wemos D1 Mini (ESP8266)
// Using GPIO numbers that are safe for ESP8266
const int highLevelPin = 4;   // High water level switch (digital input) - GPIO4 (D2)
const int lowLevelPin = 0;    // Low water level switch (digital input) - GPIO5 (D1) GPIO0 D3
const int relayPin = 2;       // Relay control pin (digital output) - GPIO2 (D4)

// WiFi credentials from arduino_secrets.h
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// Web server on port 80
ESP8266WebServer server(80);

// System state variables
enum PumpMode { AUTOMATIC, MANUAL_ON, MANUAL_OFF };
PumpMode currentMode = AUTOMATIC;
int lastTankState = -1; // -1 means undefined, 0 = empty, 1 = full
bool manualRelayState = false;
unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 1000; // Read sensors every second

void setup() {
  // Initialize serial communication first for debugging
  Serial.begin(115200);
  delay(100);
  
  // Initialize pins
  pinMode(highLevelPin, INPUT_PULLUP);
  pinMode(lowLevelPin, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  
  Serial.println("\n=== ESP8266 Water Level Control System ===");
  Serial.println("Pin Configuration:");
  Serial.println("High Level Pin: GPIO4 (D2)");
  Serial.println("Low Level Pin: GPIO5 (D1)");
  Serial.println("Relay Pin: GPIO2 (D4)");
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup web server routes
  setupWebServer();
  
  Serial.println("System ready!");
  Serial.print("Web interface available at: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Handle web server requests
  server.handleClient();
  MDNS.update();
  
  // Read sensors periodically
  if (millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();
    
    bool highLevel = digitalRead(highLevelPin) == LOW;
    bool lowLevel = digitalRead(lowLevelPin) == LOW;

    // Only print to serial every few seconds to avoid spam
    static unsigned long lastSerialOutput = 0;
    if (millis() - lastSerialOutput >= 5000) {
      Serial.print("High Level: ");
      Serial.print(highLevel ? "ON" : "OFF");
      Serial.print(" | Low Level: ");
      Serial.print(lowLevel ? "ON" : "OFF");
      
      // Determine tank state
      if (!highLevel && !lowLevel) {
        lastTankState = 0; // empty
        Serial.print(" | Tank State: EMPTY");
      }
      else if (highLevel && lowLevel) {
        lastTankState = 1; // full
        Serial.print(" | Tank State: FULL");
      }
      else if (!highLevel && lowLevel) {
        Serial.print(" | Tank State: PARTIAL");
      }
      
      // Show current mode and pump status
      Serial.print(" | Mode: ");
      if (currentMode == AUTOMATIC) Serial.print("AUTO");
      else if (currentMode == MANUAL_ON) Serial.print("MANUAL_ON");
      else if (currentMode == MANUAL_OFF) Serial.print("MANUAL_OFF");
      
      Serial.print(" | Pump: ");
      Serial.println(digitalRead(relayPin) == HIGH ? "ON" : "OFF");
      
      lastSerialOutput = millis();
    }
    
    // Handle automatic mode
    if (currentMode == AUTOMATIC) {
      // Update tank state
      if (!highLevel && !lowLevel) {
        lastTankState = 0; // empty
      }
      else if (highLevel && lowLevel) {
        lastTankState = 1; // full
      }
      
      // Relay control logic for automatic mode
      if (lastTankState == 1) { // Tank is full
        digitalWrite(relayPin, HIGH); // Start emptying
      }
      else if (lastTankState == 0) { // Tank is empty
        digitalWrite(relayPin, LOW); // Stop pump
      }
      // If partial, maintain current state
    }
    // Manual modes are handled by the web interface
  }
  
  // Small delay to prevent excessive CPU usage
  delay(10);
}

void setupWiFi() {
  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  WiFi.hostname("WaterLevelDevice");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(WiFi.hostname());
    
    // Start mDNS responder
    if (MDNS.begin("waterlevel")) {
      Serial.println("mDNS responder started (waterlevel.local)");
    }
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    Serial.println("System will continue with local operation only.");
  }
}

void setupWebServer() {
  // Main page
  server.on("/", handleRoot);
  
  // API endpoints
  server.on("/api/status", handleStatus);
  server.on("/api/mode", HTTP_POST, handleModeChange);
  server.on("/api/pump", HTTP_POST, handlePumpControl);
  
  // Static resources
  server.on("/style.css", handleCSS);
  
  // Start server
  server.begin();
  Serial.println("Web server started on port 80");
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Water Level Control System</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="/style.css">
    <script>
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('highLevel').textContent = data.highLevel ? 'ON' : 'OFF';
                    document.getElementById('lowLevel').textContent = data.lowLevel ? 'ON' : 'OFF';
                    document.getElementById('tankState').textContent = data.tankState;
                    document.getElementById('pumpStatus').textContent = data.pumpStatus;
                    document.getElementById('currentMode').textContent = data.mode;
                    document.getElementById('uptime').textContent = data.uptime;
                    
                    // Update button states
                    document.querySelectorAll('.mode-btn').forEach(btn => btn.classList.remove('active'));
                    document.getElementById('btn-' + data.mode.toLowerCase().replace('_', '-')).classList.add('active');
                    
                    // Update pump control buttons
                    const manualControls = document.getElementById('manualControls');
                    if (data.mode === 'AUTOMATIC') {
                        manualControls.style.display = 'none';
                    } else {
                        manualControls.style.display = 'block';
                        document.querySelectorAll('.pump-btn').forEach(btn => btn.classList.remove('active'));
                        if (data.mode === 'MANUAL_ON') {
                            document.getElementById('btn-pump-on').classList.add('active');
                        } else {
                            document.getElementById('btn-pump-off').classList.add('active');
                        }
                    }
                });
        }
        
        function setMode(mode) {
            fetch('/api/mode', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'mode=' + mode
            }).then(() => updateStatus());
        }
        
        function setPump(state) {
            fetch('/api/pump', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'state=' + state
            }).then(() => updateStatus());
        }
        
        // Update every 2 seconds
        setInterval(updateStatus, 2000);
        updateStatus(); // Initial load
    </script>
</head>
<body>
    <div class="container">
        <h1>Water Level Control System</h1>
        
        <div class="status-grid">
            <div class="status-card">
                <h3>High Level Sensor</h3>
                <div class="status-value" id="highLevel">Loading...</div>
            </div>
            <div class="status-card">
                <h3>Low Level Sensor</h3>
                <div class="status-value" id="lowLevel">Loading...</div>
            </div>
            <div class="status-card">
                <h3>Tank State</h3>
                <div class="status-value" id="tankState">Loading...</div>
            </div>
            <div class="status-card">
                <h3>Pump Status</h3>
                <div class="status-value" id="pumpStatus">Loading...</div>
            </div>
        </div>
        
        <div class="control-section">
            <h3>Control Mode</h3>
            <div class="mode-buttons">
                <button class="mode-btn" id="btn-automatic" onclick="setMode('AUTOMATIC')">AUTO</button>
                <button class="mode-btn" id="btn-manual-on" onclick="setMode('MANUAL_ON')">MANUAL ON</button>
                <button class="mode-btn" id="btn-manual-off" onclick="setMode('MANUAL_OFF')">MANUAL OFF</button>
            </div>
            <div class="current-mode">Current Mode: <span id="currentMode">Loading...</span></div>
        </div>
        
        <div id="manualControls" class="control-section" style="display: none;">
            <h3>Manual Pump Control</h3>
            <div class="pump-buttons">
                <button class="pump-btn" id="btn-pump-on" onclick="setPump('ON')">TURN ON</button>
                <button class="pump-btn" id="btn-pump-off" onclick="setPump('OFF')">TURN OFF</button>
            </div>
        </div>
        
        <div class="info-section">
            <p><strong>Device IP:</strong> )rawliteral" + WiFi.localIP().toString() + R"rawliteral(</p>
            <p><strong>Uptime:</strong> <span id="uptime">Loading...</span></p>
            <p><strong>Hostname:</strong> waterlevel.local</p>
        </div>
    </div>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleCSS() {
  String css = R"rawliteral(
body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    margin: 0;
    padding: 20px;
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    min-height: 100vh;
    color: #333;
}

.container {
    max-width: 800px;
    margin: 0 auto;
    background: white;
    border-radius: 15px;
    box-shadow: 0 10px 30px rgba(0,0,0,0.2);
    padding: 30px;
}

h1 {
    text-align: center;
    color: #2c3e50;
    margin-bottom: 30px;
    font-size: 2.5em;
}

.status-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    gap: 20px;
    margin-bottom: 30px;
}

.status-card {
    background: #f8f9fa;
    padding: 20px;
    border-radius: 10px;
    text-align: center;
    border: 2px solid #e9ecef;
    transition: all 0.3s ease;
}

.status-card:hover {
    transform: translateY(-5px);
    box-shadow: 0 5px 15px rgba(0,0,0,0.1);
}

.status-card h3 {
    margin: 0 0 10px 0;
    color: #495057;
    font-size: 1.1em;
}

.status-value {
    font-size: 1.5em;
    font-weight: bold;
    color: #007bff;
}

.control-section {
    margin-bottom: 30px;
    padding: 20px;
    background: #f8f9fa;
    border-radius: 10px;
}

.control-section h3 {
    margin-top: 0;
    color: #2c3e50;
}

.mode-buttons, .pump-buttons {
    display: flex;
    gap: 10px;
    flex-wrap: wrap;
    justify-content: center;
    margin-bottom: 15px;
}

.mode-btn, .pump-btn {
    padding: 12px 24px;
    border: none;
    border-radius: 8px;
    cursor: pointer;
    font-size: 1em;
    font-weight: 500;
    transition: all 0.3s ease;
    background: #6c757d;
    color: white;
}

.mode-btn:hover, .pump-btn:hover {
    background: #5a6268;
    transform: translateY(-2px);
}

.mode-btn.active {
    background: #28a745;
}

.pump-btn.active {
    background: #dc3545;
}

.current-mode {
    text-align: center;
    font-weight: bold;
    color: #495057;
    margin-top: 15px;
}

.info-section {
    background: #e9ecef;
    padding: 20px;
    border-radius: 10px;
    color: #6c757d;
}

.info-section p {
    margin: 5px 0;
}

@media (max-width: 600px) {
    .container {
        padding: 20px;
    }
    
    .status-grid {
        grid-template-columns: 1fr;
    }
    
    .mode-buttons, .pump-buttons {
        flex-direction: column;
    }
    
    .mode-btn, .pump-btn {
        width: 100%;
    }
}
)rawliteral";
  
  server.send(200, "text/css", css);
}

void handleStatus() {
  // Read current sensor states
  bool highLevel = digitalRead(highLevelPin) == LOW;
  bool lowLevel = digitalRead(lowLevelPin) == LOW;
  
  // Determine tank state
  String tankState = "UNKNOWN";
  if (!highLevel && !lowLevel) {
    tankState = "EMPTY";
  } else if (highLevel && lowLevel) {
    tankState = "FULL";
  } else if (!highLevel && lowLevel) {
    tankState = "PARTIAL";
  }
  
  // Get pump status
  bool pumpRunning = digitalRead(relayPin) == HIGH;
  String pumpStatus = pumpRunning ? "RUNNING" : "STOPPED";
  
  // Get mode string
  String mode = "AUTOMATIC";
  if (currentMode == MANUAL_ON) mode = "MANUAL_ON";
  else if (currentMode == MANUAL_OFF) mode = "MANUAL_OFF";
  
  // Calculate uptime
  unsigned long uptime = millis() / 1000;
  String uptimeStr = String(uptime / 3600) + "h " + String((uptime % 3600) / 60) + "m " + String(uptime % 60) + "s";
  
  String json = "{";
  json += "\"highLevel\":" + String(highLevel ? "true" : "false") + ",";
  json += "\"lowLevel\":" + String(lowLevel ? "true" : "false") + ",";
  json += "\"tankState\":\"" + tankState + "\",";
  json += "\"pumpStatus\":\"" + pumpStatus + "\",";
  json += "\"mode\":\"" + mode + "\",";
  json += "\"uptime\":\"" + uptimeStr + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleModeChange() {
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    if (mode == "AUTOMATIC") {
      currentMode = AUTOMATIC;
      Serial.println("Mode changed to: AUTOMATIC");
    } else if (mode == "MANUAL_ON") {
      currentMode = MANUAL_ON;
      digitalWrite(relayPin, HIGH);
      Serial.println("Mode changed to: MANUAL_ON");
    } else if (mode == "MANUAL_OFF") {
      currentMode = MANUAL_OFF;
      digitalWrite(relayPin, LOW);
      Serial.println("Mode changed to: MANUAL_OFF");
    }
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing mode parameter");
  }
}

void handlePumpControl() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "ON") {
      currentMode = MANUAL_ON;
      digitalWrite(relayPin, HIGH);
      Serial.println("Pump manually turned ON");
    } else if (state == "OFF") {
      currentMode = MANUAL_OFF;
      digitalWrite(relayPin, LOW);
      Serial.println("Pump manually turned OFF");
    }
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing state parameter");
  }
}