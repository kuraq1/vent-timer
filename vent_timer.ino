#include <WiFi.h>
#include <algorithm>
#include "time.h"

const int ventPin = 4;
const char* ssid = "ITS";
const char* password = "inzynier";

const char* ntpServer = "pool.ntp.org";
const char* tzInfo = "CET-1CEST,M3.5.0,M10.5.0/3";

int lastSyncDay = -1;

struct interval {
  int startMinutes;
  int endMinutes;
};

interval workTimes[] = {
  {0 * 60 + 0, 0 * 60 + 10},
  {1 * 60 + 0, 1 * 60 + 10},
  {2 * 60 + 0, 2 * 60 + 10},
  {3 * 60 + 0, 3 * 60 + 10},
  {4 * 60 + 0, 4 * 60 + 10},
  {5 * 60 + 0, 5 * 60 + 10},
  {6 * 60 + 0, 6 * 60 + 10},
  {7 * 60 + 0, 7 * 60 + 10},
  {8 * 60 + 0, 8 * 60 + 10},
  {9 * 60 + 0, 9 * 60 + 10},
  {10 * 60 + 0, 10 * 60 + 10},
  {11 * 60 + 0, 11 * 60 + 10},
  {12 * 60 + 0, 12 * 60 + 10},
  {13 * 60 + 0, 13 * 60 + 10},
  {14 * 60 + 0, 14 * 60 + 10},
  {15 * 60 + 0, 15 * 60 + 10},
  {16 * 60 + 0, 16 * 60 + 10},
  {17 * 60 + 0, 17 * 60 + 10},
  {18 * 60 + 0, 18 * 60 + 10},
  {19 * 60 + 0, 19 * 60 + 10},
  {20 * 60 + 0, 20 * 60 + 10},
  {21 * 60 + 0, 21 * 60 + 10},
  {22 * 60 + 0, 22 * 60 + 10},
  {23 * 60 + 0, 23 * 60 + 10}
};

bool isBetween(const interval& i, const tm& timeinfo) {
  int currentTime = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  if (i.startMinutes < i.endMinutes) {
    return (currentTime >= i.startMinutes && currentTime < i.endMinutes);
  } else {
    return (currentTime >= i.startMinutes || currentTime < i.endMinutes);
  }
}

bool syncTimeOverWiFi() {
  bool success = false;

  Serial.println("\n[WiFi] Próba połączenia z siecią...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while(WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Połączono!");
    configTzTime(tzInfo, ntpServer);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10000)) {
      Serial.println("[NTP] Czas zsynchronizowany pomyślnie.");
      success = true;
    } else {
      Serial.println("[NTP] Błąd pobierania czasu.");
    }
  } else {
    Serial.println("\n[WiFi] Nie udało się połączyć.");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("[WiFi] Rozłączono i wyłączono moduł (oszczędzanie energii).");

  return success;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- START URZĄDZENIA ---");

  btStop();
  pinMode(ventPin, OUTPUT);
  digitalWrite(ventPin, LOW);

  syncTimeOverWiFi();
}

void loop() {
  timeCallback();

  Serial.flush();

  esp_sleep_enable_timer_wakeup(10 * 1000000ULL);
  esp_light_sleep_start();
}

void timeCallback() {
  struct tm timeinfo;

  if(!getLocalTime(&timeinfo) || timeinfo.tm_year < 120) {
    Serial.println("[BŁĄD] Brak prawidłowego czasu, wymuszam synchronizację...");
    syncTimeOverWiFi();
    return;
  }

  if (timeinfo.tm_hour == 4 && timeinfo.tm_wday != lastSyncDay) {
     Serial.println("[NTP] Poranna synchronizacja czasu...");
     if (syncTimeOverWiFi()) {
        lastSyncDay = timeinfo.tm_wday;
     }
  }

  bool should_be_on = false;

  for (const auto& wTime : workTimes) {
    if (isBetween(wTime, timeinfo)) {
      should_be_on = true;
      break;
    }
  }

  if (should_be_on) {
    digitalWrite(ventPin, HIGH);
  } else {
    digitalWrite(ventPin, LOW);
  }

  Serial.printf("[INFO] Czas: %02d:%02d:%02d | Wentylator: %s\n",
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                should_be_on ? "WŁĄCZONY" : "WYŁĄCZONY");
}