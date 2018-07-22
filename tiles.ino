#include <Httpd.h>
#include <Utils.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <Configuration.h>
#include <firmata/WSFirmata.h>

extern "C" {
#include "user_interface.h"
}

#define DEBUG

// http://scratchx.org?url=https://megjlow.github.io/extensionloader.js?ip=192.168.10.211#scratch
// https://megjlow.github.io/socket.js?ip=192.168.2.105


const char uploadHtml[] = R"=====(<html charset="utf-8">
<head>
</head>
<body>
<form action="/upload.html" method="post">
<input type="file" name="file" />
<button class="submit-button" type="submit">Upload</button>
</form>
</body>
</html>)=====";

int counter1 = 0;
int counter2 = 0;

#ifdef ARDUINO_STM32_FEATHER
#include <adafruit_feather.h>
#endif


// put your network ssid in here
const char* ssid = "tiles";
// and your network password here
const char* password = "peekaboo";

using namespace httpd;

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
  /*
  char apSsid[50];
      memset(apSsid, 0, sizeof(apSsid));
      sprintf(apSsid, "esp8266-%06x", ESP.getChipId());
   */
}

void HandleRoot(HttpContext* context) {
  char* url = context->request()->url();
  char* method = context->request()->method();
  
  context->response()->setResponseCode("HTTP/1.1 200 OK");
  context->response()->addHeader("Content-Type", "text/html; charset=utf-8");
  context->response()->setBody("<html><head></head><body>It works!</body></html>");
}

void Pin(HttpContext* context) {
  int startTime = millis();
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

  int endTime = millis();
  Serial.print("Pin elapsed: "); Serial.println(endTime - startTime);
}


void Config(HttpContext* context) {
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
    context->response()->setBody("<html charset=\"utf-8\"><head></head><body>Updated</body></html>");
  }
  else {
    context->response()->setResponseCode("HTTP/1.1 400 Bad Request");
    context->response()->setBody("<html charset=\"utf-8\"><head></head><body>Missing Parameter(s)</body></html>");
  }
}

void Upload(HttpContext* context) {
  Serial.println("here");
  delay(100);
  Serial.println(uploadHtml);
  Serial.println("and here");
  delay(250);
  if(strcmp(context->request()->method(), "GET") == 0) {
    context->response()->setResponseCode("HTTP/1.1 200 OK");
    context->response()->addHeader("Content-Type", "text/html");
    context->response()->setBody(uploadHtml);
  }
  else {
    
  }
}

void Images(HttpContext* context) {
  String fname = String(context->request()->url());
  File f = SPIFFS.open(fname, "r");
  if(!f) {
    FourOhFour(context);
  }
  else {
    context->response()->sendFile(f);
  }
  f.close();
  Serial.print("fname "); Serial.println(fname);
}

void StaticPages(HttpContext* context) {
  String fname = String(context->request()->url());
  File f = SPIFFS.open(fname, "r");
  if(!f) {
    FourOhFour(context);
  }
  else {
    context->response()->sendFile(f);
  }
  f.close();
  Serial.print("fname "); Serial.println(fname);
}

void FourOhFour(HttpContext* context) {
  context->response()->setResponseCode("HTTP/1.1 404 Not Found");
  context->response()->addHeader("Content-Type", "text/html; charset=utf-8");
  context->response()->setBody("<html charset=\"utf-8\"><head></head><body>404 Not Found</body></html>");
}

void SocketOnMessage(Socket* socket) {
  //Serial.println(socket->getMessage());
  //context->sendTextMessage("reply message");
}

/*
Servo servos[MAX_SERVOS];
byte servoPinMap[TOTAL_PINS];
byte detachedServos[MAX_SERVOS];
byte detachedServoCount = 0;
byte servoCount = 0;
*/
byte portConfigInputs[TOTAL_PORTS]; // each bit: 1 = pin in INPUT, 0 = anything else

//unsigned long currentMillis;        // store the current value from millis()
//unsigned long previousMillis;       // for comparison with currentMillis
unsigned int samplingInterval = 19; // how often to run the main loop (in ms)

/* analog inputs */
int analogInputsToReport = 0; // bitwise array to store pin reporting

boolean isResetting = false;

