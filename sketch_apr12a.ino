#include <BLEDevice.h>  
#include <BLEUtils.h>  
#include <BLEScan.h>  
#include <BLEAdvertisedDevice.h>  
#include <WiFi.h>  
#include <HTTPClient.h> 
#include <FS.h>
#include <SPIFFS.h>
#include <YAMLEmitter.h>
#include <YAML.h>

const char* filePath = "/config.yaml";
char* ssid;
char* password;   
const char* serverUrl = "http://markfojz.beget.tech/index.php";  
String knownBLEAddresses[] = {"ce:d2:54:1d:2d:50","f7:b3:04:55:1c:99", "d4:23:60:af:08:59", "c8:54:d3:0b:27:e2", "c1:91:d2:fe:e7:cd", "d3:0e:5f:22:d8:87", "ce:b8:7e:9b:e3:3c", "f8:55:3b:56:cc:62", "c3:c1:79:f8:2b:d6" , "d9:35:12:7f:ab:ba", "d3:10:d8:3c:ff:40", "f3:ac:99:c6:d9:5f", "dd:72:bc:53:bc:25", "f1:7b:0d:38:d0:19", "f5:d2:2b:94:55:ec", "fa:0c:74:b2:ab:71", "f3:d1:eb:71:c1:c2", "de:7a:da:39:5b:3e"};  
const float Q = 0.125; // Process noise  
const float R = 32; // Measurement noise  
float P = 1023;  
float current_estimate = 0;  
float last_estimate = 0;  
BLEScan* pBLEScan;  

void loadConfig() {
  File configFile = SPIFFS.open(filePath, "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println("Failed to parse config file");
    return;
  }

  ssid = doc["wifi"]["ssid"];
  password = doc["wifi"]["password"];

  configFile.close();
}

  
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {  
 void onResult(BLEAdvertisedDevice advertisedDevice) {  
 for (int i=0;i<(sizeof(knownBLEAddresses)/sizeof(knownBLEAddresses[0]));i++){  
 if (strcmp(advertisedDevice.getAddress().toString().c_str(), knownBLEAddresses[i].c_str()) == 0) {  
 int current_RSSI = advertisedDevice.getRSSI();  
 Serial.println(current_RSSI);  
 float prediction = last_estimate;  
 float prediction_error = P + Q;  
  
// Update  
 float K = prediction_error / (prediction_error + R);  
 current_estimate = prediction + K * (current_RSSI - prediction);  
 P = (1 - K) * prediction_error;  
  
 last_estimate = current_estimate;  
 Serial.println(current_estimate);  
 HTTPClient http;  
 WiFiClient client;  
 bool beginSuccess = http.begin(client, serverUrl);  
 if (!beginSuccess) {  
 Serial.println("Failed to establish connection to the server");  
 }  
 http.addHeader("Content-Type", "application/json");  
 http.setUserAgent(F("Mozilla/5.0"));  
 String RSSI = "";  
 RSSI += current_RSSI;  
 String payload = "{\"key1\":" + String(current_RSSI) + ",\"key2\":" + String(26.0) + ",\"key3\":" + String(50.0) + ",\"key4\":" + String(i+1) + "}";  
 Serial.println(payload);  
 int httpCode = http.POST(payload);  
 Serial.println(httpCode);    
 break;  
 }  
}  
  
  
 }  
};  
  
  
  
void setup() {  
 Serial.begin(9600);  
 if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  loadConfig();
 WiFi.begin(ssid, password);  
 while (WiFi.status() != WL_CONNECTED) {  
 delay(1000);  
 }  
  
 BLEDevice::init("");  
 pBLEScan = BLEDevice::getScan();  
 pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(),true);  
 pBLEScan->setActiveScan(true);  
 pBLEScan->setInterval(100);  
 pBLEScan->setWindow(99);  
}  
  
void loop() {  
 delay(100);  
 pBLEScan->start(0);  
}