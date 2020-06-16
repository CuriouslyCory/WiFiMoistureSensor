#include <FS.h>                   //this needs to be first, or it all crashes and burns...

// All of the following includes are for WIFI
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager

// MQTT IOT Includes
#include <PubSubClient.h>       // https://pubsubclient.knolleary.net/

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

// MQTT config
#define MQTT_SERV "192.168.50.167"
#define MQTT_PORT 1883
#define MQTT_NAME "mqttuser"
#define MQTT_PASS ""
#define MQTT_MOIST_TOPIC "stat/%s/moistureval"
#define MQTT_MOISTPCT_TOPIC "stat/%s/moisturepct"

WiFiClient client;
PubSubClient pubclient(MQTT_SERV, MQTT_PORT, client);

char topic[100];
int soilMoistureValue = 0;
int soilMoisturePct = 0;
int currentMillis = 0;
char mqtt_id[40];
int air_value = 850;
int water_value = 450;
int refresh_seconds = 3600;

//flag for saving data
bool shouldSaveConfig = false;

void setup() {
  currentMillis = millis();
  // put your setup code here, to run once:
  Serial.begin(115200); // Starts the serial communication

  // load stored settings from filesystem
  loadFromFS();

  // connect to wifi
  connectWifi();

  // connect to mqtt server
  MQTT_connect();

  // print sensor values to serial
  printSenseVal();

  // publish sensor values to mqtt
  publishSenseVal();
  
  //the publish commands actually run async, so let's give them a few seconds to resolve before going to sleep.
  // 10 second wait
  delay(10 * 1000);

  
  //deep sleep for however long is configured, subtract current run time to get as close to a regular interval as possible. 
  // this method can still result in some drift, having a more crontab style schedule may provide more stable results.
  Serial.println("Triggered event, going to sleep");
  long refresh_microseconds = refresh_seconds * 1000 * 1000;
  Serial.print("Sleep for: ");
  Serial.println(refresh_microseconds - ((millis() - currentMillis) * 1000));
  ESP.deepSleep(refresh_microseconds - ((millis() - currentMillis) * 1000));  
}

void loop() {
}

void connectWifi() {
  // Connect to WiFi
  Serial.print("\n\nConnecting Wifi... ");
  WiFiManager wifiManager;
  char temp[6];

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_mqtt_id("mqtt_id", "MQTT ID", mqtt_id, 40);
  wifiManager.addParameter(&custom_mqtt_id);

  // id/name, placeholder/prompt, default, length
  itoa(air_value, temp, 10);
  WiFiManagerParameter custom_air_value("air_value", "Air Value", temp, 4);
  wifiManager.addParameter(&custom_air_value);

  // id/name, placeholder/prompt, default, length
  itoa(water_value, temp, 10);
  WiFiManagerParameter custom_water_value("water_value", "Water Value", temp, 4);
  wifiManager.addParameter(&custom_water_value);

  // id/name, placeholder/prompt, default, length
  itoa(refresh_seconds, temp, 10);
  WiFiManagerParameter custom_refresh_seconds("refresh_seconds", "Refresh Seconds", temp, 6);
  wifiManager.addParameter(&custom_refresh_seconds);
  
  // uncomment the next line to clear all saved settings
  //resetWifi(wifiManager);
  wifiManager.autoConnect("MoistSensorSetup");

  // Connected
  Serial.print("My Wifi IP is: ");
  Serial.println(WiFi.localIP());
  
  if (shouldSaveConfig) {
    strcpy(mqtt_id, custom_mqtt_id.getValue());
  
    strcpy(temp, custom_air_value.getValue());
    air_value = atoi(temp);
    Serial.print("Air value: ");
    Serial.println(air_value);
  
    strcpy(temp, custom_water_value.getValue());
    water_value = atoi(temp);
    Serial.print("Water value: ");
    Serial.println(water_value);

    strcpy(temp, custom_refresh_seconds.getValue());
    refresh_seconds = atoi(temp);
    Serial.print("Refresh Seconds: ");
    Serial.println(refresh_seconds);
    saveToFS();
  }
}

void printSenseVal() {
  soilMoistureValue = analogRead(A0);
  Serial.print("Soil Moisture Value:");
  Serial.println(soilMoistureValue);

  soilMoisturePct = map(soilMoistureValue, air_value, water_value, 0, 100);
  Serial.print("Soil Moisture Percentage: ");
  Serial.println(soilMoisturePct);
}

void publishSenseVal() {
  // read current value from sensor
  soilMoistureValue = analogRead(A0);

  // calculate percentage from base values
  soilMoisturePct = map(soilMoistureValue, air_value, water_value, 0, 100);

  // prep data into character arrays for publishing
  char soilMoistureValueAry[5];
  char soilMoisturePctAry[5];
  itoa(soilMoistureValue, soilMoistureValueAry, 10);
  itoa(soilMoisturePct, soilMoisturePctAry, 10);

  // publish moisture value
  sprintf(topic, MQTT_MOIST_TOPIC, mqtt_id);
  Serial.print("Publishing moisture value to: ");
  Serial.println(topic);
  pubclient.publish(topic, soilMoistureValueAry);

  // publish moisture percent
  sprintf(topic, MQTT_MOISTPCT_TOPIC, mqtt_id);
  Serial.print("Publishing to: ");
  Serial.println(topic);
  pubclient.publish(topic, soilMoisturePctAry);

}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void MQTT_connect() 
{
  // Stop if already connected.
  if (pubclient.connected()) 
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  // Generate client name based on MAC address and last 8 bits of microsecond counter
  String clientName;
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);

  Serial.print("Connecting to ");
  Serial.print(MQTT_SERV);
  Serial.print(" as ");
  Serial.println(clientName);
  
  if (pubclient.connect((char*) clientName.c_str(), MQTT_NAME, MQTT_PASS)) {
    Serial.println("Connected to MQTT broker");
  }
  else {
    Serial.println("MQTT connect failed");
    Serial.println("Will reset and try again...");
    abort();
  }

  Serial.println("MQTT Connected!");
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void resetWifi(WiFiManager &wifiManager) {
  // clean FS, for testing
  //SPIFFS.format();
  // reset wifi settings
  wifiManager.resetSettings();
}

void loadFromFS() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        const size_t CAPACITY = 1024;
        StaticJsonDocument<CAPACITY> doc;
        
        // deserialize the object
        deserializeJson(doc, buf.get(), size);
        
        // extract the data
        JsonObject json = doc.as<JsonObject>();
        
        serializeJson(doc, Serial);
        if (!doc.isNull()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_id, json["mqtt_id"]);
          air_value = (int) json["air_value"];
          water_value = (int) json["water_value"];
          refresh_seconds = (int) json["refresh_seconds"];

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}

void saveToFS() {
  Serial.println("saving config");
  const size_t CAPACITY = 1024;
  StaticJsonDocument<CAPACITY> doc;
  
  // create an object
  JsonObject json = doc.to<JsonObject>();

  char data[6];
  json["mqtt_id"] = mqtt_id;

  itoa(air_value, data, 10);
  json["air_value"] = data;

  itoa(water_value, data, 10);
  json["water_value"] = data;

  itoa(refresh_seconds, data, 10);
  json["refresh_seconds"] = data;

  // serialize the object and send the result to Serial
  serializeJson(json, Serial);

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJson(json, Serial);
  serializeJson(json, configFile);
  configFile.close();
  //end save
}

String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c +='0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}