byte reportPINs[TOTAL_PORTS]; 
byte previousPINs[TOTAL_PORTS];

#define MINIMUM_SAMPLING_INTERVAL   1

void analogWriteCallback(byte pin, int value) {
  Serial.println("Firmata analogWriteCallback");
  if (pin < TOTAL_PINS) {
    switch (WSFirmata.getPinMode(pin)) {
      /*
      case PIN_MODE_SERVO:
        if (IS_PIN_DIGITAL(pin))
          servos[servoPinMap[pin]].write(value);
        Firmata.setPinState(pin, value);
        break;
      */
      case PIN_MODE_PWM:
        if (IS_PIN_PWM(pin))
          analogWrite(PIN_TO_PWM(pin), value);
        WSFirmata.setPinState(pin, value);
        break;
    }
  }
}

void reportAnalogCallback(byte analogPin, int value) {
  Serial.println("Firmata reportAnalogCallback");
  if (analogPin < TOTAL_ANALOG_PINS) {
    if (value == 0) {
      analogInputsToReport = analogInputsToReport & ~ (1 << analogPin);
    } else {
      analogInputsToReport = analogInputsToReport | (1 << analogPin);
      // prevent during system reset or all analog pin values will be reported
      // which may report noise for unconnected analog pins
      if (!isResetting) {
        // Send pin value immediately. This is helpful when connected via
        // ethernet, wi-fi or bluetooth so pin states can be known upon
        // reconnecting.
        WSFirmata.sendAnalog(analogPin, analogRead(analogPin));
      }
    }
  }
  // TODO: save status to EEPROM here, if changed
}

void outputPort(byte portNumber, byte portValue, byte forceSend) {
  // pins not configured as INPUT are cleared to zeros
  portValue = portValue & portConfigInputs[portNumber];
  // only send if the value is different than previously sent
  if (forceSend || previousPINs[portNumber] != portValue) {
    WSFirmata.sendDigitalPort(portNumber, portValue);
    WSFirmata.flush();
    previousPINs[portNumber] = portValue;
  }
}

void setPinValueCallback(byte pin, int value) {
  Serial.print("Firmata setPinValueCallback "); Serial.print(pin); Serial.print(" "); Serial.println(value);
  counter1++;
  if (pin < TOTAL_PINS && IS_PIN_DIGITAL(pin)) {
    if (WSFirmata.getPinMode(pin) == OUTPUT) {
      WSFirmata.setPinState(pin, value);
      digitalWrite(PIN_TO_DIGITAL(pin), value);
    }
  }
  Serial.print("counter1 "); Serial.println(counter1);
}

void reportDigitalCallback(byte port, int value)
{
  Serial.print("Firmata reportDigitalCallback "); Serial.print(port); Serial.print(" "); Serial.println(value); 
  if (port < TOTAL_PORTS) {
    reportPINs[port] = (byte)value;
    // Send port value immediately. This is helpful when connected via
    // ethernet, wi-fi or bluetooth so pin states can be known upon
    // reconnecting.
    if (value) {
      outputPort(port, readPort(port, portConfigInputs[port]), true);
    }
  }
  // do not disable analog reporting on these 8 pins, to allow some
  // pins used for digital, others analog.  Instead, allow both types
  // of reporting to be enabled, but check if the pin is configured
  // as analog when sampling the analog inputs.  Likewise, while
  // scanning digital pins, portConfigInputs will mask off values from any
  // pins configured as analog
}

