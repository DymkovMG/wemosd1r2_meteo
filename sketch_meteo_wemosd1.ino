


/*

*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <Ticker.h>
#include <Adafruit_BMP085.h>
// the sensor communicates using SPI, so include the library:
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
//#include <DHT_U.h>

//#define DEBUG 
#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print  (x)
  #define DEBUG_PRINTDEC(x) Serial.print  (x,DEC)
  #define DEBUG_PRINTLN(x) Serial.println  (x)
#else  
  #define DEBUG_PRINT(x) 
  #define DEBUG_PRINTDEC(x) 
  #define DEBUG_PRINTLN(x) 
#endif

#define INCREASE 62
#define DECREASE 60
#define STABILIZATION 124

#ifndef STASSID
#define STASSID "FALK"
#define STAPSK  "3199431994"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

#define ONE_WIRE_BUS 0 //GPIO0 D3
#define TEMPERATURE_PRECISION 12// resolution
#define NUMBER_OF_SENSORS 2 
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


#define DHTPIN 14     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

boolean flag_timer = true;
Ticker timer_read_sensor;
Adafruit_BMP085 bmp;
//float pressure;

typedef struct MyBaseStructure
{
    float value;
    char vector = STABILIZATION;
}MySensorStructure;

typedef struct DHTStructure
{
    MySensorStructure humidity;
    MySensorStructure temperature;
    MySensorStructure heatindex;
}DHTStructure;

DHTStructure dht21sensor;
//MySensorStructure dhtsensor;
MySensorStructure bmpsensor;

typedef struct DallasStructure
{
    DeviceAddress dev_addr;
    String caption;
    MySensorStructure temperature;
}DallasSensor;

DallasSensor sensors_array[NUMBER_OF_SENSORS];




void setFlag() 
{
  flag_timer = true;
}

// the setup routine runs once when you press reset:
void setup() {
  
  #ifdef DEBUG
    Serial.begin(115200);// initialize serial communication 
  #endif  
  
  DEBUG_PRINTLN("Start");

  dht.begin(); // запускаем датчик

  if (!bmp.begin()) 
  {
    DEBUG_PRINTLN("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {}
  } else{DEBUG_PRINTLN("find a valid BMP085 sensor");};

  sensors_array[0].dev_addr[0] =  0x28;
  sensors_array[0].dev_addr[1] =  0x45;
  sensors_array[0].dev_addr[2] =  0x28;
  sensors_array[0].dev_addr[3] =  0x53;
  sensors_array[0].dev_addr[4] =  0x04;
  sensors_array[0].dev_addr[5] =  0x00;
  sensors_array[0].dev_addr[6] =  0x00;
  sensors_array[0].dev_addr[7] =  0x88;
  sensors_array[0].caption="Outdoor temperature : ";

  sensors_array[1].dev_addr[0] =  0x28;
  sensors_array[1].dev_addr[1] =  0x0C;
  sensors_array[1].dev_addr[2] =  0xAB;
  sensors_array[1].dev_addr[3] =  0x0C;
  sensors_array[1].dev_addr[4] =  0x06;
  sensors_array[1].dev_addr[5] =  0x00;
  sensors_array[1].dev_addr[6] =  0x00;
  sensors_array[1].dev_addr[7] =  0xE3;
  sensors_array[1].caption="House temperature : ";

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    DEBUG_PRINT("*");
  }

  DEBUG_PRINTLN("");
  DEBUG_PRINT("Connected to ");
  DEBUG_PRINTLN(ssid);
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  DEBUG_PRINTLN("HTTP server started");

    sensors.begin();

  // ArduinoOTA.setPort(8266);
   ArduinoOTA.setHostname("WEMOS_METEO");
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  ArduinoOTA.begin();

  timer_read_sensor.attach(180, setFlag);
}



void loop() 
{
  if (flag_timer)//считываем датчики
    {
      //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));//включаем лед
      read_sensors();
      flag_timer = false;//сбрасываем флаг таймера
      //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));//выключаем лед
    };
  server.handleClient();
  ArduinoOTA.handle();
}//loop

void handleRoot() 
{
  DEBUG_PRINTLN("send http answer.");
  String message = "WEMOS METEO\n============================\n";
  message += "PRESSURE : (torr) ";
  message += String(bmpsensor.value,0);
  message += " ";
  message += bmpsensor.vector;
  message += " \n";
  message += " -------------------------";
  message += " \n";
  message += "HUMIDITY % : ";
  message += String(dht21sensor.humidity.value,0);
  message += " ";
  message += dht21sensor.humidity.vector;
  message += " \n";message += "TEMPERATURE °C: ";
  message += String(dht21sensor.temperature.value,1);
  message += " ";
  message += dht21sensor.temperature.vector;
  message += " \n";
  message += "HEAT INDEX C :  ";
  message += String(dht21sensor.heatindex.value,1);
  message += " ";
  message += dht21sensor.heatindex.vector;
  message += " \n";
  message += " -------------------------";
  message += " \n";
  int i;
          for (i = 0; i < NUMBER_OF_SENSORS; i = i + 1) 
            {
              
              message += sensors_array[i].caption;
              message += " (C) ";
              message += String(sensors_array[i].temperature.value,1);
              message += " ";
              message += sensors_array[i].temperature.vector;
              message += " \n";
            }
  server.send(200, "text/plain", message);
  
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  
}



void read_sensors()
{
  DEBUG_PRINTLN("reading sensors..");
  read_sensor_pressure();
  read_sensors_dallas();
  read_sensor_dht21();
  DEBUG_PRINTLN("OK!");
}

void read_sensor_dht21()
{
  float  prevvalue = dht21sensor.humidity.value;
  dht21sensor.humidity.value=round(dht.readHumidity()); // считываем влажность
  if (prevvalue > dht21sensor.temperature.value) //падение
    {
      dht21sensor.humidity.vector=DECREASE;
    }
    else if (prevvalue == dht21sensor.humidity.value) 
    {
      dht21sensor.humidity.vector=STABILIZATION;
    }
    else // рост
    {
     dht21sensor.humidity.vector=INCREASE;
    }
  //float t = dht.readTemperature();
  prevvalue = dht21sensor.temperature.value;
  dht21sensor.temperature.value = round(dht.readTemperature()*10)/10;
  if (prevvalue > dht21sensor.temperature.value) //падение
    {
      dht21sensor.temperature.vector=DECREASE;
    }
    else if (prevvalue == dht21sensor.temperature.value) 
    {
      dht21sensor.temperature.vector=STABILIZATION;
    }
    else // рост
    {
     dht21sensor.temperature.vector=INCREASE;
    }
  // Compute heat index in Celsius (isFahreheit = false)
  prevvalue = dht21sensor.heatindex.value;
  //float hic = dht.computeHeatIndex(dht21sensor.temperature.value, dhtsensor.value, false);
  dht21sensor.heatindex.value = round(dht.computeHeatIndex(dht21sensor.temperature.value, dht21sensor.humidity.value, false)*10)/10;
  if (prevvalue > dht21sensor.temperature.value) //падение
    {
      dht21sensor.heatindex.vector=DECREASE;
    }
    else if (prevvalue == dht21sensor.heatindex.value) 
    {
      dht21sensor.heatindex.vector=STABILIZATION;
    }
    else // рост
    {
     dht21sensor.heatindex.vector=INCREASE;
    }
  
  DEBUG_PRINT("Humidity: ");
  DEBUG_PRINTLN(dht21sensor.humidity.value);
  DEBUG_PRINT("Temperature: ");
  DEBUG_PRINTLN(dht21sensor.temperature.value);
  DEBUG_PRINT(" Heat index:: ");
  DEBUG_PRINTLN(dht21sensor.heatindex.value);


  
}

void read_sensor_pressure()
{
  
    //DEBUG_PRINT("Temperature = ");
    //DEBUG_PRINT(bmp.readTemperature());
    //DEBUG_PRINTLN(" *C");
    float pressureold =bmpsensor.value;
    DEBUG_PRINT("Pressure = ");
    bmpsensor.value = round(bmp.readPressure()/133.3);
    DEBUG_PRINT(bmpsensor.value);
    DEBUG_PRINTLN(" torr");
    
    if (pressureold > bmpsensor.value) //падение
    {
      bmpsensor.vector=DECREASE;
    }
    else if (pressureold == bmpsensor.value) 
    {
      bmpsensor.vector=STABILIZATION;
    }
    else // рост
    {
     bmpsensor.vector=INCREASE;
    }
}

void read_sensors_dallas()
{
  //DEBUG_PRINTLN("Stop timer");
  
  sensors.requestTemperatures();
  float tempold; 
  int i;
  
  for (i = 0; i < NUMBER_OF_SENSORS; i = i + 1) 
  {
    tempold = sensors_array[i].temperature.value;
    sensors_array[i].temperature.value = readTempDallas(sensors_array[i].dev_addr); 
    if (tempold > sensors_array[i].temperature.value) //падение
    {
      sensors_array[i].temperature.vector=DECREASE;
    }
    else if (tempold == sensors_array[i].temperature.value) 
    {
      sensors_array[i].temperature.vector=STABILIZATION;
    }
    else // рост
    {
     sensors_array[i].temperature.vector=INCREASE;
    }
 
  }
  
   //DEBUG_PRINTLN("Start timer"); 
}

float readTempDallas(DeviceAddress DEV)
{
   float result;
   //sensors.requestTemperaturesByAddress(DEV);
   result = round(sensors.getTempC(DEV)*10)/10;
   DEBUG_PRINTLN(result);
  return result; 
}
