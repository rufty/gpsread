#ifndef GPSREAD_H
#define GPSREAD_H


// Valid position units.
typedef enum { INVALID=0, TIME, NMEA, OSGB, MHEAD, LLMINSEC, LLMINDEC, LLDECIMAL } posunit_t ;
const char* posunit_names[] = { "INVALID", "TIME", "NMEA", "OSGB", "MHEAD", "LLMINSEC", "LLMINDEC", "LLDECIMAL" } ;

// Set compile-time defaults.
#define TIMEOUT 15
#define GPSBAUD B4800
#define POSUNIT LLDECIMAL
#if __APPLE__
  #define GPSTERM "/dev/tty.usbserial"
#elif __linux
  #define GPSTERM "/dev/ttyUSB0"
#else
  #error "Unable to determine default for GPSTERM"
#endif

// Converts lat/long to OSGB coords. Lat and Lon are in decimal degrees.
void LLtoOSGB ( const double lat, const double lon, char* OSGBz, long* OSGBe, long* OSGBn ) ;


#endif //GPSREAD_H


// dbext:profile=testdb
// vim:ts=2:sw=2:tw=150:fo=tcnq2b:foldmethod=indent