void setPinModeCallback(byte pin, int mode)
{
  Serial.print("Firmata setPinModeCallback "); Serial.print(pin); Serial.print(" "); Serial.println(mode);
  if (WSFirmata.getPinMode(pin) == PIN_MODE_IGNORE)
    return;

/*
    // disable i2c so pins can be used for other functions
    // the following if statements should reconfigure the pins properly
    disableI2CPins();
  }
*/
/*
  if (IS_PIN_DIGITAL(pin) && mode != PIN_MODE_SERVO) {
    if (servoPinMap[pin] < MAX_SERVOS && servos[servoPinMap[pin]].attached()) {
      detachServo(pin);
    }
  }
  */
  if (IS_PIN_ANALOG(pin)) {
    reportAnalogCallback(PIN_TO_ANALOG(pin), mode == PIN_MODE_ANALOG ? 1 : 0); // turn on/off reporting
  }
  if (IS_PIN_DIGITAL(pin)) {
    if (mode == INPUT || mode == PIN_MODE_PULLUP) {
      portConfigInputs[pin / 8] |= (1 << (pin & 7));
    } else {
      portConfigInputs[pin / 8] &= ~(1 << (pin & 7));
    }
  }
  WSFirmata.setPinState(pin, 0);
  switch (mode) {
    case PIN_MODE_ANALOG:
      if (IS_PIN_ANALOG(pin)) {
        if (IS_PIN_DIGITAL(pin)) {
          pinMode(PIN_TO_DIGITAL(pin), INPUT);    // disable output driver
#if ARDUINO <= 100
          // deprecated since Arduino 1.0.1 - TODO: drop support in Firmata 2.6
          digitalWrite(PIN_TO_DIGITAL(pin), LOW); // disable internal pull-ups
#endif
        }
        WSFirmata.setPinMode(pin, PIN_MODE_ANALOG);
      }
      break;
    case INPUT:
      if (IS_PIN_DIGITAL(pin)) {
        pinMode(PIN_TO_DIGITAL(pin), INPUT);    // disable output driver
#if ARDUINO <= 100
        // deprecated since Arduino 1.0.1 - TODO: drop support in Firmata 2.6
        digitalWrite(PIN_TO_DIGITAL(pin), LOW); // disable internal pull-ups
#endif
        WSFirmata.setPinMode(pin, INPUT);
      }
      break;
    case PIN_MODE_PULLUP:
      if (IS_PIN_DIGITAL(pin)) {
        pinMode(PIN_TO_DIGITAL(pin), INPUT_PULLUP);
        WSFirmata.setPinMode(pin, PIN_MODE_PULLUP);
        WSFirmata.setPinState(pin, 1);
      }
      break;
    case OUTPUT:
      if (IS_PIN_DIGITAL(pin)) {
        if (WSFirmata.getPinMode(pin) == PIN_MODE_PWM) {
          // Disable PWM if pin mode was previously set to PWM.
          analogWrite(PIN_TO_DIGITAL(pin), 0);
          //digitalWrite(PIN_TO_DIGITAL(pin), LOW);
        }
        pinMode(PIN_TO_DIGITAL(pin), OUTPUT);
        WSFirmata.setPinMode(pin, OUTPUT);
      }
      break;
    case PIN_MODE_PWM:
      if (IS_PIN_PWM(pin)) {
        pinMode(PIN_TO_PWM(pin), OUTPUT);
        analogWrite(PIN_TO_PWM(pin), 0);
        WSFirmata.setPinMode(pin, PIN_MODE_PWM);
      }
      break;
      /*
    case PIN_MODE_SERVO:
      if (IS_PIN_DIGITAL(pin)) {
        Firmata.setPinMode(pin, PIN_MODE_SERVO);
        if (servoPinMap[pin] == 255 || !servos[servoPinMap[pin]].attached()) {
          // pass -1 for min and max pulse values to use default values set
          // by Servo library
          attachServo(pin, -1, -1);
        }
      }
      break;
    case PIN_MODE_I2C:
      if (IS_PIN_I2C(pin)) {
        // mark the pin as i2c
        // the user must call I2C_CONFIG to enable I2C for a device
        Firmata.setPinMode(pin, PIN_MODE_I2C);
      }
      break;
      */
    case PIN_MODE_SERIAL:
#ifdef FIRMATA_SERIAL_FEATURE
      serialFeature.handlePinMode(pin, PIN_MODE_SERIAL);
#endif
      break;
    default:
      WSFirmata.sendString("Unknown pin mode"); // TODO: put error msgs in EEPROM
  }
  // TODO: save status to EEPROM here, if changed
}

