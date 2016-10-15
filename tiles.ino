#include <Httpd.h>
#include <Utils.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <Configuration.h>

extern "C" {
#include "user_interface.h"
}

char* PROGMEM configHtml = "<html charset=\"utf-8\">"
"<head></head>"
"<body>"
"<form action=\"/config.html\" method=\"post\">"
"<label for=\"name\">NAME</label><input type=\"text\" id=\"name\" name=\"name\" /> <br />"
"<label for=\"ssid\">SSID</label><input type=\"text\" id=\"ssid\" name=\"ssid\" /> <br />"
"<label for=\"password\">PASSWORD</label><input type=\"text\" id=\"password\" name=\"password\" /><br />"
"<input type=\"submit\" />"
"</form>"
"</body>"
"</html>";



#ifdef ARDUINO_STM32_FEATHER
#include <adafruit_feather.h>
#endif


// put your network ssid in here
const char* ssid = "tiles";
// and your network password here
const char* password = "peekaboo";


httpd::sockets::ServerSocket* server = new httpd::sockets::ServerSocket(80);


Httpd* h = new Httpd(server);
WiFiUDP udp;
IPAddress broadcast;
os_timer_t heartBeatTimer;

// send a UDP broadcast on port 2048 with our chip id and IP address
void heartBeat(void* pArg) {
  Serial.println("advertise");
  udp.beginPacket(broadcast, 2048);
  udp.print("ESP-");
  udp.print(ESP.getChipId());
  udp.print(": ");
  udp.print(WiFi.localIP());
  udp.endPacket();
}

void HandleRoot(HttpContext* context) {
  char* url = context->request()->url();
  char* method = context->request()->method();
  
  context->response()->setResponseCode("HTTP/1.1 200 OK");
  context->response()->addHeader("Access-Control-Allow-Origin", "*");
  context->response()->addHeader("Content-Type", "text/html; charset=utf-8");
  context->response()->setBody("<html><head></head><body>It works!</body></html>");
}

void Pin(HttpContext* context) {
  String s = String(context->request()->url());
  int i = s.indexOf("gpio");
  String p = s.substring(i + 4);
  int pin = p.toInt();

  if (strcmp(context->request()->method(), "POST") == 0) {
    Array<char>* arr = Utils::tokeniseString(context->request()->url(), "/");
    // first element should be gpioX, second should be setting
    if (arr->count() >= 2) {
      String s = String(arr->get(1));
      int setting = s.toInt();
      // we're treating 2, 12, 13, 14 as digital pins and 4, 5 as pwm pins
      if (pin == 2 || pin == 12 || pin == 13 || pin == 14) {
        digitalWrite(pin, setting);
      }
      else if (pin == 4 || pin == 5) {
        if(setting > 1023) {
          setting = 1023;
        }
        if(setting < 0) {
          setting = 0;
        }
        analogWrite(pin, setting);
      }
    }
    delete arr;
  }
  context->response()->setResponseCode("HTTP/1.1 200 OK");
  context->response()->addHeader("Content-Type", "application/json");
  context->response()->addHeader("Access-Control-Allow-Origin", "*");
  int pinSetting = 0;
  if (pin == 2 || pin == 12 || pin == 13 || pin == 14) {
    pinSetting = digitalRead(pin);
  }
  else if (pin == 4 || pin == 5) {
    pinSetting = analogRead(pin);
  }

  String b = String("{\"value\":\"");
  b.concat(pinSetting);
  b.concat("\"}");
  context->response()->setBody(b);
}


void Config(HttpContext* context) {
  if(strcmp(context->request()->method(), "GET") == 0) {
    context->response()->setResponseCode("HTTP/1.1 200 OK");
    HttpHeader* header = new HttpHeader("Content-Type", "text/html; charset=utf-8");
    context->response()->addHeader("Access-Control-Allow-Origin", "*");
    context->response()->addHeader(header);
    context->response()->setBody(configHtml);
  }
  else {
    char* ssid = context->request()->getParameter("ssid");
    char* pass = context->request()->getParameter("password");
    char* name = context->request()->getParameter("name");
    if(ssid != NULL && name != NULL) {
      File f = SPIFFS.open("/config.txt", "w+");
      f.print("ssid="); f.print(ssid); f.print("\r\n");
      if(pass != NULL) {
        f.print("password="); f.print(password); f.print("\r\n");
      }
      f.print("name="); f.print(name); f.print("\r\n");
      f.close();
      context->response()->setResponseCode("HTTP/1.1 200 OK");
      context->response()->addHeader("Content-Type", "text/html; charset=utf-8");
      context->response()->addHeader("Access-Control-Allow-Origin", "*");
      context->response()->setBody("<html charset=\"utf-8\"><head></head><body>Updated</body></html>");
    }
    else {
      context->response()->setResponseCode("HTTP/1.1 400 Bad Request");
      context->response()->setBody("<html charset=\"utf-8\"><head></head><body>Missing Parameter(s)</body></html>");
    }
  }
}

