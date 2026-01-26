/*
  ESP32-P4 Nano simple web server demo
  - 3 pages: Home (/), Info (/info), Control (/control)
  - Clean, responsive HTML/CSS
  - Shows basic system info and a demo toggle

  NOTE:
  - Uses IP101 Ethernet PHY (internal board)
  - Adjust ETH_* pins if your board wiring differs
  - Works with Arduino-ESP32 core (ESP32 family)
*/

#include <ETH.h>
#include <WiFi.h>
#include <WebServer.h>

// -------- IP101 Ethernet PHY configuration --------
// Update these values based on your ESP32-P4 Nano wiring.
#ifndef ETH_PHY_ADDR
#define ETH_PHY_ADDR 1
#endif
#ifndef ETH_PHY_POWER
#define ETH_PHY_POWER -1
#endif
#ifndef ETH_PHY_MDC
#define ETH_PHY_MDC 23
#endif
#ifndef ETH_PHY_MDIO
#define ETH_PHY_MDIO 18
#endif
#ifndef ETH_CLK_MODE
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#endif

// -------- Web server --------
WebServer server(80);

// Demo state
static bool demoToggle = false;
static bool ethConnected = false;

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname("esp32-p4-nano");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.println(ETH.macAddress());
      Serial.print("IPv4: ");
      Serial.println(ETH.localIP());
      ethConnected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      ethConnected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      ethConnected = false;
      break;
    default:
      break;
  }
}

String ethernetStatus() {
  return ethConnected ? "Connected" : "Disconnected";
}

String ethernetIP() {
  return ethConnected ? ETH.localIP().toString() : String("N/A");
}

String ethernetMac() {
  return ethConnected ? ETH.macAddress() : String("N/A");
}

// HTML layout helper
String htmlHeader(const String &title) {
  String s;
  s.reserve(1400);
  s += "<!doctype html><html lang=\"en\"><head>";
  s += "<meta charset=\"utf-8\">";
  s += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  s += "<title>" + title + "</title>";
  s += "<style>";
  s += ":root{--bg:#0b1220;--panel:#121a2b;--card:#17233a;--accent:#62d2a2;--text:#e7edf6;--muted:#9bb0c8;}";
  s += "*{box-sizing:border-box}body{margin:0;font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;background:linear-gradient(180deg,#0b1220,#0b1220 40%,#0f1a2b);color:var(--text)}";
  s += "header{padding:20px 24px;background:rgba(18,26,43,.7);backdrop-filter:blur(6px);border-bottom:1px solid #1f2a44;position:sticky;top:0}";
  s += "nav a{color:var(--text);text-decoration:none;margin-right:16px;padding:8px 12px;border-radius:8px;background:#1a2640}";
  s += "nav a:hover{background:#223055}";
  s += ".wrap{max-width:900px;margin:24px auto;padding:0 20px}";
  s += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:16px}";
  s += ".card{background:var(--card);border:1px solid #223055;border-radius:14px;padding:16px;box-shadow:0 8px 20px rgba(0,0,0,.18)}";
  s += ".muted{color:var(--muted)}";
  s += ".btn{display:inline-block;padding:10px 14px;border-radius:10px;background:var(--accent);color:#0b1220;text-decoration:none;font-weight:600}";
  s += ".badge{display:inline-block;padding:4px 10px;border-radius:999px;background:#24365e;color:var(--text);font-size:12px}";
  s += "footer{margin-top:24px;padding:20px 24px;color:var(--muted);text-align:center}";
  s += "</style></head><body>";
  s += "<header><div class=\"wrap\">";
  s += "<div class=\"badge\">ESP32-P4 Nano</div>";
  s += "<h1 style=\"margin:8px 0 12px 0\">" + title + "</h1>";
  s += "<nav>";
  s += "<a href=\"/\">Home</a>";
  s += "<a href=\"/info\">Info</a>";
  s += "<a href=\"/control\">Control</a>";
  s += "</nav></div></header><div class=\"wrap\">";
  return s;
}

String htmlFooter() {
  String s;
  s.reserve(200);
  s += "</div><footer>";
  s += "ESP32-P4 Nano Web Server Demo";
  s += "</footer></body></html>";
  return s;
}

