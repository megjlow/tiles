#include <Httpd.h>
#include <Utils.h>

#ifdef ARDUINO_STM32_FEATHER
#include <adafruit_feather.h>
#endif

// put your network ssid in here
const char* ssid = "";
// and your network password here
const char* password = "";


// Create an instance of the server
// specify the port to listen on as an argument
//WiFiServer* server = new WiFiServer(80);

httpd::sockets::ServerSocket* server = new httpd::sockets::ServerSocket(80);


Httpd* h = new Httpd(server);
int dataDelay = 10;
//WiFiUDP udp;
//IPAddress broadcast;

void HandleRoot(HttpContext* context) {
  context->response()->setResponseCode("HTTP/1.1 200 OK");
  HttpHeader* header = new HttpHeader("Content-Type", "text/html; charset=utf-8");
  context->response()->addHeader("Access-Control-Allow-Origin", "*");
  context->response()->addHeader(header);
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
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    // Print the IP address
    Serial.println(WiFi.localIP());

#endif
  h->RegisterCallback("/index.html", (Callback)HandleRoot);
  h->RegisterCallback("/gpio", (Callback)Pin, true);

  // Start the server
  h->begin();
  Serial.println("Server started");
}


void loop() {
  h->handleClient();

  /*
    Serial.println(ESP.getFreeHeap());
    Serial.println();
  */
}

#ifdef ARDUINO_STM32_FEATHER
bool connectAP(void)
{
  // Attempt to connect to an AP
  Serial.print("Please wait while connecting to: '" WLAN_SSID "' ... ");

  if ( Feather.connect(ssid, password, ENC_TYPE_AUTO) )
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

