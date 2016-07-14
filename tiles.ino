#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <Httpd.h>
#include <Configuration.h>

const char* ssid = "darkblack";
const char* password = "peekaboo123";

const char *upload PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nAccess-Control-Allow-Origin: *\r\n\r\n"
"<!DOCTYPE html>"
"<html lang=\"en-US\">"
 "<head>"
    "<meta charset=\"UTF-8\">" 
    "<title>File upload</title>"
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.0.0/jquery.min.js'></script>"
      "<script type='text/javascript'>"
          "$(document).ready(function() {"
            ""
            "$('#upload').on('click', function(e) {"
              "e.preventDefault();"
              "var filename = $('#filename').val();"
              "$.ajax({"
                "type: \"POST\","
                "url: 'http://' + location.host + '/uploadfile',"
                "data: {"
                  "filename: filename,"
                  "contents: $('#textarea').val()"
                "},"
                "success: function(response) {"
                "}"
              "});"
            "});"
            ""
          "});"
        "</script>"
  "</head>"
  "<body>"
    "<label>Filename:</label><input type=\"text\" id=\"filename\" />"
    "<textarea id=\"textarea\" rows=\"10\" columns=\"200\" style=\"display:block;\"></textarea>"
    "<a href=\"javascript:void(0);\" id=\"upload\">Upload</a>"
  "</body>"
"</html>";

const char *cors PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\n\r\n"
"<?xml version=\"1.0\" ?>\n"
"<cross-domain-policy>\n"
"<allow-access-from domain=\"*\" />\n"
"</cross-domain-policy>";


const char *idx PROGMEM =  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
"<html>"
 "<head>"
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.0.0/jquery.min.js'></script>"
    "<script type='text/javascript'>\n"
      "$(document).ready(function() {\n"
        "$.ajax({\n"
          "type: 'GET',\n"
          "url: 'http://' + location.host + '/gpio2',\n"
          "success: function(response) {\n"
            "if(response.value == '1') {\n"
                "$('#state').text('OFF');\n"
              "}\n"
              "else {\n"
                "$('#state').text('ON');\n"
              "}\n"
          "}\n"
        "});\n"
        "$.ajax({\n"
          "type: 'GET',\n"
          "url: 'http://' + location.host + '/gpio4',\n"
          "success: function(response) {\n"
                "$('#powerstate').text(response.value);\n"
          "}\n"
        "});\n"
        "$('#on').on('click', function(e) {\n"
          "e.preventDefault();\n"
          "$.ajax({\n"
            "type: 'POST',\n"
            "dataType: 'json',\n"
            "url: 'http://' + location.host + '/gpio2/0',\n"
            "success: function(response) {\n"
              "if(response.value == '0') {\n"
                "$('#state').text('ON');\n"
              "}\n"
              "else {\n"
                "$('#state').text('OFF');\n"
              "}\n"
            "}\n"
          "});\n"
        "});\n"
        "$('#off').on('click', function(e) {\n"
          "e.preventDefault();\n"
          "$.ajax({\n"
            "type: 'POST',\n"
            "url: 'http://' + location.host + '/gpio2/1',\n"
            "success: function(response) {\n"
              "if(response.value == '1') {\n"
                "$('#state').text('OFF');\n"
              "}\n"
              "else {\n"
                "$('#state').text('ON');\n"
              "}\n"
            "}\n"
          "});\n"
        "});\n"
         "$('#setpower').on('click', function(e) {\n"
           "e.preventDefault();\n"
           "var level = $('#setting').val();\n"
           "$.ajax({\n"
            "type: 'POST',\n"
            "url: 'http://' + location.host + '/gpio4/' + level,\n"
            "success: function(response) {\n"
              "$('#powerstate').text(response.value);\n"
            "}\n"
          "});\n"
         "});\n"
      "});\n"
    "</script>"
  "</head>"
  "<body>"
    "<a href=\"javascript:void(0);\" id=\"on\">ON</a>&nbsp;&nbsp;&nbsp;&nbsp;"
    "<a href=\"javascript:void(0);\" id=\"off\">OFF</a>&nbsp;&nbsp;&nbsp;&nbsp;"
    "<label id=\"state\">OFF</label>\n"
    "<br />"
    "<br />"
    "<a href=\"javascript:void(0);\" id=\"setpower\">Set</a>&nbsp;&nbsp;&nbsp;"
    "<input type=\"text\" id=\"setting\" />&nbsp;&nbsp;&nbsp;"
    "<label id=\"powerstate\">0</label\n"
  "</body>"
