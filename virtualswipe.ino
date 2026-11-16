#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- 1. NETWORK CREDENTIALS (WINDOWS HOTSPOT) ---
WiFiMulti wifiMulti;

// --- 2. HARDWARE PINS ---
#define SS_PIN 21
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

#define GREEN_LED 27
#define RED_LED 26
#define BUZZER 25
#define SERVO_PIN 13

#define I2C_SDA 32
#define I2C_SCL 33

Servo doorServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- 3. SECURITY & RATE LIMITING ---
int failedAttempts = 0;
const int MAX_FAILURES = 3;
bool isLockdown = false;

// --- 4. mTLS CERTIFICATES & KEYS ---
const char* root_ca =
"-----BEGIN CERTIFICATE-----\n"
"MIIDDTCCAfWgAwIBAgIUbU5Md8PuIkHt+6PGxqpIYQrOFZEwDQYJKoZIhvcNAQEL\n"
"BQAwFjEUMBIGA1UEAwwLWmVyb1RydXN0Q0EwHhcNMjYwNzE1MTEyNDUxWhcNMjcw\n"
"NzE1MTEyNDUxWjAWMRQwEgYDVQQDDAtaZXJvVHJ1c3RDQTCCASIwDQYJKoZIhvcN\n"
"AQEBBQADggEPADCCAQoCggEBANZNLugWcc1SKA1NBjFccAlqQaFHxk1MuyerVWlY\n"
"snrM536Hz0imrgVy+8Kl83YmD/WcRU0Lv0rrnoatCtIxlRLebUo2CvGPSxplTDN5\n"
"4nwHJCSdvWupZLivOM9ssC8Uktsh3yYmXWxRU9Qpxv4nBThN9IxaVNDfbOy/rW5w\n"
"qGe8zmLzXPSlBH/7W1JtFJfF0aIje/wLSceuZFVfgf1azyeSpAK0ukBCoK2RDJ5I\n"
"WRIM7XYaPcP8zzUpfL/yTkDpFGfk52NRy7aPWjk8P5RqHgHYhmdbs4pUZm7MM8FT\n"
"23PpT3BOJXPVs+DnN9BUtZXzd8C1/DgtuXIFaJSHV/qVIQ8CAwEAAaNTMFEwHQYD\n"
"VR0OBBYEFNJvIZ95jiTXlRjcJBnU2RWO/22SMB8GA1UdIwQYMBaAFNJvIZ95jiTX\n"
"lRjcJBnU2RWO/22SMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEB\n"
"ADm6Trs+laAIiJ5wbfG3wqs4uB20soKOuo6cIRpe9IL6iZHWmnl4z0q0SRApegNr\n"
"Hw3wgKRaK47mGEdAqCGWolBbI5QQsBpkrbnjzeZjj9cWn3MZvTS9MlYuS17mTq4M\n"
"J5/7spBhUzH8slqPtJsZ8FIF7gk/G50pt7EJmpy8ZoIofbZvqdhHqskogX3Tch8k\n"
"1z49lLbo8dD0tdd0M5+2BlVSG8Q7gLs+kDUAkhaXaXftXBXC/wtWbF6b+PejuTsb\n"
"Jmsj/1+SZQsfJlFrj5ujWYk27+wFSVBELf3t4t/p6Pfg+A9vACVFz+JSmnh6Zyuj\n"
"rgjT1dmtb3jjC1scsL2B8lQ=\n"
"-----END CERTIFICATE-----\n";

