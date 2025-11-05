#include <WiFi.h>
#include <WebServer.h>

// ======================================
// 1. WIFI SETTINGS
// ======================================
const char* ssid = "iPhone serkan"; 
const char* password = "seko1234"; 

WebServer server(80);

// ======================================
// 2. HARDWARE (PIN) SETTINGS
// ======================================
const int PUMP_RELAY_PIN = 23;      
const int SENSOR_VCC_PIN = 4;       
const int MOISTURE_PIN = 34;        

const int PUMP_ON = LOW; 
const int PUMP_OFF = HIGH; 

const int WATERING_DURATION_MS = 3000; 
const int DRY_THRESHOLD = 2800;       

int currentMoisture = 0;          
bool wateringActive = false;        

// ======================================
// READ SOIL MOISTURE
// ======================================
int readMoisture() {
  digitalWrite(SENSOR_VCC_PIN, HIGH); 
  delay(10); 
  int moistureValue = analogRead(MOISTURE_PIN);
  digitalWrite(SENSOR_VCC_PIN, LOW); 
  currentMoisture = moistureValue; 
  return moistureValue;
}

// ======================================
// WATERING FUNCTION
// ======================================
void startWatering() {
  if (!wateringActive) { 
    Serial.println(">>> Manual Command: Watering started!");
    wateringActive = true;
    
    digitalWrite(PUMP_RELAY_PIN, PUMP_ON); 
    delay(WATERING_DURATION_MS); 
    
    digitalWrite(PUMP_RELAY_PIN, PUMP_OFF); 
    wateringActive = false;
    Serial.println("Watering completed.");
  } else {
    Serial.println("Watering already in progress. Command skipped.");
  }
}

// ======================================
// WEB SERVER HANDLERS
// ======================================

// Returns only the soil moisture value
void handleData() {
  readMoisture();
  server.send(200, "text/plain", String(currentMoisture));
}

// Main HTML page
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>Smart Plant Irrigation System</title>
<style>
  body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    text-align: center;
    margin: 0;
    padding: 0;
    background: linear-gradient(180deg, #e8f5e9 0%, #f1f8e9 100%);
    color: #2e7d32;
  }
  .container {
    max-width: 420px;
    margin: 50px auto;
    background: #ffffff;
    border-radius: 15px;
    box-shadow: 0 4px 10px rgba(0,0,0,0.15);
    padding: 25px;
  }
  h1 {
    color: #388e3c;
    margin-bottom: 20px;
  }
  .data-box {
    border: 2px solid #a5d6a7;
    border-radius: 10px;
    padding: 20px;
    background-color: #f9fff9;
    margin-bottom: 25px;
  }
  #moisture-value {
    font-size: 3em;
    font-weight: bold;
    color: #1b5e20;
  }
  #status-text {
    font-size: 1.3em;
    font-weight: 600;
    margin-top: 10px;
  }
  .status-dry {
    color: #d32f2f;
  }
  .status-medium {
    color: #f9a825;
  }
  .status-wet {
    color: #43a047;
  }
  .btn {
    padding: 12px 28px;
    font-size: 1.1em;
    border: none;
    border-radius: 30px;
    cursor: pointer;
    transition: all 0.3s ease;
    display: inline-block;
    text-decoration: none;
  }
  .btn-water {
    background-color: #43a047;
    color: white;
  }
  .btn-water:hover {
    background-color: #2e7d32;
  }
  footer {
    margin-top: 25px;
    font-size: 0.9em;
    color: #666;
  }
</style>
</head>
<body>
  <div class="container">
    <h1>Smart Plant Irrigation System</h1>

    <div class="data-box">
      <p>Soil Moisture Value:</p>
      <div id="moisture-value">Loading...</div>
      <p id="status-text">Checking status...</p>
    </div>

    <p><b> Manual Watering Control</b></p>
    <a href="/sulama">
      <button class="btn btn-water">Start 3-Second Watering</button>
    </a>

    <footer>
      Powered by ESP32 Smart Systems
    </footer>
  </div>

  <script>
    function getMoistureData() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var rawValue = parseInt(this.responseText.trim());
          document.getElementById("moisture-value").innerHTML = rawValue;

          var statusText = document.getElementById("status-text");

          if (rawValue > 2800) {
            statusText.innerHTML = "VERY DRY (Immediate Watering Needed)";
            statusText.className = "status-dry";
          } else if (rawValue > 1800) {
            statusText.innerHTML = "MODERATELY MOIST";
            statusText.className = "status-medium";
          } else {
            statusText.innerHTML = "SUFFICIENTLY WET";
            statusText.className = "status-wet";
          }
        }
      };
      xhr.open("GET", "/data", true);
      xhr.send();
    }

    // Refresh data every 2 seconds
    setInterval(getMoistureData, 2000);
    window.onload = getMoistureData;
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleWatering() {
  startWatering(); 
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Watering completed. Redirecting...");
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

// ======================================
// SETUP
// ======================================
void setup() {
  Serial.begin(115200);
  
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(SENSOR_VCC_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, PUMP_OFF);
  digitalWrite(SENSOR_VCC_PIN, LOW); 

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("Web Interface IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot); 
  server.on("/data", handleData); 
  server.on("/sulama", handleWatering); 
  server.onNotFound(handleNotFound); 
  
  server.begin(); 
  Serial.println("HTTP Web Server Started. Open the IP address above in your browser.");
}

// ======================================
// LOOP
// ======================================
void loop() {
  server.handleClient();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    WiFi.reconnect();
  }
}