void Root(HttpContext* context) {
  context->response()->setResponseCode("HTTP/1.1 404 Not Found");
  context->response()->addHeader("Content-Type", "text/html; charset=utf-8");
  context->response()->addHeader("Access-Control-Allow-Origin", "*");
  context->response()->setBody("<html charset=\"utf-8\"><head></head><body>404 Not Found</body></html>");
}

void setup() {
  Serial.begin(115200);
  
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  
#ifdef ARDUINO_STM32_FEATHER
// code for connecting using the adafruit wiced feather
  while (!Serial) delay(1);

  Serial.println("TCP Server Example (Callbacks)\r\n");

  // Print all software versions
  Feather.printVersions();

  while ( !connectAP() )
  {
    delay(500); // delay between each attempt
  }

  // Connected: Print network info
  Feather.printNetwork();
#elif ESP8266
  // code for connecting with adafruit feather huzzah or nodemcu
    // Connect to WiFi network
    WiFi.mode(WIFI_STA);
    SPIFFS.begin();

    Configuration* config = new Configuration("/config.txt");
    char* configuredSsid = config->get("ssid");
    char* configuredPassword = config->get("password");
    char* configuredHostname = config->get("name");

    if(configuredHostname != NULL) {
      WiFi.hostname(configuredHostname);
    }

    delete config;
    
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");

    if(configuredSsid != NULL) {
      Serial.println(configuredSsid);
      if(configuredPassword != NULL) {
        WiFi.begin(configuredSsid, configuredPassword);
      }
      else {
        WiFi.begin(configuredSsid);
      }
    }
    else {
      Serial.println(ssid);
      WiFi.begin(ssid, password);
    }

    bool connected = false;
    int connectAttempts = 0;
    while(!connected) {
      connected = WiFi.status() == WL_CONNECTED;
      if(!connected) {
        delay(500);
        Serial.print(".");
        connectAttempts++;
        if(connectAttempts > 60) {
          break;
        }
      }
    }
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      connectAttempts++;
      if(connectAttempts > 5) {
        break;
      }
    }

    if(connected) {
      Serial.println("");
      Serial.println("WiFi connected");

      // Print the IP address
      Serial.println(WiFi.localIP());
    }
    else {
      // failed to connect to the network start in AP mode
      Serial.println("connect failed");
      WiFi.mode(WIFI_AP);
      char apSsid[50];
      memset(apSsid, 0, sizeof(apSsid));
      sprintf(apSsid, "esp8266-%06x", ESP.getChipId());
      Serial.println(apSsid);
      IPAddress apIP(192, 168, 10, 1);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP(apSsid, "tiles-for-tales");
    }

#endif
  broadcast = ~WiFi.subnetMask() | WiFi.gatewayIP();
  // got the network broadcast address, send a UDP broadcst with out IP address then start a timer to send this every 60 seconds
  heartBeat(NULL);
  os_timer_setfn(&heartBeatTimer, heartBeat, NULL);
  os_timer_arm(&heartBeatTimer, 60000, true);

  h->RegisterCallback("/index.html", (Callback)HandleRoot);
  h->RegisterCallback("/gpio", (Callback)Pin, true);
  h->RegisterCallback("/config.html", (Callback)Config);
  h->RegisterCallback("/", (Callback)Root, true); // this is the last one that will be checked and should match anything

  // Start the server
  h->begin();
  Serial.println("Server started");
}


void loop() {
  h->handleClient();
}

#ifdef ARDUINO_STM32_FEATHER
bool connectAP(void)
{
  // Attempt to connect to an AP
  Serial.print("Please wait while connecting to: '");
  Serial.print(ssid);
  Serial.print(" ... ");

  if ( Feather.connect(ssid, password, ENC_TYPE_WPA2_AES) )
  {
    Serial.println("Connected!");
  }
  else
  {
    Serial.printf("Failed! %s (%d)", Feather.errstr(), Feather.errno());
    Serial.println();
  }
  Serial.println();

  return Feather.connected();
}
#endif

