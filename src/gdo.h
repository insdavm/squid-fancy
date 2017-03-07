/**
 * squid-fancy
 * 
 * ...an ESP8266 (Adafruit HUZZAH Feather) Garage Door Opener
 *
 * @author Austin <github.com/insdavm>
 * @date 2017.02.08
 */

#ifndef GDO_H
#define GDO_H

#define    SWITCH        12       /* input pin for door closed switch */
#define    RELAY         14       /* output pin to control 3v mechanical relay */
#define    LED           0        /* built-in red LED */
#define    ADC           A0       /* analog-to-digital converter for reading TMP36 temp sensor */

void pulseLed(short unsigned int, short unsigned int);
void callback(char*, byte*, unsigned int);
void setupWiFi(void);
void reconnect(void);
void publishTemperature(void);
void publishDoorState(void);

#endif