void sysexCallback(byte command, byte argc, byte *argv) {
  byte mode;
  byte stopTX;
  byte slaveAddress;
  byte data;
  int slaveRegister;
  unsigned int delayTime;

  Serial.println("FIRMATA SYSEX CALLBACK");

  switch (command) {
    /*
    case I2C_REQUEST:
      mode = argv[1] & I2C_READ_WRITE_MODE_MASK;
      if (argv[1] & I2C_10BIT_ADDRESS_MODE_MASK) {
        Firmata.sendString("10-bit addressing not supported");
        return;
      }
      else {
        slaveAddress = argv[0];
      }

      // need to invert the logic here since 0 will be default for client
      // libraries that have not updated to add support for restart tx
      if (argv[1] & I2C_END_TX_MASK) {
        stopTX = I2C_RESTART_TX;
      }
      else {
        stopTX = I2C_STOP_TX; // default
      }

      switch (mode) {
        case I2C_WRITE:
          Wire.beginTransmission(slaveAddress);
          for (byte i = 2; i < argc; i += 2) {
            data = argv[i] + (argv[i + 1] << 7);
            wireWrite(data);
          }
          Wire.endTransmission();
          delayMicroseconds(70);
          break;
        case I2C_READ:
          if (argc == 6) {
            // a slave register is specified
            slaveRegister = argv[2] + (argv[3] << 7);
            data = argv[4] + (argv[5] << 7);  // bytes to read
          }
          else {
            // a slave register is NOT specified
            slaveRegister = I2C_REGISTER_NOT_SPECIFIED;
            data = argv[2] + (argv[3] << 7);  // bytes to read
          }
          readAndReportData(slaveAddress, (int)slaveRegister, data, stopTX);
          break;
        case I2C_READ_CONTINUOUSLY:
          if ((queryIndex + 1) >= I2C_MAX_QUERIES) {
            // too many queries, just ignore
            Firmata.sendString("too many queries");
            break;
          }
          if (argc == 6) {
            // a slave register is specified
            slaveRegister = argv[2] + (argv[3] << 7);
            data = argv[4] + (argv[5] << 7);  // bytes to read
          }
          else {
            // a slave register is NOT specified
            slaveRegister = (int)I2C_REGISTER_NOT_SPECIFIED;
            data = argv[2] + (argv[3] << 7);  // bytes to read
          }
          queryIndex++;
          query[queryIndex].addr = slaveAddress;
          query[queryIndex].reg = slaveRegister;
          query[queryIndex].bytes = data;
          query[queryIndex].stopTX = stopTX;
          break;
        case I2C_STOP_READING:
          byte queryIndexToSkip;
          // if read continuous mode is enabled for only 1 i2c device, disable
          // read continuous reporting for that device
          if (queryIndex <= 0) {
            queryIndex = -1;
          } else {
            queryIndexToSkip = 0;
            // if read continuous mode is enabled for multiple devices,
            // determine which device to stop reading and remove it's data from
            // the array, shifiting other array data to fill the space
            for (byte i = 0; i < queryIndex + 1; i++) {
              if (query[i].addr == slaveAddress) {
                queryIndexToSkip = i;
                break;
              }
            }

            for (byte i = queryIndexToSkip; i < queryIndex + 1; i++) {
              if (i < I2C_MAX_QUERIES) {
                query[i].addr = query[i + 1].addr;
                query[i].reg = query[i + 1].reg;
                query[i].bytes = query[i + 1].bytes;
                query[i].stopTX = query[i + 1].stopTX;
              }
            }
            queryIndex--;
          }
          break;
        default:
          break;
      }
      break;
    case I2C_CONFIG:
      delayTime = (argv[0] + (argv[1] << 7));

      if (argc > 1 && delayTime > 0) {
        i2cReadDelayTime = delayTime;
      }

      if (!isI2CEnabled) {
        enableI2CPins();
      }

      break;
    */
    /*
    case SERVO_CONFIG:
      if (argc > 4) {
        // these vars are here for clarity, they'll optimized away by the compiler
        byte pin = argv[0];
        int minPulse = argv[1] + (argv[2] << 7);
        int maxPulse = argv[3] + (argv[4] << 7);

        if (IS_PIN_DIGITAL(pin)) {
          if (servoPinMap[pin] < MAX_SERVOS && servos[servoPinMap[pin]].attached()) {
            detachServo(pin);
          }
          attachServo(pin, minPulse, maxPulse);
          setPinModeCallback(pin, PIN_MODE_SERVO);
        }
      }
      break;
      */
    case SAMPLING_INTERVAL:
      Serial.println("SYSEX CALLBACK SAMPLING_INTERVAL");
      if (argc > 1) {
        samplingInterval = argv[0] + (argv[1] << 7);
        if (samplingInterval < MINIMUM_SAMPLING_INTERVAL) {
          samplingInterval = MINIMUM_SAMPLING_INTERVAL;
        }
      } else {
        //Firmata.sendString("Not enough data");
      }
      break;
    case EXTENDED_ANALOG:
      Serial.println("SYSEX CALLBACK EXTENDED_ANALOG");
      if (argc > 1) {
        int val = argv[1];
        if (argc > 2) val |= (argv[2] << 7);
        if (argc > 3) val |= (argv[3] << 14);
        analogWriteCallback(argv[0], val);
      }
      break;
    case CAPABILITY_QUERY:
      Serial.println("SYSEX CALLBACK CAPABILITY_QUERY");
      WSFirmata.write(START_SYSEX);
      WSFirmata.write(CAPABILITY_RESPONSE);
      for (byte pin = 0; pin < TOTAL_PINS; pin++) {
        if (IS_PIN_DIGITAL(pin)) {
          WSFirmata.write((byte)INPUT);
          WSFirmata.write(1);
          WSFirmata.write((byte)PIN_MODE_PULLUP);
          WSFirmata.write(1);
          WSFirmata.write((byte)OUTPUT);
          WSFirmata.write(1);
        }
        if (IS_PIN_ANALOG(pin)) {
          WSFirmata.write(PIN_MODE_ANALOG);
          WSFirmata.write(10); // 10 = 10-bit resolution
        }
        if (IS_PIN_PWM(pin)) {
          WSFirmata.write(PIN_MODE_PWM);
          WSFirmata.write(DEFAULT_PWM_RESOLUTION);
        }
        if (IS_PIN_DIGITAL(pin)) {
          WSFirmata.write(PIN_MODE_SERVO);
          WSFirmata.write(14);
        }
        if (IS_PIN_I2C(pin)) {
          WSFirmata.write(PIN_MODE_I2C);
          WSFirmata.write(1);  // TODO: could assign a number to map to SCL or SDA
        }
#ifdef FIRMATA_SERIAL_FEATURE
        serialFeature.handleCapability(pin);
#endif
        WSFirmata.write(127);
      }
      WSFirmata.write(END_SYSEX);
      WSFirmata.flush();
      break;
    case PIN_STATE_QUERY:
      Serial.print("SYSEX CALLBACK PIN_STATE_QUERY ");
      if (argc > 0) {
        byte pin = argv[0];
        WSFirmata.write(START_SYSEX);
        WSFirmata.write(PIN_STATE_RESPONSE);
        WSFirmata.write(pin);
        if (pin < TOTAL_PINS) {
          WSFirmata.write(WSFirmata.getPinMode(pin));
          WSFirmata.write((byte)WSFirmata.getPinState(pin) & 0x7F);
          if (WSFirmata.getPinState(pin) & 0xFF80) {
            WSFirmata.write((byte)(WSFirmata.getPinState(pin) >> 7) & 0x7F);
          }
          if (WSFirmata.getPinState(pin) & 0xC000) {
            WSFirmata.write((byte)(WSFirmata.getPinState(pin) >> 14) & 0x7F);
          }
          WSFirmata.write(digitalRead(pin));
        }
        WSFirmata.write(END_SYSEX);
        WSFirmata.flush();
      }
      break;
    case ANALOG_MAPPING_QUERY:
      Serial.println("SYSEX CALLBACK ANALOG_MAPPING_QUERY");
      WSFirmata.write(START_SYSEX);
      WSFirmata.write(ANALOG_MAPPING_RESPONSE);
      for (byte pin = 0; pin < TOTAL_PINS; pin++) {
        WSFirmata.write(IS_PIN_ANALOG(pin) ? PIN_TO_ANALOG(pin) : 127);
      }
      WSFirmata.write(END_SYSEX);
      WSFirmata.flush();
      break;

    case SERIAL_MESSAGE:
#ifdef FIRMATA_SERIAL_FEATURE
      serialFeature.handleSysex(command, argc, argv);
#endif
      break;
  }
}