void handleRoot() {
  String s = htmlHeader("Home");
  s += "<div class=\"grid\">";
  s += "<div class=\"card\"><h2>Welcome</h2>";
  s += "<p class=\"muted\">This is a simple multi-page web UI running on ESP32-P4.</p>";
  s += "<a class=\"btn\" href=\"/info\">View device info</a></div>";
  s += "<div class=\"card\"><h2>Status</h2>";
  s += "<p>Ethernet: <strong>" + ethernetStatus() + "</strong></p>";
  s += "<p>IP: <strong>" + ethernetIP() + "</strong></p>";
  s += "<p>MAC: <strong>" + ethernetMac() + "</strong></p>";
  s += "</div>";
  s += "<div class=\"card\"><h2>Quick Action</h2>";
  s += "<p class=\"muted\">Toggle demo state from the control page.</p>";
  s += "<a class=\"btn\" href=\"/control\">Open controls</a></div>";
  s += "</div>";
  s += htmlFooter();
  server.send(200, "text/html", s);
}

void handleInfo() {
  String s = htmlHeader("Info");
  s += "<div class=\"grid\">";
  s += "<div class=\"card\"><h2>Device</h2>";
  s += "<p>Chip model: <strong>" + String(ESP.getChipModel()) + "</strong></p>";
  s += "<p>Chip rev: <strong>" + String(ESP.getChipRevision()) + "</strong></p>";
  s += "<p>CPU freq: <strong>" + String(ESP.getCpuFreqMHz()) + " MHz</strong></p>";
  s += "</div>";
  s += "<div class=\"card\"><h2>Memory</h2>";
  s += "<p>Free heap: <strong>" + String(ESP.getFreeHeap()) + " bytes</strong></p>";
  s += "<p>Sketch size: <strong>" + String(ESP.getSketchSize()) + " bytes</strong></p>";
  s += "<p>Flash size: <strong>" + String(ESP.getFlashChipSize()) + " bytes</strong></p>";
  s += "</div>";
  s += "<div class=\"card\"><h2>Network</h2>";
  s += "<p>Status: <strong>" + ethernetStatus() + "</strong></p>";
  s += "<p>MAC: <strong>" + ethernetMac() + "</strong></p>";
  s += "<p>IP: <strong>" + ethernetIP() + "</strong></p>";
  s += "</div>";
  s += "</div>";
  s += htmlFooter();
  server.send(200, "text/html", s);
}

void handleControl() {
  String s = htmlHeader("Control");
  s += "<div class=\"grid\">";
  s += "<div class=\"card\"><h2>Demo Toggle</h2>";
  s += "<p class=\"muted\">Current state:</p>";
  s += "<p><strong>" + String(demoToggle ? "ON" : "OFF") + "</strong></p>";
  s += "<a class=\"btn\" href=\"/toggle\">Toggle</a>";
  s += "</div>";
  s += "<div class=\"card\"><h2>Tips</h2>";
  s += "<ul>";
  s += "<li>Replace the toggle with real GPIO control</li>";
  s += "<li>Use the Info page to verify device data</li>";
  s += "<li>Add authentication for production use</li>";
  s += "</ul>";
  s += "</div>";
  s += "</div>";
  s += htmlFooter();
  server.send(200, "text/html", s);
}

void handleToggle() {
  demoToggle = !demoToggle;
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleNotFound() {
  String s = htmlHeader("404");
  s += "<div class=\"card\"><h2>Page Not Found</h2>";
  s += "<p class=\"muted\">The page you requested does not exist.</p>";
  s += "<a class=\"btn\" href=\"/\">Back Home</a></div>";
  s += htmlFooter();
  server.send(404, "text/html", s);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.onEvent(WiFiEvent);

  Serial.println("Starting Ethernet (IP101)...");
  bool ethStarted = ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO,
                              ETH_PHY_IP101, ETH_CLK_MODE);
  if (!ethStarted) {
    Serial.println("ETH begin failed. Check PHY wiring and configuration.");
  }

  const unsigned long waitStart = millis();
  while (!ethConnected && (millis() - waitStart) < 15000) {
    delay(100);
  }

  if (ethConnected) {
    Serial.print("IP address: ");
    Serial.println(ETH.localIP());
  } else {
    Serial.println("No IP address yet.");
  }

  server.on("/", handleRoot);
  server.on("/info", handleInfo);
  server.on("/control", handleControl);
  server.on("/toggle", handleToggle);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
