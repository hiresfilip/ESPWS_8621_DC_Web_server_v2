/* Knihovna pro technologii OTA = over the air - uploadování přes wifi */
#include <ArduinoOTA.h>

/* Podmínka určuje, jestli je připojen čip ESP32, když ne, tak určí ESP8266
(samozřejmě za přepokladu, že tento projekt se nahrává do čipu ESP)*/

#ifdef ESP32
#include <FS.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)

/* Knihovna zajišťuje komunikaci wifi a ESP */
#include <ESP8266WiFi.h>

/* Knihovna, jejiž cílem je umožnit bezproblémové síťové prostředí pro ESP8266 */
#include <ESPAsyncTCP.h>

/* Knihovna, zajišťuje získání IP pro ESP */
#include <ESP8266mDNS.h>
#endif

/* Knihovna pro AsyncWebServer, tedy synchronizace Webu a ESP */
#include <ESPAsyncWebServer.h>

/* Knihovna, která umožňuje poslání souborového systému (file system - FS) */
#include <SPIFFSEditor.h>

/* Knihovna, která umožňuje stahování aktuálního času */
#include <NTPClient.h>

/* Knihovna zajišťuje komunikaci wifi a ESP */
#include <ESP8266WiFi.h>

/* Knihovna pro přenos UDP paketů */
#include <WiFiUdp.h>

/* Knihovna z jazyka C, umožňuje převést data na jednoduchý řetězec (string), typický pro jazyk C */
#include <string>

/* Knihovna, která se stará o GET a POST pro technologii JSON mezi ESP a stránku index.html */
#include <ArduinoJson.h>

/* Knihovna, která se stará o LED pásek WS2812B */
#include <FastLED.h>

#define LED_PIN 4
#define NUM_LEDS 116
#define CHIPSET WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
CRGB color;
int brightness;

/* Určení časové zóny */
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

/* Časová zóna je Česká Republika */
NTPClient timeClient(ntpUDP, "cz.pool.ntp.org", 3600, 60000);

/* Proměnné týkající se AsyncWebServeru */
AsyncWebServer server(80);

/* Proměnná pro Web Socket */
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

/* Proměnné týkající se NTPClientu */
String formattedDate;
String dayStamp;
String timeStamp;


/* Funkce, která zaznamenává eventy (události), které ESP dostane od serveru a vypíše do serial monitoru */
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    Serial.printf("Prijata data: ");
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      /* Celá zpráva je v jednom rámci (framu) a dostaneme z ní všechna data */
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }

        Serial.printf("%s\n",msg.c_str());
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, msg.c_str());

        int r_val = doc["red"];
        int g_val = doc["green"];
        int b_val = doc["blue"];
        int alfa = doc["alfa"];

        color = CRGB(r_val,g_val,b_val);
        brightness = alfa;

        Serial.printf("RED: %d\n", r_val);
        Serial.printf("GREEN: %d\n", g_val);
        Serial.printf("BLUE: %d\n", b_val);
        Serial.printf("ALFA: %d\n", alfa);


      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {

      /* Zpráva se zkládá z více rámců nebo je rámec rozdělen do více paketů */
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}
/* Definování proměnných k připojení na internet a k aplikaci */

/*const char* ssid = "ESPNet";
const char* password = "";*/
const char * ssid = "TP-Link_7632";
const char * password = "97261261";
/*const char * ssid = "www.computerparts.cz";
const char * password = "algoritmus";*/
const char * hostName = "ESPWS_8621_WS_DC";
const char * http_username = "admin";
const char * http_password = "admin";
const char * PARAM_MESSAGE = "message";