const char* client_cert =
"-----BEGIN CERTIFICATE-----\n"
"MIIC/TCCAeWgAwIBAgIUcYRamI2yPbcVhlUiuBw4vzPa4lwwDQYJKoZIhvcNAQEL\n"
"BQAwFjEUMBIGA1UEAwwLWmVyb1RydXN0Q0EwHhcNMjYwNzE1MTEyNTE1WhcNMjcw\n"
"NzE1MTEyNTE1WjAXMRUwEwYDVQQDDAxFU1AzMl9EZXZpY2UwggEiMA0GCSqGSIb3\n"
"DQEBAQUAA4IBDwAwggEKAoIBAQCXGTNkf9SnF4+NZYMoL7Z9Ktk2qx1mzWMc5VBA\n"
"E2VkHc5zTnxqx8quFoQOwkyYMR+/AwGmX4BWr34onc7rGIvOl4WBQ3qpG/MbS4qG\n"
"1O9VR3dC61bFEay6JW7QA6JiKH1ujh5BLqzy0p5CquQoh+9FyGwYiptRIU4FmLx0\n"
"apzNg3F13UhOtTfG/hdqRQWASYMgFkEZRdds7WG5feeusrsGd3TNlB4DeDL2U/WS\n"
"lpLklLPZ1WnFWuwF6cFCGwmYrnvAnNTzrwqOq+jAIgg1OODI/KctaLNoEciKcXJ/\n"
"Xpg1wK3wK3nhsi6XypeiaC8UVdu/FpMK687m4QECXTA5ySSHAgMBAAGjQjBAMB0G\n"
"A1UdDgQWBBRW1h7ahtoNA8YPBSu8URPUr6eDaDAfBgNVHSMEGDAWgBTSbyGfeY4k\n"
"15UY3CQZ1NkVjv9tkjANBgkqhkiG9w0BAQsFAAOCAQEATxQrqOyxQN+GBfgau1mT\n"
"NHCOgYLbKa93cgofOAZjaZO7FCo0zQwejP1Z+upuOjteYWYap4Aw+MbC/aQgBsh6\n"
"fiFTA+Y5kKurVBB7oQQBUT6hV+aLdkai9HffKAtjNKGE+rfboRb1ytX/pUoat+0A\n"
"EL6GNDAgiYCti41qE9jjROpjmQV2pZ8jNcQSt+ySUzXWaRzd+HoHxByUAeX+5IF7\n"
"/HwF2uz7kHHpBBag/xnM2MPBCFQQCYnmWtWXDSKnJKfGd2SOPv7jmQd637A5Ki8a\n"
"rnsxg4ThMucdEglmBX2YHD8AXQ2ZAeXfp4LT7yXtOJM1Ko/anXXAFk+2yeDc+v6I\n"
"mQ==\n"
"-----END CERTIFICATE-----\n";

const char* client_key =
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCXGTNkf9SnF4+N\n"
"ZYMoL7Z9Ktk2qx1mzWMc5VBAE2VkHc5zTnxqx8quFoQOwkyYMR+/AwGmX4BWr34o\n"
"nc7rGIvOl4WBQ3qpG/MbS4qG1O9VR3dC61bFEay6JW7QA6JiKH1ujh5BLqzy0p5C\n"
"quQoh+9FyGwYiptRIU4FmLx0apzNg3F13UhOtTfG/hdqRQWASYMgFkEZRdds7WG5\n"
"feeusrsGd3TNlB4DeDL2U/WSlpLklLPZ1WnFWuwF6cFCGwmYrnvAnNTzrwqOq+jA\n"
"Igg1OODI/KctaLNoEciKcXJ/Xpg1wK3wK3nhsi6XypeiaC8UVdu/FpMK687m4QEC\n"
"XTA5ySSHAgMBAAECggEARFWCV5Y0Tykpo/Hf4iHJ70uclsvGbUK/60aOB5x+A0wz\n"
"nRRG0mfu8tJ+AMrApye4SsOKUHrqaMnPTYEpHkuoltMR7sbcx3Q7ewGr8ZSFdNTw\n"
"S9RxFokKy9i4B91M6AWVzJT7e9EnKF42csIkk2v+u1mMAYFWAtgEGCXePOwmZA62\n"
"/uAKOMer0M4/yAxrKzB1tpmDd/BsxRUECpf7w7DHxBfwth0fgyDn4WPQKcCZ0cKO\n"
"8l9QU46YvutuF5dgWARgTD4mLBsWDsKUAj3ns9HxGlbR5eOjBJgydVLJlhWOdGJO\n"
"Yg8Z8BiwPiA5V38noCahm6kQgCAeRVqLw4xMKDnMKQKBgQDOJ0XfB9iV1L5D+r/j\n"
"vAyR/ihfSdqwjyIApI35KlAaY66ACF7QitYcpwpitLYFoqr5Y5mSfaVBhOsIJ7qU\n"
"5v8HZiKEiJNPlXuV0F9DIDoyqEzLth2JEyTKx0SZA8wDUKmyFeH4K4Q1LCE2G6b3\n"
"awHxj9TI8sbmq0nqlqEEO5diDQKBgQC7ohIB/nB0TYcEkgqed1f6N+rx3yOp5eOx\n"
"K39gWbrPX4vgA7exrbtBjdCy8M2u3Knmx8AC0lsRLeTzCHNNi+z3Zr9bXA9aDBuH\n"
"BVZjz7xUUJ/b6LR4w8iByYV+nqfrbqPtwxvx84QcYI8Ig1skIUM/N0rf28yCislx\n"
"HIE+CBw/4wKBgFyWcuEaawHQDKwHYEtt189x8LvrNZb6kfDr9v+jK1rOzj6eg8bx\n"
"s/wmYZm3Q9EAH/3l58FNQP1OLyj4fuoFXSD7lCEmHdMuddTxQveo77p0sBkF7/ny\n"
"aSHRoVN/yw7qGWzsOxL0qGohKcFdZf3VR4gwawbdUsuivo3nnOWDxhe5AoGAMICT\n"
"oRpRnW7RARhzUEVPY6oCvKVOxcNcWVcCJTjstmBfevfbCz36ktaC5U6kDJiT/skR\n"
"1GjmC4nOjhhYHeqWxdk0i/w/J6rLjtqNW12+tBjnSeRcin8pnIsoS/PdOlmuXG68\n"
"EY4+T0uSjcPO0ocaqAO2AioCMpiHvAp/Sja0frsCgYBSRPf0nI6b7+KwE3YCQ8Zr\n"
"pZ8R3BjjauUoE+yN+2nbrJUFrMh7VsSN3S1c/LR8xp4hBUxQLb6TNHFsOsDzOYJj\n"
"buLgi03b1ndNNNUuqzeaxyXJOHOxXOus0PGaaRFDaxVO7Tqzjg4AwjYhl3ss+iwI\n"
"6JWOAfLO0bozSNvtiWrkHw==\n"
"-----END PRIVATE KEY-----\n";