void checkDigitalInputs(void) {
  /* Using non-looping code allows constants to be given to readPort().
   * The compiler will apply substantial optimizations if the inputs
   * to readPort() are compile-time constants. */
   //Serial.println(reportPINs[12]);
  if (TOTAL_PORTS > 0 && reportPINs[0]) outputPort(0, readPort(0, portConfigInputs[0]), false);
  if (TOTAL_PORTS > 1 && reportPINs[1]) outputPort(1, readPort(1, portConfigInputs[1]), false);
  if (TOTAL_PORTS > 2 && reportPINs[2]) outputPort(2, readPort(2, portConfigInputs[2]), false);
  if (TOTAL_PORTS > 3 && reportPINs[3]) outputPort(3, readPort(3, portConfigInputs[3]), false);
  if (TOTAL_PORTS > 4 && reportPINs[4]) outputPort(4, readPort(4, portConfigInputs[4]), false);
  if (TOTAL_PORTS > 5 && reportPINs[5]) outputPort(5, readPort(5, portConfigInputs[5]), false); 
  if (TOTAL_PORTS > 6 && reportPINs[6]) outputPort(6, readPort(6, portConfigInputs[6]), false); 
  if (TOTAL_PORTS > 7 && reportPINs[7]) outputPort(7, readPort(7, portConfigInputs[7]), false); 
  if (TOTAL_PORTS > 8 && reportPINs[8]) outputPort(8, readPort(8, portConfigInputs[8]), false); 
  if (TOTAL_PORTS > 9 && reportPINs[9]) outputPort(9, readPort(9, portConfigInputs[9]), false); 
  if (TOTAL_PORTS > 10 && reportPINs[10]) outputPort(10, readPort(10, portConfigInputs[10]), false);
  if (TOTAL_PORTS > 11 && reportPINs[11]) outputPort(11, readPort(11, portConfigInputs[11]), false); 
  if (TOTAL_PORTS > 12 && reportPINs[12]) outputPort(12, readPort(12, portConfigInputs[12]), false); 
  if (TOTAL_PORTS > 13 && reportPINs[13]) outputPort(13, readPort(13, portConfigInputs[13]), false); 
  if (TOTAL_PORTS > 14 && reportPINs[14]) outputPort(14, readPort(14, portConfigInputs[14]), false); 
  if (TOTAL_PORTS > 15 && reportPINs[15]) outputPort(15, readPort(15, portConfigInputs[15]), false);
}

