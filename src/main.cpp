#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"

const char *ssid = "MiFibra-3F62";
const char *password = "rfFpcff4";

int buttonPin = 15;
int led = 12;

String FirmwareVer = {
    "1.2"};
#define URL_fw_Version "https://raw.githubusercontent.com/alopez97/testing-ota-git/main/new_version/bin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/alopez97/testing-ota-git/main/new_version/firmware.bin"

//#define URL_fw_Version "http://cade-make.000webhostapp.com/version.txt"
//#define URL_fw_Bin "http://cade-make.000webhostapp.com/firmware.bin"

void connect_wifi();
void firmwareUpdate();
int FirmwareVersionCheck();

void setup()
{
  Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLDOWN);
  pinMode(led, OUTPUT);
  Serial.print("Active firmware version:");
  Serial.println(FirmwareVer);
  connect_wifi();
}
void loop()
{
  if (digitalRead(buttonPin) == HIGH)
  {
    Serial.println("Firmware update Starting..");
    FirmwareVersionCheck();
    delay(1000);
    firmwareUpdate();
  }
  digitalWrite(led, HIGH);
  delay(5000);
  digitalWrite(led, LOW);
  delay(5000);
}

void connect_wifi()
{
  Serial.println("Waiting for WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void firmwareUpdate(void)
{
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}

int FirmwareVersionCheck(void)
{
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure *client = new WiFiClientSecure;

  if (client)
  {
    client->setCACert(rootCACertificate);

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
    HTTPClient https;

    if (https.begin(*client, fwurl))
    { // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      }
      else
      {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }

  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer))
    {
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      return 0;
    }
    else
    {
      Serial.println(payload);
      Serial.println("New firmware detected");
      return 1;
    }
  }
  return 0;
}