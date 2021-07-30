/*
Copyright (c) 2021 Watermon.
This software is released under the MIT License.
https://opensource.org/licenses/MIT
*/

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#define GET_CHIPID()  (ESP.getChipId())
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#define GET_CHIPID()  ((uint16_t)(ESP.getEfuseMac()>>32))
#endif
#include <FS.h>
#include <PubSubClient.h>
#include <AutoConnect.h>
#include <Arduino.h>

#include "SSD1306.h"
#define SSD1306_ADDRESS 0x3c
#define I2C_SDA 21
#define I2C_SCL 22
SSD1306 oled(SSD1306_ADDRESS, I2C_SDA, I2C_SCL);
//#define TEST_MQTT 1 

/*
#define S0 27
#define S1 26
#define S2 25
#define S3 33
#define sensorOut 32
*/

#define S0 15
#define S1 2
#define S2 4
#define S3 5
#define sensorOut 18


int frequency = 0;
int ipdisp = 1;


#define AUTOCONNECT_MENULABEL_CONFIGNEW   "Configure new AP"
#define AUTOCONNECT_MENULABEL_OPENSSIDS   "Open SSIDs"
#define AUTOCONNECT_MENULABEL_DISCONNECT  "Disconnect"
#define AUTOCONNECT_MENULABEL_RESET       "Reset..."
#define AUTOCONNECT_MENULABEL_HOME        "WaterMon HOME"
#define AUTOCONNECT_BUTTONLABEL_RESET     "RESET"

#define PARAM_FILE      "/param.json"
#define AUX_SETTING_URI "/mqtt_setting"
#define AUX_SAVE_URI    "/mqtt_save"
#define AUX_CLEAR_URI   "/mqtt_clear"

AutoConnectConfig  Config;

// JSON definition of AutoConnectAux.
// Multiple AutoConnectAux can be defined in the JSON array.
// In this example, JSON is hard-coded to make it easier to understand
// the AutoConnectAux API. In practice, it will be an external content
// which separated from the sketch, as the mqtt_RSSI_FS example shows.
static const char AUX_mqtt_setting[] PROGMEM = R"raw(
[
  {
    "title": "MQTT Setting",
    "uri": "/mqtt_setting",
    "menu": true,
    "element": [
      {
        "name": "style",
        "type": "ACStyle",
        "value": "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}"
      },
      {
        "name": "header",
        "type": "ACText",
        "value": "<h2>MQTT broker settings</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "caption",
        "type": "ACText",
        "value": "Publishing the Sensor Readings to MQTT channel",
        "style": "font-family:serif;color:#4682b4;"
      },
      {
        "name": "mqttserver",
        "type": "ACInput",
        "value": "",
        "label": "Server",
        "pattern": "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$",
        "placeholder": "MQTT broker server"
      },
      {
        "name": "channelid",
        "type": "ACInput",
        "label": "Channel ID",
        "pattern": "^[0-9]{6}$"
      },
      {
        "name": "userkey",
        "type": "ACInput",
        "label": "User Key"
      },
      {
        "name": "apikey",
        "type": "ACInput",
        "label": "API Key"
      },
      {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "uniqueid",
        "type": "ACCheckbox",
        "value": "unique",
        "label": "Use APID unique",
        "checked": false
      },
      {
        "name": "period",
        "type": "ACRadio",
        "value": [
          "30 sec.",
          "60 sec.",
          "180 sec."
        ],
        "label": "Update period",
        "arrange": "vertical",
        "checked": 1
      },
      {
        "name": "newline1",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "hostname",
        "type": "ACInput",
        "value": "",
        "label": "ESP host name",
        "pattern": "^([a-zA-Z0-9]([a-zA-Z0-9-])*[a-zA-Z0-9]){1,24}$"
      },
      {
        "name": "newline2",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "mode",
        "type": "ACRadio",
        "value": [
          "Sense",
          "Test",
          "Traning"
        ],
        "label": "Update Mode",
        "arrange": "vertical",
        "checked": 1
      },

      {
        "name": "sensorReadingName",
        "type": "ACInput",
        "value": "",
        "label": "Sensor Name for Traning",
        "pattern": "^([a-zA-Z0-9]([a-zA-Z0-9-])*[a-zA-Z0-9]){1,24}$"
      },
      
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save&amp;Start",
        "uri": "/mqtt_save"
      },
      {
        "name": "discard",
        "type": "ACSubmit",
        "value": "Discard",
        "uri": "/"
      }
    ]
  },
  {
    "title": "MQTT Setting",
    "uri": "/mqtt_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parameters",
        "type": "ACText"
      },
      {
        "name": "clear",
        "type": "ACSubmit",
        "value": "Clear channel",
        "uri": "/mqtt_clear"
      }
    ]
  }
]
)raw";

