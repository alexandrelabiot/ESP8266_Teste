//LED will blink when in config mode

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

//for LED status
#include <Ticker.h>
Ticker ticker;

#ifndef LED_BUILTIN
#define LED_BUILTIN 13 // ESP32 DOES NOT DEFINE LED_BUILTIN
#endif

#define DHTPIN 4      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // DHT 11

DHT dht(DHTPIN, DHTTYPE);

float t = 0.0;
float h = 0.0;

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0; // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 10000;

#define RELE1 14
#define BTNRESETCONFIG 5

int LED = LED_BUILTIN;

void tick()
{
  //toggle state
  digitalWrite(LED, !digitalRead(LED)); // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

//Codigo Adicionado para controle do servidor web (Alexandre)

ESP8266WebServer server(80);

void TesteLed()
{
  if (server.method() != HTTP_POST)
  {
    server.send(405, "text/html", "Metodo invalido");
    return;
  }

  if (server.arg("senha") != "ff0033")
  {
    server.send(401, "text/html", "Nao autorizado");
    return;
  }
  Serial.println(server.arg("ligado"));

  int ret = (server.arg("ligado") == "1") ? 1 : 0;

  digitalWrite(LED_BUILTIN, ret);

  server.send(200, "text/html", "Teste Led  - 200");
}

void TrataRele()
{
  if (server.method() != HTTP_POST)
  {
    server.send(405, "text/html", "Metodo invalido");
    return;
  }

  if (server.arg("senha") != "ff0033")
  {
    server.send(401, "text/html", "Nao autorizado");
    return;
  }
  Serial.println(server.arg("Rele ligado"));

  int ret = (server.arg("ligado") == "1") ? 1 : 0;

  digitalWrite(RELE1, ret);

  server.send(200, "text/html", "Rele  - 200");
}

void handleRoot()
{
  server.send(200, "text/html", "Alexande - 200");
}

void handleNotFound()
{
  server.send(400, "text/html", "Alexande - 400");
}

//Findo código adicionado

void setup()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // put your setup code here, to run once:
  Serial.begin(115200);

  dht.begin();

  //set led pin as output
  pinMode(LED, OUTPUT);

  //Seta pino do rele como saida
  pinMode(RELE1, OUTPUT);

  //Seta pino do reset de fabrica
  pinMode(BTNRESETCONFIG, INPUT);

  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  //reset settings - for testing

  if (digitalRead(BTNRESETCONFIG) == 0)
  {
    wm.resetSettings();

    while (digitalRead(BTNRESETCONFIG) == 0)
      ;
  }

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect())
  {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(LED, LOW);

  if (MDNS.begin("esp8266"))
  {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/led", TesteLed);

  server.on("/rele", TrataRele);

  server.on("/inline", []() {
    server.send(200, "text/html", "Alexande - Inline");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Server estarted...");
}

void loop()
{
  server.handleClient();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      t = newT;
      Serial.print("Temperatura.: ");
      Serial.println(t);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value
    if (isnan(newH))
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
      h = newH;
      Serial.print("Umidade.: ");
      Serial.println(h);
    }
  }
}