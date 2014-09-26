#include <math.h>

// Written by Chuck Gantz - chuck.gantz@globalstar.com
// Compressed for AVR by Bill Hill - bugs@wbh.org

static const char GridSquare[] = "VWXYZQRSTULMNOPFGHJKABCDE" ;
static const double a = 6377563.396 ;
static const double k0 = 0.9996012717 ;
static const double M0 = 5429228.603180 ;
static const double eccS = 0.006670539761597 ;
static const double eccP = 0.006715334668516 ;
static const double lonOR = -0.034906585039887 ;
static const double latOR = +0.855211333477221 ;

// Converts lat/long to OSGB coords.
// Lat and Lon are in decimal degrees.
void
LLtoOSGB ( const double lat, const double lon, char* OSGBz, long* OSGBe, long* OSGBn )
  {
  long posx, posy ;
  double easting, northing ;
  double latR = lat*0.017453292519943 ;
  double lonR = lon*0.017453292519943 ;
  double N = a/sqrt(1-eccS*sin(latR)*sin(latR)) ;
  double T = tan(latR)*tan(latR) ;
  double C = eccP*cos(latR)*cos(latR) ;
  double A = cos(latR)*(lonR-lonOR) ;
  double M = a*(0.998330273507751*latR-0.002505636963581*sin(2*latR)+0.000002620236941*sin(4*latR)-0.000000003381659*sin(6*latR)) ;
  easting = 400000.0+(double)(k0*N*(A+(1-T+C)*A*A*A/6+(5-18*T+T*T+72*C-58*eccP)*A*A*A*A*A/120)) ;
  northing = (double)(k0*(M-M0+N*tan(latR)*(A*A/2+(5-T+9*C+4*C*C)*A*A*A*A/24+(61-58*T+T*T+600*C-330*eccP)*A*A*A*A*A*A/720)))-100000.0 ;
  *OSGBe = (long)(easting+0.5) ; *OSGBn = (long)(northing+0.5) ;
  posx = *OSGBe/500000L ; posy = *OSGBn/500000L ;
  OSGBz[0] = GridSquare[posx+posy*5+7] ;
  posx = *OSGBe%500000L ; posx = posx/100000L ;
  posy = *OSGBn%500000L ; posy = posy/100000L ;
  OSGBz[1] = GridSquare[posx+posy*5] ;
  OSGBz[2] = '\0' ;
  *OSGBn = *OSGBn%500000L ; *OSGBn = *OSGBn%100000L ;
  *OSGBe = *OSGBe%500000L ; *OSGBe = *OSGBe%100000L ;
  }