// Adjusting WebServer class with between ESP8266 and ESP32.
#if defined(ARDUINO_ARCH_ESP8266)
typedef ESP8266WebServer  WiFiWebServer;
#elif defined(ARDUINO_ARCH_ESP32)
typedef WebServer WiFiWebServer;
#endif

AutoConnect  portal;
AutoConnectConfig config;
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);
String  serverName;
String  channelId;
String  userKey;
String  apiKey;
String  apid;
String  hostName;
String  sensorReadingName;
String  mode;
bool  uniqueid;
unsigned int  updateInterval = 0;
unsigned long lastPub = 0;

#define MQTT_USER_ID  "WaterMon"

bool mqttConnect() {
  static const char alphanum[] = "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";  // For random generation of client ID.
  char    clientId[9];

  uint8_t retry = 3;
  while (!mqttClient.connected()) {
    if (serverName.length() <= 0)
      break;

    mqttClient.setServer(serverName.c_str(), 1883);
    Serial.println(String("Attempting MQTT broker:") + serverName);

    for (uint8_t i = 0; i < 8; i++) {
      clientId[i] = alphanum[random(62)];
    }
    clientId[8] = '\0';

    if (mqttClient.connect(clientId, MQTT_USER_ID, userKey.c_str())) {
      Serial.println("Established:" + String(clientId));
      return true;
    }
    else {
      Serial.println("Connection failed:" + String(mqttClient.state()));
      if (!--retry)
        break;
      delay(3000);
    }
  }
  return false;
}

void mqttPublish(String msg) {
  //String path = String("channels/") + channelId + String("/publish/") + apiKey;
  
#ifdef TEST_MQTT
    String datamsg = "6.3, 2.9, 5.6, 1.8";
    String setosamsg = "4.9,3.1,1.5,0.1";
    String path = "/Classification";
    //String pubmsg = "[[" + setosamsg + "," + sensorReadingName + "]]";
    String pubmsg = "[[" + setosamsg + "]]";
   
    //mqttClient.publish(path.c_str(), msg.c_str());
    mqttClient.publish(path.c_str(), pubmsg.c_str() );
    
    Serial.println(String("Publish MQTT broker:") + serverName + path.c_str() + pubmsg.c_str());  
  
#else

  String path = channelId;
  mqttClient.publish(path.c_str(), msg.c_str());
  //mqttClient.publish("\All", msg.c_str());
  Serial.println(String("Publish MQTT broker:") + serverName + path.c_str() + msg.c_str());
#endif

  String pubpath = "/SensorReadingName";
  String pubmsg1 = sensorReadingName;
   
  mqttClient.publish(pubpath.c_str(), pubmsg1.c_str() );

}

int getStrength(uint8_t points) {
  uint8_t sc = points;
  long    rssi = 0;

  while (sc--) {
    rssi += WiFi.RSSI();
    delay(20);
  }
  return points ? static_cast<int>(rssi / points) : 0;
}

void getParams(AutoConnectAux& aux) {
  serverName = aux["mqttserver"].value;
  serverName.trim();
  channelId = aux["channelid"].value;
  channelId.trim();
  userKey = aux["userkey"].value;
  userKey.trim();
  apiKey = aux["apikey"].value;
  apiKey.trim();
  AutoConnectRadio& period = aux["period"].as<AutoConnectRadio>();
  //updateInterval = period.value().substring(0, 2).toInt() * 1000;
  updateInterval = period.value().substring(0, 2).toInt() * 100;
  uniqueid = aux["uniqueid"].as<AutoConnectCheckbox>().checked;
  hostName = aux["hostname"].value;
  hostName.trim();
  sensorReadingName = aux["sensorReadingName"].value;
  sensorReadingName.trim();
  //mode = String(aux["mode"].as<AutoConnectRadio>());
}

// Load parameters saved with  saveParams from SPIFFS into the
// elements defined in /mqtt_setting JSON.
String loadParams(AutoConnectAux& aux, PageArgument& args) {
  (void)(args);
  
  //SPIFFS.begin(true);
  File param = SPIFFS.open(PARAM_FILE, "r");
  if (param) {
    if (aux.loadElement(param)) {
      getParams(aux);
      Serial.println(PARAM_FILE " loaded");
    }
    else
      Serial.println(PARAM_FILE " failed to load");
    param.close();
  }
  else {
    Serial.println(PARAM_FILE " open failed");
#ifdef ARDUINO_ARCH_ESP32
    Serial.println("If you get error as 'SPIFFS: mount failed, -10025', Please modify with 'SPIFFS.begin(true)'.");
#endif
  }
  return String("");
}

