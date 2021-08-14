#include <DFRobot_sim808.h>
#include <SoftwareSerial.h>   

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if  I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;


#define PIN_TX    10
#define PIN_RX    11
#define SIM_PWR   9

#define SPK              A1
#define SENS_VIBRATE     A0
#define LED_GREEN        A3
#define LED_RED          A2

#define ONOFF_SPK        2
#define ONOFF_SECURITY   3
#define ONOFF_BIKE       4

#define PHONE_NUMBER "0975122195"
#define MESSAGE_parking  "vehicle has a dangerous accident"
#define MESSAGE_RUNGXE  "vehicle is compromised"


#define MESSAGE_LENGTH 160
char message[MESSAGE_LENGTH];
int messageIndex = 0;
char phone[16];
char datetime[24];

SoftwareSerial ObitSerial(PIN_TX,PIN_RX);
DFRobot_SIM808 sim808(&Serial);
long count;
#define DEBUG

void setup() {
  pinMode(A0,INPUT);
  pinMode(6,OUTPUT);
  
  digitalWrite(6,0);
  pinMode(5, INPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  digitalWrite(6,0);
  digitalWrite(7,0);
  digitalWrite(8,0);
  digitalWrite(9,0);
  Serial.begin(9600);
  ObitSerial.begin(9600);
  // put your setup code here, to run once:
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

 accelgyro.initialize();
 ObitSerial.println("run");
 pinMode(SIM_PWR,OUTPUT);
 digitalWrite(SIM_PWR,0);
 delay(1500);
 digitalWrite(SIM_PWR,1);
 delay(5000);
 ObitSerial.println("INit");
 
 while(!sim808.init()) { 
      digitalWrite(7,0);
      delay(500);
      digitalWrite(7,1);
      delay(500);
      ObitSerial.print("Sim808 init error\r\n");
  }

 delay(3000);
 ObitSerial.print("Sim808 init sim done\r\n");
 
 while( !sim808.attachGPS())
 {
  ObitSerial.print("Sim808 gps\r\n");
  digitalWrite(8,0);
  delay(500);
  digitalWrite(8,1);
  delay(500);
 }   
sim808.getGPS();
ObitSerial.print("Sim808 init gps done\r\n");
    
}

int parking;

char *SMS_Location_googlemap()
{
  char SMS[160];
  char lat[6], lon[6];
  sprintf(lat,"%s", sim808.GPSdata.lat);
  sprintf(lon,"%s", sim808.GPSdata.lon);
  sprintf(SMS,"https://www.google.com/maps/place/%s,%s",lat,lon);
  return SMS;
}

void GetCMDSMS()
{

  
  messageIndex = sim808.isSMSunread();
  
  if (messageIndex > 0) { 
    
      sim808.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);
      sim808.deleteSMS(messageIndex);          
      //***********In order not to full SIM Memory, is better to delete it**********
      //sim808.deleteSMS(messageIndex);
      if(obit_strcmp(message,MESSAGE_LENGTH,"location",8))
      {
        #if defined  DEBUG
        ObitSerial.println("handle message requesting current location");
        sim808.sendSMS(PHONE_NUMBER,SMS_Location_googlemap());  /// send toa do gps
        #endif
      }
      if(obit_strcmp(message,MESSAGE_LENGTH,"on",2))
      {
        #if defined  DEBUG
        ObitSerial.println("handle message requesting on alrm");
        digitalWrite(A1,1);
        sim808.sendSMS(PHONE_NUMBER,SMS_Location_googlemap());  /// send toa do gps
        #endif
      }
      if(obit_strcmp(message,MESSAGE_LENGTH,"off",3))
      {
        #if defined  DEBUG
        ObitSerial.println("handle message requesting off alrm");
        digitalWrite(A1,0);
        #endif
      }
  }

}


void CheckTilt()
{
  int16_t First_value=0, Last_value=0;
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  First_value = gx;
  delay(500);
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  Last_value = gx;
  if((abs(Last_value - First_value) > 400)&&(parking == 0))
  {
    parking =1;
    #if defined DEBUG
    ObitSerial.println(Last_value );
    ObitSerial.println(First_value );
    ObitSerial.println("Detected vehicle tilted");
    #endif
    digitalWrite(A3,1);
    delay(1000);
  }
  else
  {
    parking =0;
  }
}

void SendSMS_VehIsParking()
{
  if(parking == 1)
  {
    #if defined DEBUG
    ObitSerial.println("vehicle overturned, sending warning message");
    #endif
    sim808.sendSMS(PHONE_NUMBER,MESSAGE_parking); // send sms warning
    sim808.sendSMS(PHONE_NUMBER,SMS_Location_googlemap());  /// send location gps
    parking = 0;
    
  }
}
int veh_vibrating;
void Check_Vir()
{
  if(digitalRead(A0) == 0)
  {
    if(digitalRead(A0) == 1)
    {
    veh_vibrating ++;
    }
  }
  if(digitalRead(A0) == 1)
  {
    if(digitalRead(A0) == 0)
    {
    veh_vibrating ++;
    }
  }
  if(veh_vibrating>0)
    { 
    int i=0;
    digitalWrite(A1,1);
    while(i<10)
    {
      digitalWrite(A3,0);
      delay(500);
      digitalWrite(A3,1);
      delay(500);
      i++;
    }
    digitalWrite(A1,0);
    #if defined DEBUG
    ObitSerial.println("vehicle vibrating"); 
    #endif
    sim808.sendSMS(PHONE_NUMBER,MESSAGE_RUNGXE);
    veh_vibrating = 0;
    }
}

void loop() {
  // put your main code here, to run repeatedly:
   
   ObitSerial.println("loop");
   if(digitalRead(5) == 0) // unlock vehicle
   {
     digitalWrite(A2,1);
     digitalWrite(A3,0); // GREEN
     for(int i=0; i<10; i++)
     {
     CheckTilt();
     SendSMS_VehIsParking();
     }
   }
   else
   {
    digitalWrite(A2,0); //RED
    digitalWrite(A3,1);
    for(int i=0; i<10; i++)
    {
    Check_Vir();
    }
   }
   GetCMDSMS();
}


int obit_strcmp(uint8_t *str_real, uint8_t size_str_real, uint8_t *str_expected,uint8_t size_str_expected)
{
 int i=0;
 uint8_t next=0;
 while(i<size_str_real)
 {
   if(str_expected[0]==str_real[i])
   {
     for(int j=0; j<size_str_expected; j++)
     {
       if(str_expected[j]!=str_real[j+i]) 
       {
         i++;
         next=1;
         break;
       }
       else
         next = 0;
         //return -1;
     }
     if(next==0) return 1;
   }
   else 
   {
     i++;
     next=0;
   }
 }
 return 0;
}
