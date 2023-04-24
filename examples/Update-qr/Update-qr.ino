#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Update.h>
#include <Ticker.h>
#include "global_vars.h"
#include "html.h"
#include "favicon_ico.h"
#include "favicon_svg.h"

// If you exceed the lengths on SSID and PASSWORD, they will be truncated
#define SSID_FORMAT "ESP32-%02X%02X%02X" // 12 chars total
//#define PASSWORD "test123456"          // 10 chars total, generate if remarked

#define LED_MODE LED_SSD1306
#define LED_PIN 18 // Used for SINGLE & RGB

#if (LED_MODE == LED_SSD1306)
#include "ssd1306.h"
#endif

DNSServer dnsServer;
WebServer server(80);
Ticker tkSecond;
uint8_t otaDone = 0;
char passwd[11];
bool screenDirty = false;

const char* alphanum = "0123456789!@#$%^&*abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
String generatePass(uint8_t str_len) {
  String buff;
  for(int i = 0; i < str_len; i++)
    buff += alphanum[esp_random() % (strlen(alphanum)-1)];
  return buff;
}

void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info) {
  if (event == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED) {
    otaDone = 1;
  }
  if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
    otaDone = 0;
  }
#if (LED_MODE == LED_SSD1306)
  show_qrcode();
#endif
}

void apMode() {
  WiFi.mode(WIFI_AP);
  const IPAddress apIP(8,8,4,4); // Android 8 quirk. Thx Google!
  char ssid[13];
  uint8_t base_mac[6];
  esp_read_mac(base_mac, ESP_MAC_WIFI_STA);
  snprintf(ssid, 13, SSID_FORMAT, base_mac[3], base_mac[4], base_mac[5]);
#ifdef PASSWORD
  snprintf(passwd, 11, PASSWORD);
#else
  snprintf(passwd, 11, "%s", generatePass(10).c_str());
#endif   
  WiFi.softAP(ssid, passwd); // Set up the SoftAP
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0)); // Android 8 quirk
  WiFi.onEvent(WiFiEvent);
  log_i("AP: %s, PASS: %s", ssid, passwd);
}

void handleUpdateEnd() {
  server.sendHeader("Connection", "close");
  if (Update.hasError()) {
    server.send(502, "text/plain", Update.errorString());
  } else {
    server.sendHeader("Refresh", "10");
    server.sendHeader("Location","/");
    server.send(307);
    ESP.restart();
  }
}

void handleUpdate() {
  size_t fsize = UPDATE_SIZE_UNKNOWN;
  if (server.hasArg("size")) fsize = server.arg("size").toInt();
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    log_i("Update: %s, %d", upload.filename.c_str(), upload.totalSize);
    if (!Update.begin(fsize)) {
      otaDone = 0;
      Update.printError(Serial);
     }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    } else {
      otaDone = 100UL * Update.progress() / Update.size();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      log_w("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      log_e("%s", Update.errorString());
      otaDone = 0;
    }
  }
}

void captivePortal() {
    delay(250);

    dnsServer.start(53, "*", WiFi.softAPIP()); // All requests receive AP address

    server.on("/update", HTTP_POST, [](){handleUpdateEnd();}, [](){handleUpdate();});
    server.on("/favicon.ico", HTTP_GET, [](){
       server.sendHeader("Content-Encoding", "gzip");
       server.send_P(200, "image/x-icon", favicon_ico_gz, favicon_ico_gz_len);
    });
    server.on("/favicon.svg", HTTP_GET, [](){server.send(200, "image/svg+xml", favicon_svg);});
     // not found includes root, or whatever the captive portal client asks for
    server.onNotFound([]() {server.send(200, "text/html", indexHtml);});
    server.begin();
}

void everySecond() {
#if (LED_MODE == LED_SINGLE)
#elif (LED_MODE == LED_RGB)
#elif (LED_MODE == LED_SSD1306)
  if (otaDone > 1) show_qrcode();
#endif  
  if (otaDone > 1) log_i("ota: %d", otaDone);  
}

void setup(void) {
  Serial.begin(115200);
  delay(1000);
  apMode();
#if (LED_MODE == LED_SINGLE)
  pinMode(LED_PIN, OUTPUT);
#elif (LED_MODE == LED_RGB)
  pinMode(LED_PIN, OUTPUT);
#elif (LED_MODE == LED_SSD1306)
  Serial.println("oled mode");
  init1306(passwd);
#endif
  captivePortal();
  tkSecond.attach(1, everySecond);
}

void loop(void) {
  dnsServer.processNextRequest();
  server.handleClient();
  delay(150);
}
