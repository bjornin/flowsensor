
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ReactESP.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <vector>

#ifndef APSSID
#define APSSID "esp_fuel"
#define APPSK  "bensin"
#endif

#define INPUT_PIN 2
#define PULSES_PER_LITRE 2500

extern ReactESP app;

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);
volatile int count;
volatile int frequency;
int avg_pulses = 0; 
int total_pulses = 0;
const int num_samples = 5;
std::vector<int> samples;
int ptr = 0;
float lph;
float total_l;

const char html[] = R"html(
<html>
<head>
<meta http-equiv="refresh" content="5">
</head>
<body style="font-size:10vw">
<p>Current: %d.%02d l/h
<p>Total: %d.%02d l
</body>
</html>
)html";

void HandleRoot() {
  char content[175];
  sprintf(content, html, (int)lph, (int)(lph*100)%100, (int)total_l, (int)(total_l*100)%100);
  server.send(200, "text/html", content);
}

void SetupNetwork() {
  Serial.print("Configuring access point...");
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void IncreaseCount() {
  count++;
}

void UpdateValues(unsigned int sample) {
  total_pulses += sample;
  avg_pulses -= samples[ptr]/num_samples;
  avg_pulses += sample/num_samples;
  samples[ptr] = sample;
  ptr = (ptr+1)%num_samples;
  lph = (float)avg_pulses*3600/PULSES_PER_LITRE;
  total_l = (float)total_pulses/PULSES_PER_LITRE;
}

void GetCount() {
  noInterrupts();
  frequency = count;
  count = 0;
  interrupts();
  UpdateValues(frequency);
}

void PrintInfo() {
  char buf[50];
  sprintf(buf, "Current: %d.%02d l/h", (int)lph, (int)(lph*100)%100);
  Serial.println(buf);
  sprintf(buf, "Total: %d.%02d l", (int)total_l, (int)(total_l*100)%100);
  Serial.println(buf);
}

void SetupOTA() {
  ArduinoOTA.onStart([](){
    Serial.println("OTA start");
  });
  ArduinoOTA.onEnd([](){
    Serial.println("OTA end");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total){
    char buf[50];
    sprintf(buf, "OTA progress: %u%%\r", (progress / total) * 100);
    Serial.println(buf);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    char buf[50];
    sprintf(buf, "OTA error: %u", error);
    Serial.println(buf);
  });
  ArduinoOTA.begin();
  app.onRepeat(20, [](){
    ArduinoOTA.handle();
  });
}

ReactESP app([] () {
  Serial.begin(115200);
  Serial.println();
  SetupNetwork();
  SetupOTA();
  server.on("/", HandleRoot);
  server.begin();
  Serial.println("HTTP server started");
  samples.resize(num_samples, 0);
  app.onRepeat(1, [](){
    server.handleClient();
  });
  app.onInterrupt(INPUT_PIN, RISING, IncreaseCount);
  app.onRepeat(1000, GetCount);
  app.onRepeat(5000, PrintInfo);
});
