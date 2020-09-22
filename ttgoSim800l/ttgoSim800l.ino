#include <WiFi.h>
#include <esp_now.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 25  //D2
#define TEMPERATURE_PRECISION 9 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress ds1, ds2,ds3,ds4, ds5, ds6;

#include "DHT.h"
#define DHTPIN 32    
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
unsigned long last_time = 0; 
boolean JustOne = true; 

#define SIM800L_IP5306_VERSION_20190610

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

#include "utilities.h"

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to the module)
#define SerialAT  Serial1

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800          // Modem is SIM800
#define TINY_GSM_RX_BUFFER      1024   // Set RX buffer to 1Kb

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60  

// Define how you're planning to connect to the internet
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define simPIN ""

// Your GPRS credentials, if any
//const char apn[] = "ltcnet";
const char apn[] = "unitel3g";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";

// MQTT details
//const char* broker = "broker.hivemq.com";//207.148.75.95
const char* broker = "207.148.75.95";//207.148.75.95

const char* topicLed = "GsmClientTest/led";
const char* topicInit = "ttgo";
const char* topicLedStatus = "GsmClientTest/ledStatus";


#include <TinyGsmClient.h>
#include <PubSubClient.h>

TinyGsmClient client(modem);
const int  port = 80;

void setupModem(){
  
#ifdef MODEM_RST
    // Keep reset high
    pinMode(MODEM_RST, OUTPUT);
    digitalWrite(MODEM_RST, HIGH);
#endif

    pinMode(MODEM_PWRKEY, OUTPUT);
    pinMode(MODEM_POWER_ON, OUTPUT);

    // Turn on the Modem power first
    digitalWrite(MODEM_POWER_ON, HIGH);

    // Pull down PWRKEY for more than 1 second according to manual requirements
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(100);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(1000);
    digitalWrite(MODEM_PWRKEY, HIGH);

    // Initialize the indicator as an output
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, LED_OFF);
}


// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
//TinyGsm modem(SerialAT);
#endif
//TinyGsmClient client(modem);
PubSubClient mqtt(client);

#define LED_PIN 13
int ledStatus = LOW;

uint32_t lastReconnectAttempt = 0;

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicLed) {
    ledStatus = !ledStatus;
    digitalWrite(LED_PIN, ledStatus);
    mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");
  }
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  // Connect to MQTT Broker
//  boolean status = mqtt.connect("GsmClientTest");

  // Or, if you want to authenticate MQTT:
  //boolean status = mqtt.connect("GsmClientName", "mqtt_user", "mqtt_pass");
  boolean status = mqtt.connect("Nanogcompost12", "mn", "mn");
  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  mqtt.publish(topicInit, "Gsm Mounoy");
  mqtt.subscribe(topicLed);
  return mqtt.connected();
}

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_send_cb(OnDataSent);

  static esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, new uint8_t[6] {0x24, 0x62, 0xAB, 0xF1, 0xB5, 0x2C}, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }

  delay(10);
  dht.begin();

  pinMode(LED_PIN, OUTPUT);

  // !!!!!!!!!!!
  // Set your reset, enable, power pins here
  // !!!!!!!!!!!
      // Start power management
    if (setupPMU() == false) {
        Serial.println("Setting power error");
    }

    // Some start operations
    setupModem();

    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
   
    SerialMon.println("Wait...");
  
// Restart takes quite some time
// To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
//delay(2000); //ok

//modem.init();

  String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem: ");
    SerialMon.println(modemInfo);  

#if TINY_GSM_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if ( strlen(simPIN) && modem.getSimStatus() != 3) {
    modem.simUnlock(simPIN);
  }
#endif

#if TINY_GSM_USE_WIFI
    // Wifi connection parameters must be set before waiting for the network
  SerialMon.print(F("Setting SSID/password..."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
#endif

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(240000L)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
     delay(2000);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      SerialMon.println(" fail");
      delay(20000);
      return;
    }
    
    SerialMon.println(" success");

  if (modem.isGprsConnected()) {
    SerialMon.println("GPRS connected");
  }
#endif

// MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("OK");
  } else {
    Serial.println("Fail");
  }
}

void loop() {

  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 1000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    
    return;
  }

  if( millis() - last_time > 10000) {
    String strtm;
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    strtm =String(humidity)+","+String(temperature);
    strtm +=","+String(dht.computeHeatIndex(temperature, humidity, false));
    
    sensors.begin();
        
    if (!sensors.getAddress(ds1, 0)){ Serial.println("Unable to find address for Device 0"); return;}
    if (!sensors.getAddress(ds2, 1)){ Serial.println("Unable to find address for Device 1");return;}
    if (!sensors.getAddress(ds3, 2)){ Serial.println("Unable to find address for Device 2"); return;}
    if (!sensors.getAddress(ds4, 3)){ Serial.println("Unable to find address for Device 3");return;}
    if (!sensors.getAddress(ds5, 4)){ Serial.println("Unable to find address for Device 4"); return;}
    if (!sensors.getAddress(ds6, 5)){ Serial.println("Unable to find address for Device 5");return;}
    
    sensors.setResolution(ds1, TEMPERATURE_PRECISION);
    sensors.setResolution(ds2, TEMPERATURE_PRECISION);
    sensors.setResolution(ds3, TEMPERATURE_PRECISION);
    sensors.setResolution(ds4, TEMPERATURE_PRECISION);
    sensors.setResolution(ds5, TEMPERATURE_PRECISION);
    sensors.setResolution(ds6, TEMPERATURE_PRECISION);
    
    sensors.requestTemperatures();
    
    float tds1 = sensors.getTempC(ds1);
    float tds2 = sensors.getTempC(ds2);
    float tds3 = sensors.getTempC(ds3);
    float tds4 = sensors.getTempC(ds4);
    float tds5 = sensors.getTempC(ds5);
    float tds6 = sensors.getTempC(ds6);
    
  if(tds1==-127)return;
  if(tds2==-127)return;
  if(tds3==-127)return;
  if(tds4==-127)return;
  if(tds5==-127)return;
  if(tds6==-127)return;

  // playload
  strtm +=","+ String(tds1)+","+ String(tds2)+","+ String(tds3)+","+ String(tds4) +","+String(tds5)+","+String(tds6); //ok
    
   mqtt.publish(topicInit, string2char("200916TTGO,"+strtm));//20200916
   SerialMon.println(string2char("200916TTGO,"+strtm));
        JustOne = false;
        last_time = millis(); 

// Send data to esp32 display
  Serial.print("Sending... ");
  esp_err_t result = esp_now_send(new uint8_t[6] {0x24, 0x62, 0xAB, 0xF1, 0xB5, 0x2C}, (uint8_t*)strtm.c_str(), strtm.length());
  if (result != ESP_OK) {
    Serial.println("Send error");
  }
         
 }

  delay(1000);  
  mqtt.loop();

 // Restart takes quite some time
 // To skip it, call init() instead of restart()
    
}
