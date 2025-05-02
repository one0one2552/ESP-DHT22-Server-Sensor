#include <ESP8266WiFi.h>
#include <ArduinoHttpClient.h>
#include <DHT.h>
#include <ESP8266WiFiMulti.h> // Für robustere WLAN-Verbindungen

// WLAN-Details (werden vom ESP8266WiFiMulti verwaltet)
ESP8266WiFiMulti wifiMulti;

// Webserver-Adresse und Port
const char* serverIP = "192.168.1.50";
const int serverPort = 80;
const String apiPath = "/log";

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Sleep-Dauer in Mikrosekunden (30 Minuten)
const uint64_t sleepTimeUs = 1800000000;

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP-01 Booting (Light Sleep Mode)");

  // WLAN-Konfiguration für ESP8266WiFiMulti
  wifiMulti.addAP("MYSSID", "MYPASSWORD");
  // Du kannst hier weitere WLAN-Netzwerke hinzufügen

  Serial.print("Verbinde mit WLAN");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("\nWLAN verbunden");
    Serial.print("IP-Adresse: ");
    Serial.println(WiFi.localIP());

    dht.begin();
    delay(2000); // Wartezeit für DHT22

    // Aktiviere den Light Sleep Modus
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  } else {
    Serial.println("\nWLAN Verbindung fehlgeschlagen!");
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      WiFiClient client;
      HttpClient http(client, serverIP, serverPort);
      String serverPathWithParams = String(apiPath) + "?temperatur=" + String(t) + "&feuchtigkeit=" + String(h);

      Serial.print("Sende Daten an: ");
      Serial.println(serverPathWithParams);

      http.get(serverPathWithParams.c_str());
      long httpResponseCode = http.responseStatusCode();
      Serial.print("HTTP Antwort-Code: ");
      Serial.println(httpResponseCode);
      String response = http.readString();
      Serial.println("Antwort vom Server:");
      Serial.println(response);
      http.stop();

      Serial.printf("Gehe in den Light Sleep für %d Minuten...\n", (int)(sleepTimeUs / 1000000 / 60));
      delay(sleepTimeUs / 1000); // Wartezeit, bevor der ESP in den Sleep geht
      WiFi.forceSleepBegin();
      delay(100);
      WiFi.forceSleepWake(); // Korrigierte Funktion zum Aufwachen
      delay(100);
      // Nach dem Aufwachen muss sich der ESP neu verbinden.
      // Die loop() Funktion wird erneut ausgeführt.
    } else {
      Serial.println("Fehler beim Lesen der Sensorwerte.");
      delay(sleepTimeUs / 1000); // Wartezeit vor erneutem Versuch
    }
  } else {
    Serial.println("WLAN nicht verbunden, versuche erneut...");
    delay(5000);
  }
}