#include "gps.h"
#include "gps_link.h"

GPS::GPS(HardwareSerial* SerialPort){
  gps_device = new Adafruit_GPS(SerialPort);
  last_recv = 0;
  timeout_gps_failed = 3000; // [ms]
  gps_connection = false;
}
GPS::~GPS(){
  delete(gps_device);
  gps_device = NULL;
}

int GPS::setup_gps(){
  /************************************/
  // GPS Setting
  /************************************/
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  delay(1000);
  gps_device->begin(9600);
  gps_device->sendCommand(PMTK_SET_BAUD_115200);
  gps_device->sendCommand(PMTK_SET_BAUD_115200);
  delay(200);
  gps_device->sendCommand(PMTK_SET_BAUD_115200);
  delay(200);

  gps_device->begin(115200);
  gps_device->sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  //gps_device->sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  gps_device->sendCommand(PMTK_SET_NMEA_UPDATE_5HZ); // 5 Hz update rate
  delay(200);

  // Verification : wait for the first valid NMEA sentence
  unsigned long last_setup = millis();
  while(true){
    gps_device->read();
    if(gps_device->newNMEAreceived() && gps_device->parse(gps_device->lastNMEA()))
      return true;
    if(millis() - last_setup > timeout_gps_failed)
      return false;
    yield();
  }
}

int GPS::read_all(){
  perform_read.tic();
  /************************************/
  // GPS Parser : drain the UART buffer and parse every complete sentence
  // Outputs
  //   Adafruit_GPS GPS : GPS Data
  /************************************/
  char c = 0;
  do{
    c = gps_device->read();
    if(gps_device->newNMEAreceived()){
      last_recv = millis();
      gps_device->parse(gps_device->lastNMEA()); // a broken sentence is just skipped
    }
  }while(c);

  // Check GPS Timeout : connected while NMEA arrives within the timeout
  gps_connection = gpsLinkAlive(last_recv, millis(), timeout_gps_failed);

  perform_read.toc();

  return gps_connection;
}