WiFiClientSecure client;

void resetLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  HI PLEASE  ");
  lcd.setCursor(0, 1);
  lcd.print(" SWIPE YOUR CARD  ");
}

void pollForMobileApproval(String uid);
void triggerFeedback(bool isGranted, String lcdMessage);

bool checkAlarmStatus() {
  String activeStatusUrl = "https://YOUR_IP_ADDRESS/api/alarm-status";

  HTTPClient https;
  bool alarmIsOn = true; 

  if (https.begin(client, activeStatusUrl.c_str())) {
    int httpCode = https.GET();
    if (httpCode == 200) {
      String response = https.getString();
      if (response.indexOf("\"alarm\":false") > 0) {
        alarmIsOn = false;
      }
    }
    https.end();
  }
  return alarmIsOn;
}

void sendSystemAlert(String alertMessage) {
  String activeAlertUrl = "https://YOUR_IP_ADDRESS/api/alarm-status";

  HTTPClient https;
  if (https.begin(client, activeAlertUrl.c_str())) {
    https.addHeader("Content-Type", "application/json");
    String payload = "{\"message\":\"" + alertMessage + "\"}";
    https.POST(payload);
    https.end();
  }
}

void registerFailedAttempt() {
  failedAttempts++;
  Serial.print("[!] Failed Attempts: ");
  Serial.println(failedAttempts);

  if (failedAttempts >= MAX_FAILURES) {
    isLockdown = true;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" SYSTEM LOCKED! ");
    lcd.setCursor(0, 1);
    lcd.print(" Brute Force!   ");

    digitalWrite(RED_LED, HIGH);

    sendSystemAlert("Brute Force Attack Detected! Multiple invalid cards or denied requests.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting System.");

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);

  doorServo.attach(SERVO_PIN, 500, 2400);
  doorServo.write(0);

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("\n[+] Connecting to Wi-Fi...");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi.");

  // Connect directly to Windows Mobile Hotspot
  wifiMulti.addAP("YOUR_NETWORK_NAME", "YOUR_WIFI_PASSWORD");

  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[+] Wi-Fi Connected!");
  Serial.print("[+] Connected SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("[+] ESP32 Local IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < 1000000000) {
    delay(500);
  }

  client.setCertificate(client_cert);
  client.setPrivateKey(client_key);
  client.setInsecure();

  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);

  Serial.println("[+] System Ready. Awaiting card scans...");
  resetLCD();
}