/* Funkce, která nastaví wifi připojení */
void setup(){
  Serial.begin(115200);
  color = CRGB(255,0,0);
  brightness = 200;
  FastLED.setBrightness(brightness);
  Serial.setDebugOutput(true);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(hostName);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
  }

  /* Funkce, která aktivuje timeClienta z knihovny NTP Client */
  timeClient.begin();
  timeClient.setTimeOffset(3600);

  /* Funkce nadefinuje pásek i se světelností */
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  //FastLED.setBrightness(alfa); /* MAX: 255 */

  /* Funkce, která pošle skrze technologii OTA eventy (údálosti) prohlížeči */
  ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
  ArduinoOTA.onEnd([]() { events.send("Update End", "ota"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
    events.send(p, "ota");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
    else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
    else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
    else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
    else if(error == OTA_END_ERROR) events.send("End Failed", "ota");
  });

  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();

  MDNS.addService("http","tcp",80);
 
 /* Aktivace souborového systému (file system -> FS) pro ukládání na ESP čip */
  SPIFFS.begin();

/* Vyvolání funkce ws (WebSocket), která naslouchá na jendotlivé eventy (události) */
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

/* Při připojení na aplikaci, klient zašle zkušební zprávu */
  events.onConnect([](AsyncEventSourceClient *client){
    client->send("hello!",NULL,millis(),1000);
  });
  server.addHandler(&events);
  

#ifdef ESP32
  server.addHandler(new SPIFFSEditor(SPIFFS, http_username,http_password));
#elif defined(ESP8266)
  server.addHandler(new SPIFFSEditor(http_username,http_password));
#endif

/* API část je zapoznámkovaná jelikož používám technologii Web Socket */
   /*server.on("/post", HTTP_GET, [](AsyncWebServerRequest *request){
        String message;
        request->send(200, "text/plain", String(timeClient.getFormattedTime()));
    });
    server.on("/data", HTTP_POST, [](AsyncWebServerRequest *request){
        String message;
        if (request->hasParam(PARAM_MESSAGE, true)) {
            message = request->getParam(PARAM_MESSAGE, true)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, POST: " + String(message));
    });

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });*/
  
 
/* Routování, metody POST, GET, PUT, UNKNOW, atd. včetně chyby 404 - stránka nenalezena, WebSocket technologie */
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  /* Metoda pro uploadování souboru na server */
  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char*)data);
    if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });

  /* Metoda pro příjem informací ze stránky index.html do ESP */
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char*)data);
    if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });
  server.begin();
}