"</html>";

const char* js PROGMEM =  "HTTP/1.1 200 OK\r\nContent-Type: application/javascript; charset=UTF-8\r\nAccess-Control-Allow-Origin: *\r\n\r\n"
"new (function() {\n"
  "var ext = this;\n"
  "var descriptor = {\n"
    "blocks: [\n"
      "[' ', 'SUN: digital pin %m.pin setting %m.dsetting', 'setDigital', '1', 'off'],\n"
      "[' ', 'pwm pin %m.ppin setting %n', 'setPwm', '1', '100'],\n"
      "[' ', 'digital pin %m.pin get', 'getDigital', '1'],\n"
      "[' ', 'pwm pin %m.ppin get', 'getPwm', '1']\n"
    "],\n"
    "'menus': {\n"
      "'pin': ['1', '2', '3', '4'],\n"
      "'dsetting': ['on', 'off'],\n"
      "'ppin': ['1', '2']\n"
     "},\n"
    "url: 'https://github.com/savaka2/scratch-extensions/wiki/Link-Opener-extension'\n"
  "};\n"
  "ext._shutdown = function() {};"
  "ext._getStatus = function() {"
    "return {status:2, msg:'Ready'};"
  "};"
  "ext.getPwm = function(pin) {\n"
  "};\n"
  "ext.setPwm = function(pin, setting) {\n"
  "};\n"
  "ext.getDigital = function(pin) {\n"
  "};\n"
  "ext.setDigital = function(setting, url) {"
    "alert(url);"
    "var url = 'http://%%%%%%/gpio2/' + setting; "
"console.log('setting' + setting);"
"$.ajax({"
"type: 'POST'," 
"url: url," 
"success: function(response) {"
"alert('success');"
"}"
"});"
"};"
"ScratchExtensions.register('Link Opener', descriptor, ext);})();";

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
Httpd httpd = Httpd();
WiFiUDP udp = WiFiUDP();
unsigned long lastUpdateTime = 0;
unsigned long updatePeriod = 30000;