// Save the value of each element entered by '/mqtt_setting' to the
// parameter file. The saveParams as below is a callback function of
// /mqtt_save. When invoking this handler, the input value of each
// element is already stored in '/mqtt_setting'.
// In Sketch, you can output to stream its elements specified by name.
String saveParams(AutoConnectAux& aux, PageArgument& args) {
  // The 'where()' function returns the AutoConnectAux that caused
  // the transition to this page.
  AutoConnectAux&   mqtt_setting = *portal.aux(portal.where());
  getParams(mqtt_setting);
  AutoConnectInput& mqttserver = mqtt_setting["mqttserver"].as<AutoConnectInput>();

  // The entered value is owned by AutoConnectAux of /mqtt_setting.
  // To retrieve the elements of /mqtt_setting, it is necessary to get
  // the AutoConnectAux object of /mqtt_setting.
  File param = SPIFFS.open(PARAM_FILE, "w");
  mqtt_setting.saveElement(param, { "mqttserver", "channelid", "userkey", "apikey", "uniqueid", "period", "hostname", "sensorReadingName", "mode" });
  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText&  echo = aux["parameters"].as<AutoConnectText>();
  echo.value = "Server: " + serverName;
  echo.value += mqttserver.isValid() ? String(" (OK)") : String(" (ERR)");
  echo.value += "<br>Channel ID: " + channelId + "<br>";
  echo.value += "User Key: " + userKey + "<br>";
  echo.value += "API Key: " + apiKey + "<br>";
  echo.value += "Update period: " + String(updateInterval / 1000) + " sec.<br>";
  echo.value += "Use APID unique: " + String(uniqueid == true ? "true" : "false") + "<br>";
  echo.value += "ESP host name: " + hostName + "<br>";
  
  echo.value += "Sesor Reading Name: " + sensorReadingName + "<br>";
  echo.value += "Sensor Mode: " + mode + "<br>";
  
  return String("");
}

void handleRoot() {
  String  content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "</head>"
    "<body>"
    "<iframe width=\"450\" height=\"260\" style=\"transform:scale(0.79);-o-transform:scale(0.79);-webkit-transform:scale(0.79);-moz-transform:scale(0.79);-ms-transform:scale(0.79);transform-origin:0 0;-o-transform-origin:0 0;-webkit-transform-origin:0 0;-moz-transform-origin:0 0;-ms-transform-origin:0 0;border: 1px solid #cccccc;\" src=\"https://thingspeak.com/channels/454951/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&type=line\"></iframe>"
    "<p style=\"padding-top:5px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
    "</body>"
    "</html>";

  WiFiWebServer&  webServer = portal.host();
  webServer.send(200, "text/html", content);
}

// Clear channel using ThingSpeak's API.
void handleClearChannel() {
  HTTPClient  httpClient;
  WiFiClient  client;
  String  endpoint = serverName;
  endpoint.replace("mqtt", "api");
  String  delUrl = "http://" + endpoint + "/channels/" + channelId + "/feeds.json?api_key=" + userKey;

  Serial.print("DELETE " + delUrl);
  if (httpClient.begin(client, delUrl)) {
    Serial.print(":");
    int resCode = httpClient.sendRequest("DELETE");
    String  res = httpClient.getString();
    httpClient.end();
    Serial.println(String(resCode) + "," + res);
  }
  else
    Serial.println(" failed");

  // Returns the redirect response.
  WiFiWebServer&  webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String("/"));
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
}

