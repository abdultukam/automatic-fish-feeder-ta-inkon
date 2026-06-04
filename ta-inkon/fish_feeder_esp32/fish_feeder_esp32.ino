#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include "time.h"

// --- WIFI ---
const char* ssid     = "Oppo promax";
const char* password = "12345678";

// --- YOUR LAPTOP SERVER ---
// Expose online with: ssh -R 80:localhost:3000 nokey@localhost.run or npx ngrok http 3000
const char* serverIP = "https://capsule-abstain-fanfare.ngrok-free.dev";

// --- TIME (WIB = GMT+7) ---
const char* ntpServer    = "pool.ntp.org";
const long  gmtOffset    = 7 * 3600;
const int   dstOffset    = 0;

// --- HARDWARE ---
#define SERVO_PIN 13
Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- STATE ---
int  feedAngle    = 45;
bool isFeeding    = false;
bool servoMoved   = false;
unsigned long feedStart = 0;
int  lastFeedMin  = -1;

// ----- HELPERS -----

String getTime() {
  struct tm t;
  if (!getLocalTime(&t)) return "??:??:??";
  char buf[10];
  strftime(buf, sizeof(buf), "%H:%M:%S", &t);
  return String(buf);
}

void setLCD(String l1, String l2) {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(l1.substring(0, 16));
  lcd.setCursor(0, 1); lcd.print(l2.substring(0, 16));
}

// Tell the laptop server what just happened
void notifyServer(String event, int angle = 0) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[notifyServer] WiFi not connected");
    return;
  }
  
  String url = String(serverIP) + "/event?type=" + event + "&angle=" + String(angle) + "&time=" + getTime();
  HTTPClient http;
  int code = -1;
  
  Serial.println("[notifyServer] Sending: " + url);
  
  if (url.startsWith("https://")) {
    WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, url);
    http.addHeader("ngrok-skip-browser-warning", "true");
    code = http.GET();
    http.end();
  } else {
    WiFiClient client;
    http.begin(client, url);
    http.addHeader("ngrok-skip-browser-warning", "true");
    code = http.GET();
    http.end();
  }
  
  Serial.printf("[notifyServer] Response Code: %d\n", code);
}

// Poll the laptop server for any pending commands
void pollCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[pollCommands] WiFi not connected");
    return;
  }
  
  String url = String(serverIP) + "/command";
  HTTPClient http;
  int code = -1;
  String body = "";
  
  if (url.startsWith("https://")) {
    WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, url);
    http.addHeader("ngrok-skip-browser-warning", "true");
    code = http.GET();
    if (code == 200) {
      body = http.getString();
    }
    http.end();
  } else {
    WiFiClient client;
    http.begin(client, url);
    http.addHeader("ngrok-skip-browser-warning", "true");
    code = http.GET();
    if (code == 200) {
      body = http.getString();
    }
    http.end();
  }
  
  if (code == 200) {
    if (body.startsWith("FEED:")) {
      feedAngle = body.substring(5).toInt();
      Serial.printf("[pollCommands] Received command: %s\n", body.c_str());
      if (!isFeeding) {
        isFeeding   = true;
        servoMoved  = false;
        feedStart   = millis();
        notifyServer("feeding_start", feedAngle);
        setLCD("FEEDING...", "Angle: " + String(feedAngle));
      }
    }
  } else {
    Serial.printf("[pollCommands] Error connecting to server: %d\n", code);
  }
}

// ----- SETUP -----
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  setLCD("Booting...", "");

  myServo.attach(SERVO_PIN);
  myServo.write(0);
  delay(300);

  WiFi.begin(ssid, password);
  setLCD("Connecting WiFi", ssid);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nIP: " + WiFi.localIP().toString());
  setLCD("WiFi OK", WiFi.localIP().toString());
  delay(1500);

  configTime(gmtOffset, dstOffset, ntpServer);
  notifyServer("boot");
  setLCD("Ready", "09 13 20 WIB");
}

// ----- LOOP -----
void loop() {
  // 1. Handle active feeding (non-blocking sweep)
  if (isFeeding) {
    if (!servoMoved) {
      feedStart  = millis();
      servoMoved = true;
    }
    
    unsigned long elapsed = millis() - feedStart;
    if (elapsed < 3000) {
      int angle = (elapsed * feedAngle) / 3000;
      myServo.write(angle);
      delay(20); // Smooth sweep update rate
    } else {
      myServo.write(0);
      isFeeding  = false;
      servoMoved = false;
      notifyServer("feeding_done", feedAngle);
      setLCD("Done!", getTime());
    }
    return; // Skip polling while feeding
  }

  // 2. Check for remote commands every 2 seconds
  static unsigned long lastPoll = 0;
  if (millis() - lastPoll >= 2000) {
    pollCommands();
    lastPoll = millis();
  }

  // 3. Automatic schedule: 09:00, 13:00, 20:00
  struct tm t;
  if (getLocalTime(&t)) {
    bool isScheduled = (t.tm_min == 0) &&
                       (t.tm_hour == 9 || t.tm_hour == 13 || t.tm_hour == 20);
    if (isScheduled && lastFeedMin != t.tm_min) {
      feedAngle  = 45;
      isFeeding  = true;
      servoMoved = false;
      feedStart  = millis();
      lastFeedMin = t.tm_min;
      notifyServer("schedule", feedAngle);
      setLCD("Auto Feeding", getTime());
    }
    if (t.tm_min != 0) lastFeedMin = -1;

    // 4. LCD idle display (only update if time changes to avoid flickering)
    static String lastLCDTime = "";
    if (!isFeeding) {
      String currentTime = getTime();
      if (currentTime != lastLCDTime) {
        setLCD("Time: " + currentTime, "Next:09,13,20WIB");
        lastLCDTime = currentTime;
      }
    }
  }

  delay(100);
}