void setup() {
  Serial.begin(115200);
  delay(10);

  SPIFFS.begin();
  Serial.println("spiffs started");

  Configuration* config = new Configuration("/config.txt");
Serial.println("ocnfig constructed");
  char* hostname = config->getConfigurationSetting("hostname");
  if(hostname != NULL) {
    Serial.print("got hostname: ");
    Serial.println(hostname);
  }
  else {
    Serial.println("failed to retrieve hostname");
    hostname = "unknown";
  }
  
/*
  File f = SPIFFS.open("/config.txt", "w");
  if(f) {
    f.println("hostname=SUN");
    f.println("ip=192.168.2.102");
    f.close();
  }
*/
  /*
  SPIFFS.format();
  Serial.println("format complete");
  SPIFFS.end();
  */
  
  /*
  File f = SPIFFS.open("/config.txt", "r");
  if(f) {
    Serial.println("got config file");
    String line = f.readStringUntil('\n');
    Serial.print("got");
    Serial.println(line);
  }
  */

  /*
  File f = SPIFFS.open("/crossdomain.xml", "w");
  if(f) {
    f.println(cors);
    f.close();
  }

  SPIFFS.end();
  */

  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 0);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  delete config;
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    unsigned long currentTime = millis();

    /*
    // send a broadcast packet every thirty seconds
    if((currentTime - lastUpdateTime) > updatePeriod) {
      lastUpdateTime = currentTime;
      Serial.println("time to register");
      IPAddress addr = IPAddress(192, 168, 2, 255);
      udp.beginPacketMulticast(addr, 2121, addr);
      udp.write("Hello\n");
      udp.endPacket();
    }
    */

    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }

  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));

  int bytesavailable = client.available();

  client.readBytes(buffer, bytesavailable);
  buffer[bytesavailable + 1] = '\0';

  HttpRequest* req = httpd.parseRequest(buffer);

  String url = String(req->getRequestUrl());
  String method = String(req->getRequestMethod());
  Serial.println(url);

  //WiFi.localIP().toString().toCharArray(buf, 50);

  if(method.indexOf("GET") != -1) {
    File f = SPIFFS.open(url, "r");
    if(f) {
      char buffer[256];
      int available = 0;
      size_t read = 0;
      while((available = f.available()) > 0) {
        if(available > sizeof(buffer)) {
          read = f.readBytes((char*)buffer, sizeof(buffer));
        }
        else {
          read = f.readBytes((char*)buffer, available);
        }
        client.write((char*)buffer, read);
      }
      f.close();
    }
    // return the test page
    if(url.indexOf("test.html") != -1) {
      client.print(idx);
    }
    else if(url.indexOf("fileupload.html") != -1) {
      client.print(upload);
    }
    else if(url.indexOf("extension.js") != -1) {
      char buf[50];
      WiFi.localIP().toString().toCharArray(buf, 50);
      String b = String(js);
      String c = String(buf);
      b.replace("%%%%%%", c);
      client.print(b);
    }
    else {
      // GET the setting of a pin
      int pin = req->getPinNumber();
      int setting = req->getPinSetting();
      if(pin == 2 || pin == 12 || pin == 13 || pin == 14) {
        // digital pins
        HttpResponse* resp = httpd.createResponse("200", digitalRead(pin));
        client.print(resp->getResponse());
        delete resp;
      }
      else if(pin == 4 || pin == 5) {
        // pwm pins
        HttpResponse* resp = httpd.createResponse("200", analogRead(pin));
        client.print(resp->getResponse());
        delete resp;
      }
    }
  }
  else if(method.indexOf("POST") != -1) {
    Serial.println("post");
    if(url.indexOf("/uploadfile") != -1) {
      Serial.println("upload file request");
      HttpResponse* resp = httpd.createResponse("200", 1);
      client.print(resp->getResponse());
      delete resp;
    }
    else {
      // WRITE the state of a pin
      int pin = req->getPinNumber();
      int setting = req->getPinSetting();
      if(pin == 2 || pin == 12 || pin == 13 || pin == 14) {
        Serial.print("setting digital pin #");
        Serial.print(pin);
        Serial.print(" to ");
        Serial.println(setting);
        digitalWrite(pin, setting);
        HttpResponse* resp = httpd.createResponse("200", digitalRead(pin));
        client.print(resp->getResponse());
        delete resp;
        Serial.print("digital pin #");
        Serial.print(pin);
        Serial.print(" is set to ");
        Serial.println(digitalRead(pin));
      }
      else if(pin == 4 || pin == 5) {
        Serial.print("setting pwm pin #");
        Serial.print(pin);
        Serial.print(" to ");
        Serial.println(setting);
        analogWrite(pin, setting); 
        HttpResponse* resp = httpd.createResponse("200", analogRead(pin));
        client.print(resp->getResponse());
        delete resp;
        Serial.print("pwm pin #");
        Serial.print(pin);
        Serial.print(" is set to ");
        Serial.println(analogRead(pin));
      }
    }
    /*
    if(url.indexOf("/gpio") != -1) {
      if(url.indexOf("/gpio2/0") != -1) {
        digitalWrite(2, 0);
        val = "high";
        Response* resp = httpd.createResponse("200", digitalRead(2));
        client.print(resp->getResponse());
        delete resp;
      }
      else if(url.indexOf("/gpio2/1") != -1) {
        digitalWrite(2, 1);
        val = "low";
        Response* resp = httpd.createResponse("200", digitalRead(2));
        client.print(resp->getResponse());
        delete resp;
      }
    }
    */
  }

  delay(1);
  delete req;

} 