void interrupt(int pin) {
  if(h->firmataConnected()) {
    Serial.print("pin:");Serial.print(pin);
    WSFirmata.write(START_SYSEX);
    //Serial.print(START_SYSEX, HEX); Serial.print(" ");
    WSFirmata.write(PIN_STATE_RESPONSE);
    //Serial.print(PIN_STATE_RESPONSE, HEX);
    WSFirmata.write(pin);
    if (pin < TOTAL_PINS) {
      //Serial.print(" state:");Serial.println(Firmata.getPinState(pin));
      WSFirmata.write(WSFirmata.getPinMode(pin));
      //Serial.print(WSFirmata.getPinMode(pin), HEX); Serial.print(" ");
      WSFirmata.write((byte)WSFirmata.getPinState(pin) & 0x7F);
      //Serial.print("state: "); Serial.print((byte)Firmata.getPinState(pin) & 0x7F, HEX); Serial.println();
      //Serial.print("pin value: "); Serial.println(digitalRead(pin));
      if (WSFirmata.getPinState(pin) & 0xFF80) WSFirmata.write((byte)(WSFirmata.getPinState(pin) >> 7) & 0x7F);
      if (WSFirmata.getPinState(pin) & 0xC000) WSFirmata.write((byte)(WSFirmata.getPinState(pin) >> 14) & 0x7F);
    }
    WSFirmata.write(END_SYSEX);
    WSFirmata.flush();
  }
}

void interrupt12() {
  Serial.println("interrupt on pin 12");
  interrupt(12);
}

void interrupt13() {
  Serial.println("interrupt on pin 13");
}

