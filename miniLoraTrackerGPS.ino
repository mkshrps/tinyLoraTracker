#include <avr/pgmspace.h>
#include <string.h>
#include <ctype.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
//#include <string.h>

// Lora pin definitions
#define LORA_NSS  10
//#define LORA_RESET        7                // Comment out if not connected
#define LORA_DIO0           2                
#define LORA_DIO5           3
#define LED_WARN            5
#define LED_OK              6
#define A0_MULTIPLIER      4.9

#define EXTRA_FIELD_FORMAT      ",%d,%d,%d"          // List of formats for extra fields. Make empty if no such fields.  Always use comma at start of there are any such fields.
#define EXTRA_FIELD_LIST        ,(int)((GPS.Speed * 13) / 7), GPS.Direction, GPS.Satellites

//#define EXTRA_FIELD_FORMAT      ",%d,%d,%d,%d,%d"          // List of formats for extra fields. Make empty if no such fields.  Always use comma at start of there are any such fields.
//#define EXTRA_FIELD_LIST            ,(int)((GPS.Speed * 13) / 7), GPS.Direction, GPS.Satellites, DS18B20_Temperatures[0], Channel0Average
                                                                // List of variables/expressions for extra fields. Make empty if no such fields.  Always use comma at start of there are any such fields.
#define SENTENCE_LENGTH      80                  // This is more than sufficient for the standard sentence.  Extend if needed; shorten if you are tight on memory.
// LORA settings
#define LORA_PAYLOAD_ID   "OO5"            // Do not use spaces.
#define LORA_SLOT            11
#define LORA_REPEAT_SLOT_1   -1
#define LORA_REPEAT_SLOT_2   -1

#define LORA_TIME_INDEX      2
#define LORA_TIME_MUTLIPLER  2
#define LORA_TIME_OFFSET     1
#define LORA_PACKET_TIME    500
#define LORA_FREQUENCY      434.5

#define LORA_ID              0
#define LORA_CYCLETIME       5                // Set to zero to send continuously
#define LORA_MODE            3                // setup to test with radiohead 20K8, ERR4-8,xplicit, SF7 CRC ON
#define LORA_BINARY          0
#define LORA_RH_FORMAT       1               // enable radiohead header formatting so a radiohead client can receive the messages


static const int RXPin = 4, TXPin = 5;
static const uint32_t GPSBaud = 9600;

unsigned char packet[40];                 // used to send arbitrary LoRa Messages for diagnostics and gen info
char rxBuffer[40];
struct TBinaryPacket
{
  uint8_t   PayloadIDs;
  uint16_t  Counter;
  uint16_t  BiSeconds;
  float   Latitude;
  float   Longitude;
  int32_t   Altitude;
};  //  __attribute__ ((packed));

struct TGPS
{
  int Hours, Minutes, Seconds;
  unsigned long SecondsInDay;         // Time in seconds since midnight
  float Longitude, Latitude;
  long Altitude;
  unsigned int Satellites;
  int Speed;
  int Direction;
  byte FixType;
  byte psm_status;
  float InternalTemperature;
  float BatteryVoltage;
  float ExternalTemperature;
  float Pressure;
  unsigned int BoardCurrent;
  unsigned int errorstatus;
  byte FlightMode;
  byte PowerMode;
} GPS;


int SentenceCounter=0;
byte to_node = 1;
byte from_node = 10;
byte ID = 10;
byte flags = 0;


/****************************************/


// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup()
{
  Serial.begin(9600);
  ss.begin(GPSBaud);
  
  SetupLoRa();
  
}

void loop()
{

//    Serial.println("xxx");
    // TX something very simple to test transmission      
        // Send our telemetry

/*        char *Message = "$Hello world";
        unsigned char packet[50];
        packet[0] = 10;
        packet[1] = 1;
        packet[2] = 1;
        packet[3] = 0;
        strcpy(&packet[4],Message);
        SendLoRaPacket(&packet[0], strlen(Message)+5);
*/
    //delay(10000);
      // This sketch displays information every time a new sentence is correctly encoded.
    while (ss.available() > 0)
      if (gps.encode(ss.read())){
      
        //Serial.println("decoded GPS");
      // load up the LoRa struct with data
      if (gps.location.isValid()){
        //Serial.println("location valid");
        GPS.Longitude = (float) gps.location.lng();
        GPS.Latitude = (float) gps.location.lat();
      }

      if (gps.altitude.isValid()){
        //Serial.println("altitude valid");
        GPS.Altitude = (long) gps.altitude.meters();
      }

      // if gps data is ready then send if possible otherwise wait until lora is ready
      if (gps.location.isValid() && gps.altitude.isValid()){
        //Serial.println("values valid");

        // build the message to send
        char gpsMessage[50];
        char str[20];
        strcpy(gpsMessage,"$");

        dtostrf(GPS.Longitude,2,6,str);
        strcat(gpsMessage,str);
        
        dtostrf(GPS.Latitude,2,6,str);
        strcat(gpsMessage,",");
        strcat(gpsMessage,str);
        
        ltoa(GPS.Altitude,str,10);
        strcat(gpsMessage,",");
        strcat(gpsMessage,str);

        Serial.println(gpsMessage);

        sendLoRaMessage(packet,gpsMessage);

        while(LoRaIsTransmitting()){   // wait till message sent
          delay(10);
        }
        // wait for response from server or timeout and go round again
        // put into rx mode
        //Serial.println("Rx enabled");
        startReceiving();
        byte rxcount = 30;
        int rxbytes = 0;
        while(rxcount > 0){
          delay(100);
          if(LoRaMsgReady()){
            rxbytes = receiveMessage(rxBuffer, 30); // DA's lora receive routine - lora.ino
            rxcount = 0;
            
          }
          else{
            rxcount-- ;
          }

        }
        if(rxbytes>0){
          rxBuffer[rxbytes] = (char) 0;
        Serial.println("Message Received");
        Serial.print("This Node=> ");
        Serial.print(rxBuffer[0],DEC);
        Serial.print(" Gateway Node=> ");
        Serial.print(rxBuffer[1],DEC);

        Serial.print(" Bytes Received=> ");

        Serial.print(rxbytes);
        Serial.println(&rxBuffer[4]);


        // displayInfo();
        delay(500);
        }

    // trasnsmit here
    
      }
    }
  
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));

    sendLoRaMessage(packet,"$?No GPS"); // sends lora packet in radiohead format
    delay(5000);
  }



}


void loadRH_Header(char *packet,byte to,byte from,byte ID,byte flags){
        packet[0] = to;
        packet[1] = from;
        packet[2] = ID;
        packet[3] = flags;
        
}

bool sendLoRaMessage(char *packet,char *Msg){

    loadRH_Header(packet,to_node,from_node,ID,flags);
    if(strlen(Msg) <=50){
    strcpy(&packet[4],Msg);
    SendLoRaPacket(&packet[0], strlen(Msg)+5);
    Serial.println(packet);
    
    return true;
  }
  return false;
}

void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }
  Serial.print(F("  Altitude: "));
  if (gps.altitude.isValid())
      {
        Serial.print(gps.altitude.meters());
      }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}
