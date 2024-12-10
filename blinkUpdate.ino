#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

const char* ssid = "FIRST_GalaxyA51";
const char* password = "first25331990";

const char* githubRepoOwner = "pbangtawee";
const char* githubRepoName = "blinkUpdate";
const char* currentVersion = "1.0.0";

unsigned long lastCheckUpdate;
unsigned long lastBlink;

void setup()
{
  Serial.begin(115200);
  delay(500);

  lastBlink = millis();
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  lastCheckUpdate = millis();
  checkForUpdate();
}

void loop()
{
  if(lastBlink - millis() >= 1000)
  {
    lastBlink = millis();
    digitalWrite(2, !digitalRead(2));
  }
  
  if (lastCheckUpdate - millis() >= 10000)
  {
    lastCheckUpdate = millis();
    checkForUpdate();
  }
  
}

void checkForUpdate()
{
  Serial.println("Checking for updates...");
  HTTPClient http;

  String url = "https://api.github.com/repos/" + String(githubRepoOwner) + "/" + String(githubRepoName) + "/releases/latest";
  http.begin(url);
  http.addHeader("User-Agent", "ESP32-OTA-Updater"); // GitHub need User-Agent

  int httpCode = http.GET();
  if (httpCode == 200)
  {
    String payload = http.getString();
    Serial.println("Release info: " + payload);

    // Pull version from JSON Response
    int tagIndex = payload.indexOf("\"tag_name\":\"") + 12;
    int tagEndIndex = payload.indexOf("\"", tagIndex);
    String latestVersion = payload.substring(tagIndex, tagEndIndex);

    Serial.println("Current version: " + String(currentVersion));
    Serial.println("Latest version: " + latestVersion);

    if (latestVersion != currentVersion)
    {
      Serial.println("New version found, updating...");
      performOTAUpdate(latestVersion);
    }
    else
    {
      Serial.println("No new updates available.");
    }
  }
  else
  {
    Serial.println("Failed to fetch release info, HTTP code: " + String(httpCode));
  }

  http.end();
}

void performOTAUpdate(String latestVersion)
{
  HTTPClient http;
  String binUrl = "https://github.com/" + String(githubRepoOwner) + "/" + String(githubRepoName) + "/releases/download/" + latestVersion + "/firmware.bin";
  
  http.begin(binUrl);
  int httpCode = http.GET();

  if (httpCode == 200)
  {
    int contentLength = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    if (Update.begin(contentLength))
    {
      size_t written = Update.writeStream(*stream);
      if (written == contentLength)
      {
        Serial.println("Update written successfully.");
      }
      else
      {
        Serial.println("Written only " + String(written) + "/" + String(contentLength) + ". Update failed.");
      }

      if (Update.end())
      {
        if (Update.isFinished())
        {
          Serial.println("Update successfully completed. Rebooting...");
          ESP.restart();
        }
        else
        {
          Serial.println("Update not finished. Something went wrong.");
        }
      }
      else
      {
        Serial.println("Error occurred: " + String(Update.getError()));
      }
    }
    else
    {
      Serial.println("Not enough space to begin OTA.");
    }
  }
  else
  {
    Serial.println("Failed to download firmware, HTTP code: " + String(httpCode));
  }

  http.end();
}