void setup() {
  Serial.begin(115200);

  counter1 = 0;
  counter2 = 0;
  
  //pinMode(2, OUTPUT);
  //pinMode(4, OUTPUT);
  //pinMode(5, OUTPUT);
  //pinMode(12, OUTPUT);
  //pinMode(13, OUTPUT);
  //pinMode(14, OUTPUT);
  
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
      Serial.println(ESP.getChipId());
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
  //heartBeat(NULL);
  //os_timer_setfn(&heartBeatTimer, heartBeat, NULL);
  //os_timer_arm(&heartBeatTimer, 60000, true);

  h->addGlobalHeader("Access-Control-Allow-Origin", "*");

  h->RegisterCallback("/index.html", (Callback)HandleRoot);
  h->RegisterCallback("/config.html", "POST", (Callback)Config);
  h->RegisterCallback("/gpio", (Callback)Pin, true);
  h->RegisterCallback("/upload.html", (Callback)Upload);
  h->RegisterCallback("/images", (Callback)Images, true);
  h->RegisterCallback("/", (Callback)StaticPages, true);
  h->RegisterCallback("/", (Callback)FourOhFour, true); // this is the last one that will be checked and should match anything

  h->RegisterSocketCallback((SocketCallback)SocketOnMessage); // callback handler for websocket messages

  WSFirmata.setFirmwareVersion(FIRMATA_FIRMWARE_MAJOR_VERSION, FIRMATA_FIRMWARE_MINOR_VERSION);
        
  WSFirmata.attach(ANALOG_MESSAGE, analogWriteCallback);
  WSFirmata.attach(START_SYSEX, sysexCallback);
  WSFirmata.attach(REPORT_DIGITAL, reportDigitalCallback);
  WSFirmata.attach(SET_PIN_MODE, setPinModeCallback);
  WSFirmata.attach(SET_DIGITAL_PIN_VALUE, setPinValueCallback);

  // Start the server
  h->begin();
  Serial.println("Server started");

  Serial.print("Pins: ");Serial.println(TOTAL_PINS);

  //pinMode(12, INPUT);
  //attachInterrupt(digitalPinToInterrupt(12), interrupt12, CHANGE);  

  //pinMode(D6, INPUT);

  //reportDigitalCallback(1, 1);
  //reportDigitalCallback(2, 1);
  //reportDigitalCallback(3, 1);
  /*
Digital pins 0—15 can be INPUT, OUTPUT, or INPUT_PULLUP. Pin 16 can be INPUT, OUTPUT or INPUT_PULLDOWN_16. At startup, pins are configured as INPUT.

*/
}

/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
 */

/*
#elif defined(ESP8266)
#define TOTAL_ANALOG_PINS       NUM_ANALOG_INPUTS
#define TOTAL_PINS              A0 + NUM_ANALOG_INPUTS
#define PIN_SERIAL_RX           3
#define PIN_SERIAL_TX           1
#define IS_PIN_DIGITAL(p)       (((p) >= 0 && (p) <= 5) || ((p) >= 12 && (p) < A0))
#define IS_PIN_ANALOG(p)        ((p) >= A0 && (p) < A0 + NUM_ANALOG_INPUTS)
#define IS_PIN_PWM(p)           digitalPinHasPWM(p)
#define IS_PIN_SERVO(p)         (IS_PIN_DIGITAL(p) && (p) < MAX_SERVOS)
#define IS_PIN_I2C(p)           ((p) == SDA || (p) == SCL)
#define IS_PIN_SPI(p)           ((p) == SS || (p) == MOSI || (p) == MISO || (p) == SCK)
#define IS_PIN_INTERRUPT(p)     (digitalPinToInterrupt(p) > NOT_AN_INTERRUPT)
#define IS_PIN_SERIAL(p)        ((p) == PIN_SERIAL_RX || (p) == PIN_SERIAL_TX)
#define PIN_TO_DIGITAL(p)       (p)
#define PIN_TO_ANALOG(p)        ((p) - A0)
#define PIN_TO_PWM(p)           PIN_TO_DIGITAL(p)
#define PIN_TO_SERVO(p)         (p)
#define DEFAULT_PWM_RESOLUTION  10
 */


void loop() {
  h->handleClient();
  checkDigitalInputs();
  //delay(500);
  //Serial.println(digitalRead(D6));
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
