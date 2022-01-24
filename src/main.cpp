#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"


#include <ModbusIP_ESP8266.h>
#include <Losant.h>

#include <secrets.h>

//WiFi credentials
const char *SSID = SSID_SECRET;
const char *PASS = PASS_SECRET;

//Losant credentials
const char *LOSANT_ID = ID;
const char *LOSANT_KEY = KEY;
const char *LOSANT_SECRET = SECRET;


#define DEBUG true

int buttonPin = 15;
int led = 12;

String FirmwareVer = {
    "1.0"};

#define URL_fw_Version "https://raw.githubusercontent.com/alopez97/testing-ota-git/main/new_version/bin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/alopez97/testing-ota-git/main/new_version/firmware.bin"

//Losant
WiFiClientSecure wifiClient;    //Initialize Secure WiFi Client
LosantDevice device(LOSANT_ID); //Initialize Losant Device

// Modbus Registers Offsets
const int TEST_COIL = 1;
//ModbusIP object
ModbusIP mb;

//Auxiliar functions declaration
void wifiConnect();
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
void LosantReconnect();

void firmwareUpdate();
int FirmwareVersionCheck();

void setup()
{
  Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLDOWN);
  pinMode(led, OUTPUT);
  Serial.print("Active firmware version:");
  Serial.println(FirmwareVer);

  wifiConnect(); //Connect wifi

  mb.server(); //Modbus slave
  mb.addCoil(TEST_COIL, 0);

  LosantReconnect(); //Losant
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


  LosantReconnect();
  mb.task();
  delay(10);
  device.loop();
  Serial.print("Coil 1: ");
  Serial.println(mb.Coil(TEST_COIL));
  delay(1000);

  //JSON parser document
  StaticJsonDocument<100> doc; //Important to set a value that is larger than the payload we want to send
  JsonObject data = doc.to<JsonObject>();

  //Currents
  data["bool"] = mb.Coil(TEST_COIL); //currentL1

  // device.setId(perifericalId); //
  device.sendState(data); //Send data to Losant
  delay(3000);
}

void wifiConnect()
{
  if (WiFi.status() != WL_CONNECTED)
  {

    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
#if DEBUG
      Serial.print(".");
#endif
      delay(500);
    }

#if DEBUG
    Serial.println("Successfully connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
#endif

    wifiClient.setCACert(root_ca_mqtt_losant); //Set the Certificate in order to establish a secure MQTT Connection
  }
}

/**
 * Function to connect to Losant whenever not connected.  
 */

void LosantReconnect()
{
  if (!device.connected())
  {

    while (!device.connected())
    {
      device.setId(LOSANT_ID); //Its CRITICAL that we set the Gateway Id before trying to connect to Losant. If WiFi goes down and comes back,
      //the last Id that the device object will have is the periphericalId, so we need to correct this using the setId function.

      device.connectSecure(wifiClient, LOSANT_KEY, LOSANT_SECRET);
      delay(1000);

#if DEBUG
      Serial.print("*");
      // These functions could be useful in case Losant doesn't allow the connection, to debug what is the problem.
      Serial.println(device.mqttClient.lastError());
      Serial.println(device.mqttClient.returnCode());
#endif
    }
  }
}

void firmwareUpdate(void)
{
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  Serial.println("Downloading .bin file...");
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
      Serial.println("The new firmware will be downloaded and installed...");
      return 1;
    }
  }
  return 0;
}