#ifndef GPSREAD_H
#define GPSREAD_H


// Valid position units.
typedef enum { INVALID=0, TIME, NEMA, LLMINSEC, LLMINDEC, LLDECIMAL } posunit_t ;
const char* posunit_names[] = { "INVALID", "TIME", "NEMA", "LLMINSEC", "LLMINDEC", "LLDECIMAL" } ;

// Set compile-time defaults.
#define TIMEOUT 15
#define GPSBAUD 4800
#define POSUNIT LLDECIMAL
#if __APPLE__
  #define GPSTERM "/dev/tty.usbserial"
#elif __linux
  #define GPSTERM "/dev/ttyUSB0"
#else
  #error "Unable to determine default for GPSTERM"
#endif

#endif //GPSREAD_H


// dbext:profile=testdb
// vim:ts=2:sw=2:tw=150:fo=tcnq2b:foldmethod=indent