/* Jednotky minut */
int digitMin1[10][28] = {
{ 4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,-1,-1,-1,-1 },       //0
{ 12,13,14,15,16,17,18,19,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, //1
{ 0,1,2,3,8,9,10,11,12,13,14,15,20,21,22,23,24,25,26,27,-1,-1,-1,-1,-1,-1,-1,-1 },       //2
{ 0,1,2,3,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,-1,-1,-1,-1,-1,-1,-1,-1 },       //3
{ 0,1,2,3,4,5,6,7,12,13,14,15,16,17,18,19,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },         //4
{ 0,1,2,3,4,5,6,7,8,9,10,11,16,17,18,19,20,21,22,23,-1,-1,-1,-1,-1,-1,-1,-1 },           //5
{ 0,1,2,3,4,5,6,7,8,9,10,11,16,17,18,19,20,21,22,23,24,25,26,27,-1,-1,-1,-1 },           //6
{ 8,9,10,11,12,13,14,15,16,17,18,19,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },   //7
{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27 },           //8
{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,-1,-1,-1,-1 }            //9
};

/* Desítky minut */
int digitMin2[10][28] = {
{ 28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1 }, //0
{ 28,29,30,31,48,49,50,51,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, //1
{ 32,33,34,35,36,37,38,39,44,45,46,47,48,49,50,51,52,53,54,55,-1,-1,-1,-1,-1,-1,-1,-1 }, //2
{ 28,29,30,31,32,33,34,35,44,45,46,47,48,49,50,51,52,53,54,55,-1,-1,-1,-1,-1,-1,-1,-1 }, //3
{ 28,29,30,31,40,41,42,43,48,49,50,51,52,53,54,55,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, //4
{ 28,29,30,31,32,33,34,35,40,41,42,43,44,45,46,47,52,53,54,55,-1,-1,-1,-1,-1,-1,-1,-1 }, //5
{ 28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,52,53,54,55,-1,-1,-1,-1 }, //6
{ 28,29,30,31,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, //7
{ 28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55 }, //8
{ 28,29,30,31,32,33,34,35,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,-1,-1,-1,-1 }  //9
};

/* Jednotky hodin */
int digitHod1[10][28] = {
{ 64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,-1,-1,-1,-1 }, //0
{ 72,73,74,75,76,77,78,79,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, //1
{ 60,61,62,63,68,69,70,71,72,73,74,75,80,81,82,83,84,85,86,87,-1,-1,-1,-1,-1,-1,-1,-1 }, //2
{ 60,61,62,63,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,-1,-1,-1,-1,-1,-1,-1,-1 }, //3
{ 60,61,62,63,64,65,66,67,72,73,74,75,76,77,78,79,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, //4
{ 60,61,62,63,64,65,66,67,68,69,70,71,76,77,78,79,80,81,82,83,-1,-1,-1,-1,-1,-1,-1,-1 }, //5
{ 60,61,62,63,64,65,66,67,68,69,70,71,76,77,78,79,80,81,82,83,84,85,86,87,-1,-1,-1,-1 }, //6
{ 68,69,70,71,72,73,74,75,76,77,78,79,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }, //7
{ 60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87 }, //8
{ 60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,-1,-1,-1,-1 }  //9
};

/* Desítky hodin */
int digitHod2[10][28] = {
{ 88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,-1,-1,-1,-1 },      //0
{ 88,89,90,91,108,109,110,111,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },              //1
{ 92,93,94,95,96,97,98,99,104,105,106,107,108,109,110,111,112,113,114,115,-1,-1,-1,-1,-1,-1,-1,-1 },      //2
{ 88,89,90,91,92,93,94,95,104,105,106,107,108,109,110,111,112,113,114,115,-1,-1,-1,-1,-1,-1,-1,-1 },      //3
{ 88,89,90,91,100,101,102,103,108,109,110,111,112,113,114,115,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },      //4
{ 88,89,90,91,92,93,94,95,100,101,102,103,104,105,106,107,112,113,114,115,-1,-1,-1,-1,-1,-1,-1,-1 },      //5
{ 88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,112,113,114,115,-1,-1,-1,-1 },      //6
{ 88,89,90,91,104,105,106,107,108,109,110,111,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },          //7
{ 88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115 },  //8
{ 88,89,90,91,92,93,94,95,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,-1,-1,-1,-1 }   //9
};

/* Jednotky minuty */
void displayMin1(int cislo){
  for(int i = 0; i < 28; i++){
    leds[digitMin1[cislo][i]] = color;
  }
}

/* Desítky minut */
void displayMin2(int cislo){
  for(int i = 0; i < 28; i++){
    leds[digitMin2[cislo][i]] = color;
  }
}

/* Dvojtečka */
void dvojteckaON(){
  for(int z = 56; z <= 59; z++){
    leds[z] = color;
  }
}

void dvojteckaOFF(){
  for(int z = 56; z <= 59; z++){
    leds[z] = CRGB::Black;
  }
}

/* Jednotky hodin */
void displayHod1(int cislo){
  for(int i = 0; i < 28; i++){
    leds[digitHod1[cislo][i]] = color;
  }
}

/* Desítky hodin */
void displayHod2(int cislo){
  for(int i = 0; i < 28; i++){
    leds[digitHod2[cislo][i]] = color;
  }
}

/* Vymazání celého displeje */
  void displayBlack(){
    for(int i = 0; i < NUM_LEDS; i++){
      leds[i] = CRGB::Black;
    }

  }

/* Funkce pro zobrazení času přes LED pásek WS2812B */
void displayTime(int h, int m){
  displayMin1(m % 10);
  displayMin2(m / 10);
  displayHod1(h % 10);
  displayHod2(h / 10);
}

/* Funkce, smyčka, která se neustále opakuje v intervalu 1 sekundy -> delay(1000), tedy 1 000 ms -> 1 sekunda */
void loop(){

  /* Vytvoření proměnné pro data uložená v JSON formátu */
  String jsondata;

  /* "OTA" začne naslouchat */
  ArduinoOTA.handle();

  /* Vyčištění klientů pro naslouchání WebSocketu */
  ws.cleanupClients();

  /* Pokud se timeClient neupdatuje, tak bude silou updatován */
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }

  /* Do proměnné formattedDate se uloží formátovaný čas z NTPClienta */
  formattedDate = timeClient.getFormattedTime();
  /*Serial.printf("%s\n", formattedDate);*/
  
  /* Vytažení (extrakce) dat v podobě HH:MM:SS*/
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);

    /* Deklarace proměnných, do kterých je uložen čas z NTP Clienta */
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();

  //dvojteckaON();
  //  dvojteckaON();
    unsigned long currentMillis = millis();
    unsigned long previousMillis = 0;
    if (currentMillis - previousMillis >= 1000) {
      // save the last time you blinked the LED
    previousMillis = currentMillis;
      displayBlack();
      displayTime(h,m);
      dvojteckaON();
    } 
  FastLED.show();

  /* Statický dokument v JSONu, slouží ESP pro příjem času a uploadování tohoto času na stránku index.html */
  StaticJsonDocument<200> doc;
  doc["time"] = dayStamp.c_str();
  serializeJson(doc, jsondata);
  
  /* Poslání JSON dat skrze WebSocket */
  events.send(jsondata.c_str());
  doc.clear();
}