void setup() {
  delay(1000);
  //Serial.begin(115200);
  Serial.begin(9600);
  Serial.println();
  //SPIFFS.begin();
  SPIFFS.begin(true);
  
  oled.init();
  oled.flipScreenVertically();
  oled.setFont(ArialMT_Plain_16);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  delay(50);

  oled.clear();
  oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "WaterMon.org");
  oled.display();
  
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
 
  // Setting frequency-scaling to 20%
  digitalWrite(S0,HIGH);
  digitalWrite(S1,HIGH);


  if (portal.load(FPSTR(AUX_mqtt_setting))) {
    AutoConnectAux& mqtt_setting = *portal.aux(AUX_SETTING_URI);
    PageArgument  args;
    loadParams(mqtt_setting, args);
    if (uniqueid) {
      config.apid = String("ESP") + "-" + String(GET_CHIPID(), HEX);
      Serial.println("apid set to " + config.apid);
    }
    if (hostName.length()) {
      config.hostName = hostName;
      Serial.println("hostname set to " + config.hostName);
    }
    
    /*
    if (sensorReadingName.length()) {
      config.sensorReadingName = sensorReadingName;
      Serial.println("sensorReadingName set to " + config.sensorReadingName);
    }*/

    config.bootUri = AC_ONBOOTURI_HOME;
    config.homeUri = "/";

  config.apid = "WaterMon";
  config.psk  = "Password";
  config.autoReconnect = true;
  config.title = "WaterMon";
  /*
  config.staip = IPAddress(192,168,1,15);
  config.staGateway = IPAddress(192,168,1,1);
  config.staNetmask = IPAddress(255,255,255,0);
  config.dns1 = IPAddress(192,168,1,1);
  */
  //Portal.config(Config);

    
    portal.config(config);

    portal.on(AUX_SETTING_URI, loadParams);
    portal.on(AUX_SAVE_URI, saveParams);
  }
  else
    Serial.println("load error");

  Serial.print("WiFi ");
  if (portal.begin()) {
    Serial.println("connected:" + WiFi.SSID());
    Serial.println("IP:" + WiFi.localIP().toString());
    oled.clear();
    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, WiFi.localIP().toString());
    oled.display();
  }
  else {
    Serial.println("connection failed:" + String(WiFi.status()));
    Serial.println("Needs WiFi connection to start publishing messages");
  }
  
  Serial.print("Starting WebServer ");
  WiFiWebServer&  webServer = portal.host();
  webServer.on("/", handleRoot);
  webServer.on(AUX_CLEAR_URI, handleClearChannel);
}

void loop() {

  //Serial.println("Connected:" + WiFi.SSID());
  
  delay(200);

  // Colour Sensor Reading 
  // Setting red filtered photodiodes to be read
  digitalWrite(S2,LOW);
  digitalWrite(S3,LOW); 
  // Reading the output frequency
  frequency = pulseIn(sensorOut, LOW);
  // Printing the value on the serial monitor
  String red = String(frequency, DEC);
  
  delay(200);
  // Setting Green filtered photodiodes to be read
  digitalWrite(S2,HIGH);
  digitalWrite(S3,HIGH);
  // Reading the output frequency
  frequency = pulseIn(sensorOut, LOW);
  // Printing the value on the serial monitor
  String green = String(frequency, DEC);
  
  delay(200);
  // Setting Blue filtered photodiodes to be read
  digitalWrite(S2,LOW);
  digitalWrite(S3,HIGH);
  // Reading the output frequency
  frequency = pulseIn(sensorOut, LOW);
  // Printing the value on the serial monitor
  String blue = String(frequency,DEC);

  
  delay(200);
  // Setting Clear filtered photodiodes to be read
  digitalWrite(S2,HIGH);
  digitalWrite(S3,LOW);
  // Reading the output frequency
  frequency = pulseIn(sensorOut, LOW);
  // Printing the value on the serial monitor
  String clear = String(frequency,DEC);
  delay(200);
  
  String sensordisp = "R: " + red + " G: " + green +   " B: " + blue +  " C: " + clear;

  oled.clear();
  oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, sensordisp);
  oled.display();
       
  
  if (WiFi.status() == WL_CONNECTED) {
     Serial.println("Connected:" + WiFi.SSID());
     String sensordisp = "R: " + red + ", G: " + green +   ", B: " + blue +  ", C: " + clear;
     
  
    // MQTT publish control
    if (updateInterval > 0) {
      if (millis() - lastPub > updateInterval) {
        if (!mqttClient.connected()) {
          mqttConnect();
        }
    
        String sensorval = "[[" + red + "," + green +   "," + blue +  "," + clear + "]]";
        mqttPublish(sensorval);

        String redstr = "/red";
        String bluestr = "/blue";
        String greenstr = "/green";
        String clearstr = "/clear";
        
        mqttClient.publish(redstr.c_str(), red.c_str());
        mqttClient.publish(bluestr.c_str(), blue.c_str());
        mqttClient.publish(greenstr.c_str(), green.c_str());
        mqttClient.publish(clearstr.c_str(), clear.c_str());
           
        mqttClient.loop();
        lastPub = millis();
      }
    }
  }
  portal.handleClient();
}
