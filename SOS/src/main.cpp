#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <M5Core2.h>
#include <MQTTClient.h>
#include <WiFiClientSecure.h>
#include <FastLED.h>
#include "secrets.h"
#include "WiFi.h"
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

#define ADCPIN 36 // Analogue to Digital, Yellow Edukit Cable 
#define DACPIN 26 // Digital to Analogue, White Edukit Cable
#define DHTTYPE DHT11
#define SCALE 3
#define LED_PINS 25
#define NUM_LEDS 10
#define OUTPUT_GAIN 100
// The MQTT topics that this device should publish/subscribe to
#define AWS_IOT_PUBLISH_TOPIC AWS_IOT_PUBLISH_TOPIC_THING
#define AWS_IOT_SUBSCRIBE_TOPIC AWS_IOT_SUBSCRIBE_TOPIC_THING

#define PORT 8883

WiFiClientSecure wifiClient = WiFiClientSecure();
MQTTClient mqttClient = MQTTClient(256);

CRGB leds[NUM_LEDS];
uint8_t hue = 0;
AudioGeneratorWAV *wav;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

long lastMsg = 0;

DHT dht(DACPIN, DHTTYPE);
float temp, humid;

float dht11(String dataType ) {
  float value;

  if (dataType == "temperature") {
    value = dht.readTemperature();
  }
  else {
    value = dht.readHumidity();
  }
  return value;
}

