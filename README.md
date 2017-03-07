## Garage Door Opener
### w/ the Adafruit Huzzah Feather ESP8266 Breakout

----

#### Overview
This program uses an [Adafruit HUZZAH Feather](https://www.adafruit.com/product/2821) to connect to WiFi, connect to an [MQTT](https://en.wikipedia.org/wiki/MQTT) broker, and then listen for MQTT traffic.  When the string ```TOGGLE``` is published to the MQTT topic ```openhab/garage/relay1```, pin 14 is pulled high for 500ms, turning a 3v ICStation relay on and then quickly off again, simulating a momentary press of the wall switch and opening or closing the garage door.

#### Door State Reporting
I also includes a magnetic reed switch fitted at the top of the door/wall junction to signal whether or not the door is open or closed.  The state of the door (whether or not pin 12 is HIGH or LOW) is reported to the MQTT topic ```openhab/garage/switch``` every **10** seconds.

If the magnetic reed switch circuit is open (meaning the door is open), then PIN 12 will be HIGH (*because we enable the internal pullup resistor on ```setup()``` to pull the pin to 3v3*).

When the door closes, the magnet on the door physically pulls the reed switch closed, closing the circuit and connecting PIN 12 to GND, which pulls it LOW (0v).

If, during every 10-second check, PIN 12 is LOW, the program will report the door as CLOSED.  If PIN 12 is HIGH, the program will report the door as OPEN.

#### Temperature Reporting

The program will report the temperature from the [TMP36 temperature sensor](https://www.sparkfun.com/products/10988) every **10 minutes**.  The Analog-to-Digital Converter (ADC) on the ESP8266 module is a 10-bit ADC with a max voltage input of 1v.  Since the TMP36 can output up to 2v if the temperature is high enough, we need to divide the voltage in half so that we don't overload the ADC is the temperature gets above 50<sup>o</sup>C (*which will correlate to about 1v*).

To do this, I'm using two 10K ohm resistors to create a [Resistive Voltage Divider](https://en.wikipedia.org/wiki/Voltage_divider#Resistive_divider) (RVD).  Since both resistors are the same value, the voltage will get divided in half.  So instead of a 100mV - 2000mV range, we'll have a 50mV - 1000mV range which is compatible with the ESP8266's ADC.  

To calculate the temperature from the voltage, we use a formula corresponding with the data in the datasheet, which comes out to:

```
TEMP = analogRead(ADC_PIN) * 1000;       /* multiply ADC reading (1-1024) by the Analog Reference Voltage (about 1v in this case) */
TEMP /= 1023;                            /* get millivolts, but... */
TEMP *= 2;                               /* ...we divided the voltage down by 2 with our RVD, so multiply back up to simulate a 100mv - 2000mv range */
TEMP -= 500;                             /* subtract 500mV to get us to 0 deg C */
TEMP /= 10;                              /* YIELD: Degrees C  (divide by 10 because it's 1 deg C per 10 mV) */
TEMP *= 1.8;
TEMP += 32;                              /* YIELD: Degrees F  (multiply by 1.8 and add 32) */
```

#### OpenHAB Configuration

You'll need to have the MQTT binding and the MAP transformation installed, then add this to an .items file:

```
Group All
Group Garage (All)

Switch	relay        "Door Control"          <garagedoor>   (Garage)  {mqtt=">[openhab2:openhab/garage/relay1:command:ON:TOGGLE],
                                                                             >[openhab2:openhab/garage/relay1:command:OFF:TOGGLE],
                                                                             <[openhab2:openhab/garage/switch:state:MAP(garage.map)]"}
																			   
Number  garage_temp  "Garage Temp [%.1f °F]" <temperature>  (Garage)  {mqtt="<[openhab2:openhab/garage/temperature:state:default]"}
String  garage_door  "Door State [%s]"       <lock>         (Garage)  {mqtt="<[openhab2:openhab/garage/switch:state:default]"}
```

And this in your sitemap:

```
sitemap default label="Home"
{

Frame label="Garage"
{
	Switch item=relay
	Text item=garage_door
	Text item=garage_temp
}

}
```

And last but not least, your garage.map file in the transformations/ folder:

```
Open=ON
Closed=OFF
```


----

##### Author
[Austin](insdavm@gmail.com)
