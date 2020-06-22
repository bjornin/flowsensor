
#include <Arduino.h>
#include <ReactESP.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifndef APSSID
#define APSSID "esp_fuel"
#define APPSK  "bensin"
#endif

#define INPUT_PIN 2

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);
volatile int count;
volatile int frequency;

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  char content[50];
  sprintf(content, "<h1>Frequency: %d</h1>", frequency);
  server.send(200, "text/html", content);
}

void setup_network() {
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void IncreaseCount() {
  count++;
}

void GetCount() {
  noInterrupts();
  frequency = count;
  count = 0;
  interrupts();
  Serial.println(frequency); // Debug print instead
}

ReactESP app([] () {
  Serial.begin(115200);
  Serial.println();
  setup_network();
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
  app.onRepeat(1, [](){
    server.handleClient();
  });
  app.onInterrupt(INPUT_PIN, RISING, IncreaseCount);
  app.onRepeat(1000, GetCount);
});
