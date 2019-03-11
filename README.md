# Arduino mini Lora/GPS tracker
Thus  is an experimental very simple, functioning, Arduino lora tracker transmitter which fits into an Arduino pro mini 168 or 328. I am using it for an educational project with a local High School so no frills. 
It can interface with a Ublox compatible GPS via serial link 

## Connections:                                        |
                                                     |
* Arduino  3 - RFM98W DIO5              |
* Arduino  2 - RFM98W DIO0              |
* Arduino 10  - RFM98W NSS              |
* Arduino 11 - RFM98W MOSI              |
* Arduino 12 - RFM98W MISO              |
* Arduino 13 - RFM98W CLK  

* GPS RX 4
* GPS TX 5

I needed a very light arduino/lora package for a pico baloon and had a few Arduino 168 processors lying around which have 16k Flash rom and 1k ram. 
I wanted to create a very basic tracker which would compile small enough and be relatively easy to explain to students so I trimmed down lora code to the bare minimum and combined the TinyGPS library for the GPS. 

This tracker does not include command to GPS for the flight mode which still needs to be added.

Lora packet messages include Radiohead header so can be used with a Raspberry pi receiver using the Radiohead library. 
*        packet[0] = to;
*       packet[1] = from;
*        packet[2] = ID;
*        packet[3] = flags;

ascii message data is:
*        Longitude:  2.6
*        Latitude:   2.6
*        Altitude:   10

        Acknowledgements:
        Dave Akerman - Flextrack project https://github.com/daveake
        https://github.com/codegardenllc/tiny_gps_plus






