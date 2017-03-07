/**
 * squid-fancy
 * 
 * ...an ESP8266 (Adafruit HUZZAH Feather) Garage Door Opener
 *
 * @author Austin <github.com/insdavm>
 * @date 2017.02.08
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

#include "gdo.h"

const char    *ESSID                =   "your-essid";                  /* ESSID */
const char    *PASSWORD             =   "your-wifi-password";          /* WPA2-PSK Passphrase */
const char    *HOSTNAME             =   "esp8266-garage";              /* DHCP hostname */
const char    *MQTT_SERVER          =   "your-mqtt-broker-ip";         /* IP address or FQDN of MQTT broker */
const short    MQTT_PORT            =   1883;                          /* Port MQTT broker is running on */
const char    *MQTT_SUBSCRIBE_TOPIC =   "openhab/garage/relay1";       /* MQTT topic to subscribe to for commands */
const char    *MQTT_PUBLISH_TOPIC   =   "openhab/garage/temperature";  /* MQTT topic to publish the temperature to */
unsigned long tempTime              =   0;
unsigned long switchTime            =   0;
unsigned long tempInterval          =   600000;                        /* 10 min */
unsigned long switchInterval        =   10000;                         /* 10 sec */

WiFiClient espClient;
PubSubClient client(espClient);

/**
 * Pulses the ESP8266 HUZZAH's red LED (#0)
 *
 * @param ms milliseconds between each pulse
 * @param iterations number of times to turn on and off
 */
void pulseLed(short unsigned int ms, short unsigned int iterations)
{
  int i=0;
  do {
    digitalWrite(LED, HIGH);
    delay(ms);
    digitalWrite(LED, LOW);
    delay(ms);
    i++;
  } while (i < iterations);
  
  digitalWrite(LED, LOW);
}

/**
 * Callback function for when a message is received by the 
 * MQTT client on the subscribed topic.
 *
 * @param topic the topic
 * @param payload the actual message
 * @param length the length of the message in bytes
 */
void callback(char* topic, byte* payload, unsigned int length)
{
  char buffer[length + 1];
  int i = 0;
  while (i<length) {
    buffer[i] = payload[i];
    i++;
  }
  buffer[i] = '\0';

  String msgString = String(buffer);
  Serial.println("Inbound: " + String(topic) + ":" + msgString);
  
  if ( msgString == "TOGGLE" ) {
    
    digitalWrite(RELAY, HIGH); /* relay on when 3v3 is applied */
    delay(500);
    digitalWrite(RELAY, LOW); /* relay off when 0v is applied */
    pulseLed(35, 20);
    
  }
}

/**
 * Get the ESP8266's 2.4gz 802.11 b/g/n radio connected
 * to the network so we can actually receive MQTT traffic
 */
void setupWiFi()
{ 
  Serial.println();

  WiFi.hostname(HOSTNAME);

  /*
   * For some reason this defaults to AP_STA, so we need 
   * to explicitly turn the AP functionality off 
   */
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ESSID, PASSWORD);

  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    pulseLed(250, 0); /* blink semi-fast while trying to connect */
  }

  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname: ");
  Serial.print(WiFi.hostname());
  Serial.println();
  pulseLed(50, 35); /* very fast blink when connected... */
}

/**
 * Attempt reconnection to the MQTT broker and subscription
 * to the MQTT topic indefinitely
 */
void reconnect()
{
  while (!client.connected()) {
    
    Serial.print("Attempting MQTT connection...");
    String clientId = WiFi.hostname();
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(MQTT_SUBSCRIBE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
    
  }
}

/**
 * Get temperature from TMP36 sensor and publish
 * to the MQTT topic defined in MQTT_PUBLISH_TOPIC.
 *
 * Know what the analog reference (aRef) is on your board.
 * 
 *    ADC Resolution           ADC Reading
 *   ----------------   =   -----------------
 *        aRef               Measured Voltage
 * 
 * For me, it was about 1064 mV.
 */
void publishTemperature()
{
  int adcReading = analogRead(ADC);
  float mV = adcReading * 1064.0;
  mV /= 1024.0;
  mV *= 2.0;                     

  /* Celsius = ((mV - 500) / 10),   Fahrenheit = (C * 1.8) + 32 */
  float tempF = ( ((mV - 500) / 10) * 1.8 ) + 32;
  
  Serial.print("Publishing temperature: ");
  Serial.println(tempF);
  
  /* convert tempF to a char array and publish to MQTT */
  char charArray[10];
  String tempFString = String(tempF);
  tempFString.toCharArray(charArray, tempFString.length()+1);
  client.publish(MQTT_PUBLISH_TOPIC, charArray);

  pulseLed(35, 20);
}

/**
 * Publish door state
 */
void publishDoorState()
{
  char *state = (char *) "";                    /* C++ interprets char* as string constant so we're explicitly casting it */
  if ( digitalRead(SWITCH) != HIGH ) {          /* pin is pulled to ground (i.e., circuit connected i.e., door closed!) */
    state = (char *) "Closed";
    Serial.println("Switch circuit is closed.");
  } else if ( digitalRead(SWITCH) == HIGH ) {
    state = (char *) "Open";
    Serial.println("Switch circuit is open.");
  }
  client.publish("openhab/garage/switch", state);
}
  
void setup()
{
  pinMode(SWITCH, INPUT_PULLUP);                /* set door switch pin as input*/
  pinMode(RELAY, OUTPUT);                       /* set relay pin to output */
  digitalWrite(RELAY, LOW);                     /* relay is off when 0v is applied */
  pinMode(LED, OUTPUT);                         /* set LED pin to output */
  digitalWrite(LED, HIGH);                      /* turn LED off initially (yes, this special LED is off when pulled HIGH) */
  Serial.begin(115200);                         /* start serial connection for debugging messages */
  setupWiFi();                                  /* start WiFi connection */
  client.setServer(MQTT_SERVER, MQTT_PORT);     /* set the MQTT broker address and port */
  client.setCallback(callback);                 /* set the function to use when a message is received */
}


void loop()
{
  /* Send temperature if it's time to do so */
  if ( millis() - tempTime > tempInterval ) {
    tempTime = millis();
    publishTemperature();
  }

  /* Send door state if it's time to do so */
  if ( millis() - switchTime > switchInterval ) {
    switchTime = millis();
    publishDoorState();
  }
  
  /* Make sure we're connected to the MQTT broker */
  if (!client.connected()) {
    reconnect();
  }

  /* Monitor for MQTT traffic */
  client.loop();
}
