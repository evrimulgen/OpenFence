#include "PinDefines.h"
#include "MCP73871.h"
#include "PA6C.h"
#include "MPU9250.h"
#include "Audio_OF.h"
#include "LoRa_OF.h"
#include "Geofence.h"
#include <Wire.h>
#include <SPI.h>
#include <RTCZero.h>

#define WAITGPSFIX
//#define GPSTestPoints

MCP73871 battery;
RTCZero rtc;
PA6C gps;
MPU9250 mpu9250;
Audio_OF audio;
LoRa_OF lora;
Geofence fence;

//Global Variables - Updatable in Web Interface
int distanceThresholds[]={0, 2, 4, 6, 8, 10}; //Distance in metres
// magbias[]={-90,400,0}; //Float in MPU9250.h library

int polyCorners = 0;
position fencePoints[255];
uint8_t fenceversion = 0;

position me, metransmitted;
datetime nowdt;
int alerts = 0;
int shocks = 0;



void setup() {
  init_pins();
  init_comms();
  delay(3000);
  audio.initAudio();
  rtc.begin(); // initialize RTC
  gps.initGPS();
  lora.initLoRa();
  init_mpu();
  //mpu9250.wakeOnmotion();

  // while(1){
  //   float compass = mpu9250.getHeading();
  //   SerialUSB.println(compass);
  //   delay(100);
  // }

  fencePoints[0].lat=145.138458;  
  fencePoints[0].lon=-37.911625; 
  fencePoints[1].lat=145.138707;
  fencePoints[1].lon=-37.911662;
  fencePoints[2].lat=145.138653;
  fencePoints[2].lon=-37.911887;
  fencePoints[3].lat=145.138399; 
  fencePoints[3].lon=-37.911850;
  polyCorners=4;

  SerialUSB.println("Setup Complete");

  #ifdef WAITGPSFIX
  SerialUSB.print("Getting GPS Fix");
  bool gpsfix=0;
  while(!gpsfix){
    bool temp=digitalRead(GPS_FIX);
    delay(1000);
    bool temp1=digitalRead(GPS_FIX);
    if(temp == 0 && temp1 == 0) gpsfix=1;
    SerialUSB.print(".");
  }
  SerialUSB.println("Fix Obtained");
  audio.enableAmp();
  audio.setvolumeBoth(20);
  delay(500);
  audio.disableAmp();
  delay(500);
  audio.enableAmp();
  delay(500);
  audio.disableAmp();
  #endif
}


int dacValue = 0;
int i=0;

void loop() {
  //SerialUSB.println("Alive...");
  

  //audioout();

  //battery.printStatus();
  //gps.getGPRMC();


  // if(mpu9250.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01) {  // On interrupt, check if data ready interrupt

  //   mpu9250.readAccelData(accelCount);  // Read the x/y/z adc values   
  //   // Now we'll calculate the accleration value into actual g's
  //   ax = (float)accelCount[0]*aRes - accelBias[0];  // get actual g value, this depends on scale being set
  //   ay = (float)accelCount[1]*aRes - accelBias[1];   
  //   az = (float)accelCount[2]*aRes - accelBias[2];  
   
  //   mpu9250.readGyroData(gyroCount);  // Read the x/y/z adc values
  //   // Calculate the gyro value into actual degrees per second
  //   gx = (float)gyroCount[0]*gRes - gyroBias[0];  // get actual gyro value, this depends on scale being set
  //   gy = (float)gyroCount[1]*gRes - gyroBias[1];  
  //   gz = (float)gyroCount[2]*gRes - gyroBias[2];   
  
  //   mpu9250.readMagData(magCount);  // Read the x/y/z adc values   
  //   // Calculate the magnetometer values in milliGauss
  //   // Include factory calibration per data sheet and user environmental corrections
  //   mx = (float)magCount[0]*mRes*magCalibration[0] - magbias[0];  // get actual magnetometer value, this depends on scale being set
  //   my = (float)magCount[1]*mRes*magCalibration[1] - magbias[1];  
  //   mz = (float)magCount[2]*mRes*magCalibration[2] - magbias[2];   
  // }



#ifndef GPSTestPoints
  gps.getGPRMC();
  me.lat=gps.getLatitude();
  me.lon=gps.getLongitude();
  nowdt.time=gps.getTime();
  nowdt.date=gps.getDate();
#endif

#ifdef GPSTestPoints
  byte seconds=rtc.getSeconds();
  if(seconds < 10){     //Inside
    me.lat=-37.911765; 
    me.lon=145.138300;
    nowdt.time=51810;
    nowdt.date=240516;
  }else if(seconds <20){
    me.lat=-37.911643; 
    me.lon= 145.137766;
    nowdt.time=51820;
    nowdt.date=240516;
  }else if(seconds <30){
    me.lat=-37.911623;
    me.lon=145.137901;
    nowdt.time=51830;
    nowdt.date=240516;
  }else if(seconds <40){
    me.lat=-37.911520;
    me.lon=145.138454;
    nowdt.time=51840;
    nowdt.date=240516;
  }else if(seconds <50){
    me.lat=-37.911997;
    me.lon=145.138565;
    nowdt.time=51850;
    nowdt.date=240516;
  }else if(seconds <60){
    me.lat=-37.912090;
    me.lon=145.138230;
    nowdt.time=51900;
    nowdt.date=240516;
  }
#endif

  if(fence.distance(me, metransmitted) > 5){
    metransmitted=me;
    SerialUSB.println("Sending Packet");
    lora.sendPosition(me, nowdt, alerts, shocks, fenceversion);
  }
   
  SerialUSB.println(polyCorners);
  SerialUSB.println(fenceversion);
  SerialUSB.println(fencePoints[0].lat);



  fenceProperty result=fence.geofence(me, fencePoints, polyCorners);
  float compass = mpu9250.getHeading();

  SerialUSB.println(compass);
  SerialUSB.println(result.sideOutside);
  SerialUSB.println(result.distance);
  SerialUSB.println(result.bearing);

  static bool outside;

  if(result.distance > 0){

    if(!outside){
      alerts++;
      outside=1;
    } 
    
    int val = compass-result.bearing;
    int volume = 10; //result.distance * 100;
    if(volume>100) volume=100;

    if(val > 180) val=val-360;
    else if(val < -180) val=val+360;
    SerialUSB.println(val);
    if(val>0){
      if(val>90){
        audio.setvolumeBoth(0);
        audio.disableAmp();
      }
      else{
        //Sound RHS
        audio.enableAmp();
        audio.setvolumeRight(volume);
        audio.setvolumeLeft(0);
      } 
    }
    else{
      if(val < -90){
        audio.setvolumeBoth(0);
        audio.disableAmp();
      }
      else{
        //Sound LHS
        audio.enableAmp();
        audio.setvolumeLeft(volume);
        audio.setvolumeRight(0);

      } 
    }


  }else{
    outside=0;
  }


  // SerialUSB.print(digitalRead(MPU_INT)); SerialUSB.print("\t Int Status: ");
  // if(digitalRead(MPU_INT)){
  //   SerialUSB.println(mpu9250.readStatus(),HEX);
  //   mpu9250.wakeOnmotion();
    
  // }
  // else SerialUSB.println("No Interrupt");

  delay(1000);
}
