#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <M5Core2.h>

#define ADCPIN 36 // Analogue to Digital, Yellow Edukit Cable 
#define DACPIN 26 // Digital to Analogue, White Edukit Cable
#define DHTTYPE DHT11

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

  M5.Lcd.setCursor(0,50);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("Humidity = %.0f", humid);
  Serial.printf("%.2f degrees\n%.0f \n", temp, humid);

  if ( digitalRead(ADCPIN) == HIGH) {
    M5.Lcd.clear();
    Serial.println("Water sensor reading: HIGH");
    Serial.println("Water Detected");
    M5.Lcd.setCursor(0,60);
    M5.Lcd.print("Water Reading = HIGH");
    M5.Lcd.setCursor(0,70);
    M5.Lcd.print("Water detected!!!!!!");
  } else {
    M5.Lcd.clear();
    Serial.println("Water sensor reading: LOW");
    M5.Lcd.setCursor(0,60);
    M5.Lcd.print("Water Reading = LOW");
    M5.Lcd.setCursor(0,70);
    M5.Lcd.print("No Water detected");
   }
  M5.update(); //Update M5Stack Core2
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
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
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
}