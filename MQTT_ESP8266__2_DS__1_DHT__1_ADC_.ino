#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"

// PIN Sensor
#define tempPin1  12
#define tempPin2  14
#define DHTPIN    13
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

/************************* WiFi Access Point *********************************/
#define WLAN_SSID       "mikrotik"
#define WLAN_PASS       "baliteam888"
/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "test.mosquitto.org"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    ""
#define AIO_KEY         ""
/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish topikTemp1 = Adafruit_MQTT_Publish(&mqtt,"/temp1");
Adafruit_MQTT_Publish topikTemp2 = Adafruit_MQTT_Publish(&mqtt,"/temp2");
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt,"humidity");
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt,"temperatur");
Adafruit_MQTT_Publish voltage = Adafruit_MQTT_Publish(&mqtt,"voltage");
//Adafruit_MQTT_Publish rawValue = Adafruit_MQTT_Publish(&mqtt,"rawValue");
Adafruit_MQTT_Publish amps = Adafruit_MQTT_Publish(&mqtt,"Amps");

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWirePin1(tempPin1);
OneWire oneWirePin2(tempPin2);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensorSuhu1(&oneWirePin1);
DallasTemperature sensorSuhu2(&oneWirePin2);

float tempPin1Value = 0; 
float tempPin2Value = 0;
int lastSend = 0;
void dht11();
void MQTT_connect();
void WiFi_connect();

const int analogIn = A0;
int mVperAmp = 66; // use 100 for 20A Module and 66 for 30A Module
int RawValue= 0;
int ACSoffset = 2500; 
double Voltage = 0;
double Amps = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();
  sensorSuhu1.begin();
  sensorSuhu2.begin();

  // init WiFi
  Serial.println("Connecting to WiFi");
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void dht11() {
  // put your main code here, to run repeatedly:
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
   Serial.println("Humidity\t: ");
   Serial.println(h);
   Serial.print(" % ");
   Serial.println("\n");
   Serial.println("Temperature\t: ");
   Serial.println(t);
   Serial.print(" C ");
   Serial.println("\n");
}

void loop() {
  // put your main code here, to run repeatedly:
  MQTT_connect();
  
  float h = dht.readHumidity();
  //Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  int16_t adc0;

  RawValue = analogRead(analogIn);
  Voltage = (RawValue / 1024.0) * 5000; // Gets you mV
  Amps = ((Voltage - ACSoffset) / mVperAmp);
 
  
  sensorSuhu1.requestTemperatures(); 
  tempPin1Value = sensorSuhu1.getTempCByIndex(0);
  Serial.print("temp1: ");
  Serial.println(tempPin1Value);

  sensorSuhu2.requestTemperatures(); 
  tempPin2Value = sensorSuhu2.getTempCByIndex(0);
  Serial.print("temp2: ");
  Serial.println(tempPin2Value);
  Serial.println("");

  Serial.print("Raw Value = " ); // shows pre-scaled value 
  Serial.print(RawValue);  
  Serial.print("\t mV = "); // shows the voltage measured 
  Serial.print(Voltage,3); // the '3' after voltage allows you to display 3 digits after decimal point
  Serial.print("\t Amps = "); // shows the voltage measured 
  Serial.println(Amps,3); // the '3' after voltage allows you to display 3 digits after decimal point
  
  
 // Now we can temperatur stuff!
  Serial.print(F("\nSending temperature val "));
  Serial.print(t);
  Serial.print("...");
  
  // Now we can humidity stuff!
   Serial.println(F("\nSending humidity val "));
   Serial.println(h);
   Serial.println("...");
  
   humidity.publish(h);
   temperature.publish(t);
   amps.publish(Amps);
   voltage.publish(Voltage);
   topikTemp1.publish(tempPin1Value);
   topikTemp2.publish(tempPin2Value);
   lastSend = millis();
   dht11();

}


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
 
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

   Serial.println("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
