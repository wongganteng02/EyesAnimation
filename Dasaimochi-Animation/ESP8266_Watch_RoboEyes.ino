
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>


// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#include <FluxGarage_RoboEyes.h>
roboEyes roboEyes;
// WiFi
const char* ssid = "JAHID";
const char* password = "Deauther";

// RoboEyes


// Time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 21600, 60000);

// Temp
float temperature = 0.0;

// Button Setup
#define BUTTON_PIN D5
bool isClockMode = false;
bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    display.print(".");
    display.display();
    retry++;
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi Connected!");
  } else {
    display.println("WiFi Failed!");
  }
  display.display();

  timeClient.begin();
  timeClient.update();
  getTemperature();

  // RoboEyes Setup
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
}

void loop() {
  timeClient.update();
  checkButton();
  if (isClockMode) {
    showClock();
  } else {
    roboEyes.update();
  }
}

// Button Check Function (fixed)
void checkButton() {
  if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
    buttonPressed = true;
    lastDebounceTime = millis();
  }

  if (buttonPressed && (millis() - lastDebounceTime) > debounceDelay) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      isClockMode = !isClockMode;
      Serial.print("Mode toggled: ");
      Serial.println(isClockMode ? "Clock" : "Eyes");
      display.clearDisplay();
    }
    buttonPressed = false;
  }
}

void showClock() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(timeClient.getFormattedTime());

  time_t rawTime = timeClient.getEpochTime();
  struct tm* ti = localtime(&rawTime);
  char dateBuf[12];
  sprintf(dateBuf, "%02d-%02d-%04d", ti->tm_mday, ti->tm_mon + 1, ti->tm_year + 1900);

  display.setTextSize(1);
  display.setCursor(0, 30);
  display.print("Date: ");
  display.println(dateBuf);

  display.setCursor(0, 45);
  display.print("Temp: ");
  display.print(temperature, 1);
  display.println(" C");

  display.display();
}

void getTemperature() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://api.open-meteo.com/v1/forecast?latitude=23.8103&longitude=90.4125&current_weather=true");
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        temperature = doc["current_weather"]["temperature"].as<float>();
      } else {
        Serial.println("JSON parse error");
        temperature = 0.0;
      }
    } else {
      Serial.println("Temp HTTP failed");
    }
    http.end();
  }
}
