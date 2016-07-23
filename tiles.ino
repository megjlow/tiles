#include <ESP8266WiFi.h>
#include <FS.h>
#include <Httpd.h>
#include <Configuration.h>

const char* ssid = "";
const char* password = "";

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
                "async: false,"
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


// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
Httpd httpd = Httpd();

void setup() {
  Serial.begin(115200);
  delay(10);

  SPIFFS.begin();
  Serial.println("spiffs started");


  Configuration* config = new Configuration("/config.txt");
  char* hostname = config->getConfigurationSetting("hostname");
  if(hostname != NULL) {
    Serial.print("got hostname: ");
    Serial.println(hostname);
  }
  else {
    Serial.println("failed to retrieve hostname");
    hostname = "unknown";
  }

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
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  char* buffer = new char[4096];
  memset(buffer, 0, sizeof(buffer));

  int bytesavailable = client.available();

  client.readBytes(buffer, bytesavailable);
  buffer[bytesavailable + 1] = '\0';

  HttpRequest* req = httpd.parseRequest(buffer);

  Serial.println(buffer);

  Serial.print("after parse ");
  Serial.println(ESP.getFreeHeap());

  delete buffer;

  String url = String(req->getRequestUrl());
  String method = String(req->getRequestMethod());


  if(method.indexOf("GET") != -1) {
    
    File f = SPIFFS.open(url, "r");
    if(f) {
      HttpResponse* response = httpd.createResponse("200", 0);
      response->sendFile(client, f);
      delete response;
    }
    else if(url.indexOf("gpio") == -1) {
      Serial.println("not found");
      HttpResponse* response = httpd.createResponse("404", 0);
      client.print(response->getResponse());
      delete response;
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
      char* fname = req->getParameter("filename");
      if(fname != NULL) {
        File f = SPIFFS.open(fname, "w");
        f.print(req->getParameter("contents"));
        f.close();
      }
      HttpResponse* resp = httpd.createResponse("200", 1);
      client.print(resp->getResponse());
      delete resp;
    }
    else {
      int pin = req->getPinNumber();
      int setting = req->getPinSetting();
      if(pin == 2 || pin == 12 || pin == 13 || pin == 14) {
        /*
        Serial.print("setting digital pin #");
        Serial.print(pin);
        Serial.print(" to ");
        Serial.println(setting);
        */
        digitalWrite(pin, setting);
        HttpResponse* resp = httpd.createResponse("200", digitalRead(pin));
        client.print(resp->getResponse());
        delete resp;
        /*
        Serial.print("digital pin #");
        Serial.print(pin);
        Serial.print(" is set to ");
        Serial.println(digitalRead(pin));
        */
      }
      else if(pin == 4 || pin == 5) {
        /*
        Serial.print("setting pwm pin #");
        Serial.print(pin);
        Serial.print(" to ");
        Serial.println(setting);
        */
        analogWrite(pin, setting); 
        HttpResponse* resp = httpd.createResponse("200", analogRead(pin));
        client.print(resp->getResponse());
        delete resp;
        /*
        Serial.print("pwm pin #");
        Serial.print(pin);
        Serial.print(" is set to ");
        Serial.println(analogRead(pin));
        */
      }
    }
  }

  delay(1);
  delete req;
} 