void loop() {
  if (isLockdown) {
    digitalWrite(BUZZER, HIGH);
    delay(300);
    digitalWrite(BUZZER, LOW);
    delay(200);
    digitalWrite(BUZZER, HIGH);
    delay(300);
    digitalWrite(BUZZER, LOW);

    if (!checkAlarmStatus()) {
      isLockdown = false;
      failedAttempts = 0;
      digitalWrite(RED_LED, LOW);
      resetLCD();
    }
    
    delay(1000); 
    return;
  }

  if (wifiMulti.run() != WL_CONNECTED) {
    delay(1000);
    return;
  }

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return; 
  }

  String cardUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardUID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    cardUID += String(rfid.uid.uidByte[i], HEX);
  }
  cardUID.toUpperCase();
  rfid.PICC_HaltA();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reading Card...");

  String activeSwipeUrl = "https://YOUR_IP_ADDRESS/api/swipe";

  HTTPClient https;
  if (https.begin(client, activeSwipeUrl.c_str())) {
    https.addHeader("Content-Type", "application/json");
    String payload = "{\"rfid_uid\":\"" + cardUID + "\"}";
    int httpResponseCode = https.POST(payload);
    String responseBody = "";

    if (httpResponseCode > 0) {
      responseBody = https.getString();
    }

    Serial.println();
    Serial.print("[SCAN] UID: ");
    Serial.print(cardUID);
    Serial.print(" | HTTP Status: ");
    Serial.println(httpResponseCode);

    https.end();

    if (httpResponseCode == 200) {
      if (responseBody.indexOf("pending") > 0) {
        pollForMobileApproval(cardUID);
      } else {
        triggerFeedback(false, "Unknown Error");
      }
    } else if (httpResponseCode == 401) {
      triggerFeedback(false, "Invalid Card!");
      registerFailedAttempt();
    } else if (httpResponseCode == 403) {
      triggerFeedback(false, "Card Suspended!");
      registerFailedAttempt();
    } else {
      triggerFeedback(false, "Conn Failed!");
    }

  } else {
    triggerFeedback(false, "API Setup Error!");
  }

  delay(2000);
}

void pollForMobileApproval(String uid) {
  int attempts = 0;
  bool actionTaken = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Awaiting Mobile");
  lcd.setCursor(0, 1);
  lcd.print("Approval...  30s");

  digitalWrite(BUZZER, HIGH);
  delay(50);
  digitalWrite(BUZZER, LOW);

  while (attempts < 15 && !actionTaken) {
    delay(2000);
    attempts++;

    int timeLeft = 30 - (attempts * 2);
    lcd.setCursor(13, 1);
    lcd.print(timeLeft);
    lcd.print("s ");

    if (wifiMulti.run() == WL_CONNECTED) {
      String activePollUrl = "https://192.168.137.1:3000/api/poll";

      HTTPClient https;
      if (https.begin(client, activePollUrl.c_str())) {
        https.addHeader("Content-Type", "application/json");
        String payload = "{\"rfid_uid\":\"" + uid + "\"}";
        int httpCode = https.POST(payload);
        String response = https.getString();
        https.end();

        if (response.indexOf("Access Granted") > 0) {
          triggerFeedback(true, "Access Granted!");
          failedAttempts = 0;
          actionTaken = true;
        } else if (response.indexOf("Access Denied") > 0) {
          triggerFeedback(false, "Denied on App!");
          registerFailedAttempt();
          actionTaken = true;
        }
      }
    }
  }

  if (!actionTaken) {
    triggerFeedback(false, "Timeout!");
  }
}

void triggerFeedback(bool isGranted, String lcdMessage) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(lcdMessage);

  if (isGranted) {
    lcd.setCursor(0, 1);
    lcd.print("Door Unlocked.");

    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    doorServo.write(90);

    delay(200);
    digitalWrite(BUZZER, LOW);

    delay(3000);

    doorServo.write(0);
    digitalWrite(GREEN_LED, LOW);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Door Locked.");

    digitalWrite(RED_LED, HIGH);

    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER, HIGH);
      delay(150);
      digitalWrite(BUZZER, LOW);
      delay(150);
    }

    delay(1500);
    digitalWrite(RED_LED, LOW);
  }

  if (!isLockdown) {
    resetLCD();
  }
}