void displayValues(float temp, float humid) {
  Serial.println("Printing readings now:");

  M5.Lcd.setCursor(0,0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("Temperature = %.2fC", temp);

  M5.Lcd.setCursor(0,20);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("Humidity = %.0f", humid);
  Serial.printf("%.2f degrees\n%.0f \n", temp, humid);

  if ( digitalRead(ADCPIN) == HIGH) {
    Serial.println("Water sensor reading: HIGH");
    Serial.println("Water Detected");
    M5.Lcd.setCursor(0,70);
    M5.Lcd.print("Water Reading = HIGH");
    M5.Lcd.setCursor(0,90);
    M5.Lcd.print("Water detected!!!!!!");
  } else {
    Serial.println("Water sensor reading: LOW");
    M5.Lcd.setCursor(0,70);
    M5.Lcd.print("Water Reading = LOW");
    M5.Lcd.setCursor(0,90);
    M5.Lcd.print("No Water detected");
   }
  M5.update(); //Update M5Stack Core2
}

void connectWifi()
{
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(WIFI_SSID);

  // Connect to the specified Wi-Fi network
  // Retries every 500ms
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
}

// Handle message from AWS IoT Core
void messageHandler(String &topic, String &payload)
{
  Serial.println("Incoming: " + topic + " - " + payload);

  // Parse the incoming JSON
  StaticJsonDocument<200> jsonDoc;
  deserializeJson(jsonDoc, payload);

  const bool LED = jsonDoc["LED"].as<bool>();

  // Decide to turn LED on or off
  if (LED) {
    Serial.print("LED STATE: ");
    Serial.println(LED);
    M5.Axp.SetLed(true);
  } else if (!LED) {
    Serial.print("LED_STATE: ");
    Serial.println(LED);
    M5.Axp.SetLed(false);
  }
}

// Connect to the AWS MQTT message broker
void connectAWSIoTCore()
{
  // Create a message handler
  mqttClient.onMessage(messageHandler);

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  wifiClient.setCACert(AWS_CERT_CA);
  wifiClient.setCertificate(AWS_CERT_CRT);
  wifiClient.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on AWS
  Serial.print("Attempting to AWS IoT Core message broker at mqtt:\\\\");
  Serial.print(AWS_IOT_ENDPOINT);
  Serial.print(":");
  Serial.println(PORT);

  // Connect to AWS MQTT message broker
  // Retries every 500ms
  mqttClient.begin(AWS_IOT_ENDPOINT, PORT, wifiClient);
  while (!mqttClient.connect(THINGNAME.c_str())) {
    Serial.print("Failed to connect to AWS IoT Core. Error code = ");
    Serial.print(mqttClient.lastError());
    Serial.println(". Retrying...");
    delay(500);
  }
  Serial.println("Connected to AWS IoT Core!");

  // Subscribe to the topic on AWS IoT
  mqttClient.subscribe(AWS_IOT_SUBSCRIBE_TOPIC.c_str());
}

void rainbowTask(void *pvParameters)
{
  while (1)
  {
    FastLED.showColor(CHSV(hue++, 255, 255));
  }
}

void audioTask()
{
  while (1)
  {
    if (wav->isRunning())
    {
      if (!wav->loop())
      {
        wav->stop();
      }
    }
    else
    {
      wav->stop();
      break;
    }
  }
}

void initialiseAudio()
{
  // Enable speaker
  M5.Axp.SetSpkEnable(true);

  // Initialise SD card slot
  if (!SD.begin())
  {
    Serial.println("SD card failed to mount or not present");
    while (1)
      ;
  }
  M5.Lcd.println("SD card initialised");

  if (SD.exists("/HumidityAlert.wav"))
  {
    Serial.println("HumidityAlert.wav exists");
  }
  else
  {
    Serial.println("HumidityAlert.wav doesn't exist");
  }

  // Load wav file
  file = new AudioFileSourceSD("/HumidityAlert.wav");
  id3 = new AudioFileSourceID3(file);
  out = new AudioOutputI2S(0, 0);
  out->SetPinout(12, 0, 2);
  out->SetOutputModeMono(true);
  out->SetGain((float)OUTPUT_GAIN / 100.0);
  wav = new AudioGeneratorWAV();
  wav->begin(id3, out);
}

void setup() {
  // put your setup code here, to run once:
  bool LCDEnable = true;
  bool SDEnable = true; 
  bool SerialEnable = true;
  bool I2CEnable = true;
  pinMode(DACPIN, INPUT); // Set pin 26 to input mode.
  pinMode(ADCPIN, INPUT); // Set pin 36 to input mode.
  M5.begin(LCDEnable, SDEnable, SerialEnable, I2CEnable); // Init M5Core2.
  dht.begin();
  Serial.begin(115200);
  Serial.println(F("Testing SOS sensor"));
  M5.Lcd.print("Device setup");
  M5.Lcd.clear();
  M5.Axp.SetLed(false);
  initialiseAudio();

  // Specify LED pins
  FastLED.addLeds<SK6812, LED_PINS>(leds, NUM_LEDS);

  connectWifi();
  connectAWSIoTCore();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1500);
  temp = dht11("temperature");
  humid = dht11("humidity");

  Serial.printf("%.2f degrees\n%.0f \n", temp, humid);

  if (isnan(temp) || isnan(humid)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    M5.Lcd.clear(); 
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("Unable to fetch temp/humidity readings, trying again");
    M5.update(); //Update M5Stack Core2
    return;
  }
  else {
    M5.Lcd.clear(); 
  }

  delay(500);
  displayValues(temp, humid);
  
  if (humid > 45) {
    audioTask();
    initialiseAudio();
  }
  
  // MQTT
  // Reconnection Code if disconnected from the MQTT Client/Broker
  if (!mqttClient.connected()) {
    Serial.println("Device has disconnected from MQTT Broker, reconnecting...");
    connectAWSIoTCore();
  }
  mqttClient.loop();

  long now = millis();
  if (now - lastMsg > 30000) {
    lastMsg = now;
    // Initialise json object and print
    StaticJsonDocument<200> jsonDoc;
    char jsonBuffer[512];

    JsonObject thingObject = jsonDoc.createNestedObject("body");
    thingObject["id"] = "b0b3e11b-01c4-444d-ad5b-073f6fe6f43b";
    thingObject["type"] = "SMS";
    thingObject["message"] = "Alert: Humidity over 45%! High Chance of Mould! Please check!";
    thingObject["temp"] = temp;
    thingObject["humidity"] = humid;

    serializeJsonPretty(jsonDoc, jsonBuffer);
    Serial.println("");
    Serial.print("Publishing to " + AWS_IOT_PUBLISH_TOPIC + ": ");
    Serial.println(jsonBuffer);

    // M5.Lcd.clear();
    M5.Lcd.printf("Message has been sent at %d", millis());

    // Publish json to AWS IoT Core
    mqttClient.publish(AWS_IOT_PUBLISH_TOPIC.c_str(), jsonBuffer);
  }
}