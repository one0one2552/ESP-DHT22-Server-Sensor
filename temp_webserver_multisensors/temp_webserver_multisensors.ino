#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <vector>
#include <map> // Für die Speicherung der Daten nach IP-Adresse

// WLAN-Details für den Wemos D1 Mini
const char* ssid = "MYSSID";
const char* password = "MYPASSWORD";
ESP8266WebServer server(80); // Webserver auf Port 80


float temperatur = 0.0;
float feuchtigkeit = 0.0;
String latestLog = "";
extern unsigned long _lastUpdate; // Möglicherweise der interne Zeitstempel

String formatTimestamp(long timestamp) {
    time_t rawtime = timestamp + 7200; // Offset für GMT+2 (Sommerzeit in Bern)
    struct tm * timeinfo = localtime(&rawtime);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}

struct SensorData {
    long timestamp;
    float temperature;
    float humidity;
};

std::map<String, std::vector<SensorData>> historicalDataByIP;
std::vector<SensorData> historicalData;
const size_t maxDataPoints = 48; // Für 24 Stunden alle 2 Stunden

// NTP Client initialisieren (Bern, GMT+2, Update alle Stunde)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 3600000);

void setup() {
    Serial.begin(115200);
    Serial.println("\nESP8266 Webserver gestartet");

    // Statische IP-Konfiguration
    IPAddress staticIP(192, 168, 1, 50);
    IPAddress gateway(192, 168, 1, 1);   // Deine Router-IP-Adresse
    IPAddress subnet(255, 255, 255, 0);  // Dein Subnetz
    IPAddress dns(192, 168, 1, 1);       // Dein DNS-Server (oft die Router-IP)

    WiFi.config(staticIP, gateway, subnet, dns);

    WiFi.begin(ssid, password);
    Serial.print("Verbinde mit WLAN...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nVerbunden mit WLAN");
    Serial.print("IP-Adresse: ");
    Serial.println(WiFi.localIP());

    timeClient.begin();
    timeClient.update(); // Sofortige Aktualisierung der Zeit

    server.on("/", handleRoot);
    server.on("/log", handleLog);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("Webserver gestartet");
}



void handleLog() {
    String clientIP = server.client().remoteIP().toString();
    Serial.printf("Daten empfangen von IP: %s\n", clientIP.c_str());

    float temp = server.arg("temperatur").toFloat();
    float hum = server.arg("feuchtigkeit").toFloat();
    String log = server.arg("log");

    // Aktuelle Werte immer aktualisieren (vielleicht mit einer Kennung?)
    // Für den Moment überschreiben wir die globalen Variablen
    temperatur = temp;
    feuchtigkeit = hum;
    latestLog = log;

    timeClient.update();
    unsigned long currentEpoch = timeClient.getEpochTime();

    // Speichere die historischen Daten basierend auf der IP-Adresse
    if (historicalDataByIP.find(clientIP) == historicalDataByIP.end()) {
        historicalDataByIP[clientIP] = std::vector<SensorData>();
    }
    if (historicalDataByIP[clientIP].size() >= maxDataPoints) {
        historicalDataByIP[clientIP].erase(historicalDataByIP[clientIP].begin());
    }
    historicalDataByIP[clientIP].push_back({currentEpoch, temp, hum});
    Serial.printf("Daten von %s geloggt um: %s (Epoch: %lu)\n", clientIP.c_str(), timeClient.getFormattedTime().c_str(), currentEpoch);

    server.send(200, "text/plain", "Daten und Log empfangen");
    Serial.printf("Temp: %.2f °C, Hum: %.2f %%, Log: %s von %s\n", temp, hum, log.c_str(), clientIP.c_str());
}

void handleRoot() {
    String html = "<!DOCTYPE html><html><head><title>DHT22 Daten und Log</title><meta http-equiv='refresh' content='5'></head><body>";
    html += "<h1>Aktuelle Sensorwerte</h1>";
    html += "<p>Temperatur: <strong>" + String(temperatur) + "</strong> &deg;C</p>";
    html += "<p>Luftfeuchtigkeit: <strong>" + String(feuchtigkeit) + "</strong> %</p>";
    html += "<hr><h2>Letzter Log vom ESP-01:</h2><pre>";
    html += latestLog;
    html += "</pre><hr><h2>Letzte Messwerte pro ESP:</h2>";

    if (historicalDataByIP.empty()) {
        html += "<p>Noch keine historischen Daten von Clients vorhanden.</p>";
    } else {
        for (auto const& [ip, dataPoints] : historicalDataByIP) {
            html += "<h3>Daten von ESP-01 mit IP: " + ip + "</h3><pre>";
            if (dataPoints.empty()) {
                html += "Noch keine historischen Daten von diesem ESP vorhanden.";
            } else {
                timeClient.setTimeOffset(7200);
                for (int i = dataPoints.size() - 1; i >= 0; --i) {
                    String formattedTime = formatTimestamp(dataPoints[i].timestamp);
                    html += formattedTime + " - Temp: " + String(dataPoints[i].temperature) + " &deg;C, Hum: " + String(dataPoints[i].humidity) + " %\n";
                }
            }
            html += "</pre><hr>";
        }
    }

    html += "</body></html>";
    server.send(200, "text/html", html);
}



void handleNotFound() {
    server.send(404, "text/plain", "Seite nicht gefunden");
}

void loop() {
    server.handleClient();
    timeClient.update();
}