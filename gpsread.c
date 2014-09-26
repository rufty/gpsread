/****************************************************************************************************************************************************/
/*  Purpose:    Read, parse and display data from a USB serial GPS.                                                                                 */
/*  Author:     Copyright (c) 2014, W.B.Hill <mail@wbh.org> All rights reserved.                                                                    */
/*  License:    GPLv2 - see file LICENSE or http://www.gnu.org                                                                                      */
/*  License:    BSD - see http://opensource.org/licenses/BSD-2-Clause                                                                               */
/****************************************************************************************************************************************************/
/*                                                                                                                                                  */
/*  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:  */
/*  --- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.                */
/*  --- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the       */
/*      documentation and/or other materials provided with the distribution.                                                                        */
/*                                                                                                                                                  */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   */
/*  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR    */
/*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       */
/*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF       */
/*  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         */
/*  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                                                                    */
/*                                                                                                                                                  */
/****************************************************************************************************************************************************/

// gcc -o gpsread gpsread.c -lintl -lconfuse
// New arguments need to be added where tagged 'ADDARGS'.

// Standard infrastructure
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
// Trig functions.
#include <math.h>
// For serial comms.
#include <fcntl.h>
#include <termios.h>
// For basename()
#include <libgen.h>
// Options parsing.
#include <getopt.h>
// Config file reading.
#include <confuse.h>
// For timeouts.
#include <signal.h>
// Required by strtol()
#include <limits.h>
// Compile-time defults.
#include "gpsread.h"


// Convert macros to strings.
#define STR(S) _STR(S)
#define _STR(S) #S


// Show terse info. ADDARG
void
usage ( char* appname )
  {
  printf ( "Usage: %s -t%d -b%d -d%s -u%s\n", appname, TIMEOUT, GPSBAUD, GPSTERM, STR(POSUNIT) ) ;
  }


// Show version info.
void
version ( char* appname )
  {
  printf ( "%s v%s, W.B.Hill <mail@wbh.org>, 19 Sept 2014\n", appname, STR(VERSION) ) ;
  }


// Show some help. ADDARG
void
help ( char* appname )
  {
  printf ( "Usage: %s [option] ...\n", appname ) ;
  printf ( "\t-h,--help     This help.\n" ) ;
  printf ( "\t-t,--timeout  Time to wait for GPS in seconds, default %ds\n", TIMEOUT ) ;
  printf ( "\t-b,--baudrate GPS device baudrate. Default %d\n", GPSBAUD ) ;
  printf ( "\t-d,--device   GPS tty device. Default %s\n", GPSTERM ) ;
  printf ( "\t-u,--units    Units to show position in. Default %s\n", STR(POSUNIT) ) ;
  printf ( "%s v%s, W.B.Hill <mail@wbh.org>, 19 Sept 2014\n", appname, STR(VERSION) ) ;
  }


// Die if we hit the timeout.
void
sighandler ( int sig )
  {
  if ( sig == SIGALRM )
    {
    fprintf ( stderr, "Timed out trying to read GPS.\n" ) ;
    exit ( EXIT_FAILURE ) ;
    }
  }


// Check the baud rate is OK.
int
is_valid_baud ( int br )
  {
  switch ( br )
    {
    case 50 :
    case 75 :
    case 110 :
    case 134 :
    case 150 :
    case 200 :
    case 300 :
    case 600 :
    case 1200 :
    case 1800 :
    case 2400 :
    case 4800 :
    case 9600 :
    case 19200 :
    case 38400 :
      return 0 ;
    default :
      return -1 ;
    }
  }


// Convert a string to a posuint_t
posunit_t
map_posunit ( const char* value )
  {
  if ( !strcasecmp ( value, posunit_names[TIME] ) ) return TIME ;
  else if ( !strcasecmp ( value, posunit_names[NEMA] ) ) return NEMA ;
  else if ( !strcasecmp ( value, posunit_names[OSGB] ) ) return OSGB ;
  else if ( !strcasecmp ( value, posunit_names[LLMINSEC] ) ) return LLMINSEC ;
  else if ( !strcasecmp ( value, posunit_names[LLMINDEC] ) ) return LLMINDEC ;
  else if ( !strcasecmp ( value, posunit_names[LLDECIMAL] ) ) return LLDECIMAL ;
  else return INVALID ;
  }


// Check we don't get a negative.
int
validate_uint ( cfg_t* cfg, cfg_opt_t* opt )
  {
  int value = cfg_opt_getnint ( opt, 0 ) ;
  if ( value < 0 )
    {
    cfg_error ( cfg, "Invalid timeout." ) ;
    return -1 ;
    }
  return 0 ;
  }


// Check the baud rate's a good'un.
int
validate_baud ( cfg_t* cfg, cfg_opt_t* opt )
  {
  int value ;
  value = cfg_opt_getnint ( opt, 0 ) ;
  if ( is_valid_baud ( value ) )
    {
    cfg_error ( cfg, "Invalid baudrate." ) ;
    return -1 ;
    }
  return 0 ;
  }


// Check the GPS device.
int
validate_term ( cfg_t* cfg, cfg_opt_t* opt )
  {
  /*
  if ( access ( cfg_opt_getnstr ( opt, 0 ), F_OK ) )
    {
    cfg_error(cfg, "Setting %s, problem with GPS tty value: %s", opt->name, cfg_opt_getnstr ( opt, 0 ) ) ;
    return -1 ;
    }
  */
  // Not NULL.
  if ( cfg_opt_getnstr ( opt, 0 ) == NULL )
    {
    cfg_error(cfg, "Setting %s, no GPS tty value.", opt->name ) ;
    return -1 ;
    }
  // Needs to be "/dev/tty*", at least 8 chars.
  if( strlen ( cfg_opt_getnstr ( opt, 0 ) ) < 8 )
    {
    cfg_error(cfg, "Setting %s, problem with GPS tty value: %s", opt->name, cfg_opt_getnstr ( opt, 0 ) ) ;
    return -1 ;
    }
  return 0 ;
  }


// Callback for posunit in the config files.
int
parse_posunit ( cfg_t* cfg, cfg_opt_t* opt, const char* value, void* result )
  {
  // We don't use this.
  opt = opt ;
  // Allocate for the returned value.
  *(void**) result = malloc ( sizeof ( int ) ) ;
  // Save the answer.
  int mapped = map_posunit ( value ) ;
  // OK?
  if ( mapped )
    {
    **(posunit_t**) result = mapped ;
    return 0 ;
    }
  else
    {
    cfg_error ( cfg, "Invalid position unit '%s'", value ) ;
    free ( *(void**) result ) ;
    return 1 ;
    }
  }


// Parse the config files, then the command line, setup and then finally do stuff.
int
main ( int argc, char* argv[] )
  {
  // Configuration variables. ADDARG
  static int timeout ;
  static int gpsbaud ;
  static char* gpsterm ;
  static posunit_t posunit ;
  // Config file. ADDARG
  static cfg_opt_t opts[] =
    {
    CFG_INT ( "timeout", TIMEOUT, CFGF_NONE ),
    CFG_INT ( "gpsbaud", GPSBAUD, CFGF_NONE ),
    CFG_STR ( "gpsterm", GPSTERM, CFGF_NONE ),
    CFG_PTR_CB ( "posunit", STR(POSUNIT), CFGF_NONE, parse_posunit, free ),
    CFG_END()
    } ;
  // Command line options. ADDARG
  static struct option long_options[] =
    {
      { "help",      no_argument,       0,  'h' },
      { "version",   no_argument,       0,  'v' },
      { "timeout",   required_argument, 0,  't' },
      { "baudrate",  required_argument, 0,  'b' },
      { "device",    required_argument, 0,  'd' },
      { "units",     required_argument, 0,  'u' },
      { 0, 0, 0, 0 }
    } ;
  // Load the config files.
  char* etcconf ;
  char* usrconf ;
  cfg_t* confuse ;
  // Generate the config file names from the app name.
  asprintf ( &etcconf, "/etc/%s.conf", basename ( argv[0] ) ) ;
  asprintf ( &usrconf, "%s/.%src", getenv ( "HOME" ), basename ( argv[0] ) ) ;
  // Init the config library.
  confuse = cfg_init ( opts, CFGF_NONE ) ;
  // Check the values.
  cfg_set_validate_func ( confuse, "timeout", validate_uint ) ;
  cfg_set_validate_func ( confuse, "gpsbaud", validate_baud ) ;
  cfg_set_validate_func ( confuse, "gpsterm", validate_term ) ;
  // Read the /etc/app.conf file.
  if ( cfg_parse ( confuse, etcconf ) == CFG_PARSE_ERROR )
    {
    fprintf ( stderr, "Problem with config file '%s'\n", etcconf ) ;
    exit ( EXIT_FAILURE ) ;
    }
  // Read the ~/.apprc file.
  if ( cfg_parse ( confuse, usrconf ) == CFG_PARSE_ERROR )
    {
    fprintf ( stderr, "Problem with config file '%s'\n", usrconf ) ;
    exit ( EXIT_FAILURE ) ;
    }
  // Save the values. ADDARG
  gpsterm = strdup ( cfg_getstr ( confuse, "gpsterm" ) ) ;
  gpsbaud = cfg_getint ( confuse, "gpsbaud" ) ;
  timeout = cfg_getint ( confuse, "timeout" ) ;
  posunit = *(posunit_t*) cfg_getptr ( confuse, "posunit" ) ;
  // Done - free stuff.
  cfg_free ( confuse ) ;
  free ( etcconf ) ;
  free ( usrconf ) ;
  // Now parse the command line.
  int opt = 0 ;
  int long_index = 0 ;
  // Process the command line ADDARG
  while ( ( opt = getopt_long ( argc, argv, "hvt:b:d:u:", long_options, &long_index ) ) != -1 )
    {
    switch ( opt )
      {
      case 'h' :
        help ( basename ( argv[0] ) ) ;
        exit ( EXIT_SUCCESS ) ;
      case 'v' :
        version ( basename ( argv[0] ) ) ;
        exit ( EXIT_SUCCESS ) ;
      case 't' :
        timeout = (int) strtol ( optarg, (char **)NULL, 10 ) ;
        break ;
      case 'b' :
        if ( !is_valid_baud ( (int) strtol ( optarg, (char **)NULL, 10 ) ) )
          {
          gpsbaud = (int) strtol ( optarg, (char **)NULL, 10 ) ;
          }
        else
          {
          fprintf ( stderr, "Invalid baudrate: %s\n", optarg ) ;
          exit ( EXIT_FAILURE ) ;
          }
        break ;
      case 'd' :
        // Needs to be "/dev/tty*", at least 8 chars.
        if( strlen ( optarg ) < 8 )
          {
          fprintf ( stderr, "Problem with GPS tty value: %s\n", optarg ) ;
          exit ( EXIT_FAILURE ) ;
          }
        else
          {
          free ( gpsterm ) ;
          gpsterm = strdup ( optarg  ) ;
          }
        break ;
      case 'u' :
        if ( map_posunit ( optarg ) )
          {
          posunit =  map_posunit ( optarg ) ;
          }
        else
          {
          fprintf ( stderr, "Invalid position unit: %s\n", optarg ) ;
          exit ( EXIT_FAILURE ) ;
          }
        break ;
      default :
        usage ( basename ( argv[0] ) ) ;
        exit ( EXIT_FAILURE ) ;
      }
    }
  // Shouldn't be anything left.
  if ( optind != argc )
    {
    fprintf ( stderr, "Unexpected command line argument.\n" ) ;
    usage ( basename ( argv[0] ) ) ;
    exit ( EXIT_FAILURE ) ;
    }
  // Set a callback for the alarm() signal.
  signal ( SIGALRM, sighandler ) ;
  // Timeout after specified seconds.
  alarm ( timeout ) ;
  // Start serial comms to GPS.
  int tty ;
  struct termios gpsio ;
  // Default to zero.
  memset ( &gpsio, 0, sizeof ( gpsio ) ) ;
  // Set for GPS comms.
  gpsio.c_iflag = 0 ;
  gpsio.c_oflag = 0 ;
  gpsio.c_cflag = CS8 | CREAD | CLOCAL ;
  gpsio.c_lflag = 0 ;
  gpsio.c_cc[VMIN] = 1 ;
  gpsio.c_cc[VTIME] = 5 ;
  // Open the tty device.
  tty = open ( gpsterm, O_RDWR | O_NONBLOCK ) ;
  // Did it work?
  if ( tty < 0 )
    {
    perror ( "Can't access GPS device" ) ;
    exit ( EXIT_FAILURE );
    }
  // Set baud rate.
  cfsetospeed ( &gpsio, gpsbaud ) ;
  cfsetispeed ( &gpsio, gpsbaud ) ;
  // Set properties.
  tcsetattr ( tty, TCSANOW, &gpsio ) ;
  // Attempt to fetch data.
  int i, len = 0, state = 0 ;
  unsigned char c ;
  char gpsbuffer[256] ;
  // The data.
  char utctime[10] ;
  char* raw ;
  char latc[3], lonc[4] ;
  int latd, lond ;
  double latm, lonm ;
  // Loop until break or timeout.
  while ( 1 )
    {
    // Read the GPSdongle
    i = read ( tty, &c, 1 ) ;
    // Give up if it's got nothing for us.
    if ( i <= 0 )
      {
      usleep ( 250 ) ;
      continue ;
      }
    // Don't save until the start of a sentence.
    if ( state == 0 )
      {
      if ( c == '$' ) state = 1 ;
      }
    // Save the data data.
    else if ( state == 1 )
      {
      // The end?
      if ( c == '\r' )
        {
        // Make it a valid string.
        gpsbuffer[len] = '\0' ;
        // Ready for the next time.
        len = 0 ;state = 0 ;
        // Process it.
        if ( ( gpsbuffer[0] == 'G' ) && ( gpsbuffer[1] == 'P' ) && ( gpsbuffer[2] == 'G' ) && ( gpsbuffer[3] == 'G' ) && ( gpsbuffer[4] == 'A' ) )
          {
          char **fp, *field[15], *ds ;
          ds = gpsbuffer + 6 ;
          // Save a copy, before it gets split.
          raw = strdup ( gpsbuffer ) ;
          for ( fp = field ; ( *fp = strsep( &ds, ",")) != NULL ; ) if ( ++fp >= &field[15]) break;
          /*
          0    = UTC of Position
          1    = Latitude
          2    = N or S
          3    = Longitude
          4    = E or W
          5    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
          6    = Number of satellites in use [not those in view]
          7    = Horizontal dilution of position
          8    = Antenna altitude above/below mean sea level (geoid)
          9    = Meters  (Antenna height unit)
          10   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and
                 mean sea level.  -=geoid is below WGS-84 ellipsoid)
          11   = Meters  (Units of geoidal separation)
          12   = Age in seconds since last update from diff. reference station
          13   = Diff. reference station ID#
          14   = Checksum
          */
          // Got a good reading?
          if ( (int) strtol ( field[5], (char **)NULL, 10 ) )
            {
            // Save it!
            snprintf ( utctime, 9, "%c%c:%c%c:%c%c", field[0][0], field[0][1], field[0][2], field[0][3], field[0][4], field[0][5] ) ;
            latc[0] = field[1][0] ;
            latc[1] = field[1][1] ;
            latc[2] = '\0' ;
            latd = (int) strtol ( latc, (char **)NULL, 10 ) ;
            latd *= ( field[2][0] == 'N' ) ? +1 : -1 ;
            latm = strtod ( field[1]+2, NULL ) ;
            lonc[0] = field[3][0] ;
            lonc[1] = field[3][1] ;
            lonc[2] = field[3][2] ;
            lonc[3] = '\0' ;
            lond = (int) strtol ( lonc, (char **)NULL, 10 ) ;
            lond *= ( field[4][0] == 'E' ) ? +1 : -1 ;
            lonm = strtod ( field[3]+3, NULL ) ;
            // Done it!
            break ;
            }
          else
            {
            free ( raw ) ;
            }
          }
        }
      // Save if not too much.
      else if ( len < 254 )
        {
        gpsbuffer[len++] = c ;
        }
      // This has gone on too long.
      else
        {
        // Give up.
        len = 0 ; state = 0 ;
        }
      }
    // Should never happen!
    else
      {
      // Give up.
      len = 0 ; state = 0 ;
      }
    }
  // Disable timeout.
  signal ( SIGALRM, SIG_IGN ) ;
  // Done with it now.
  close ( tty ) ;
  // Done with any config data. ADDARG
  free ( gpsterm ) ;
  // Show the data.
  double latt, lats, lont, lons ;
  char z[3] ;
  long e, n ;
  switch ( posunit )
    {
    case TIME :
      printf ( "%s\n", utctime ) ;
      break ;
    case NEMA :
      printf ( "$%s\n", raw ) ;
      break ;
    case OSGB :
      LLtoOSGB ( latd + latm / 60.0, lond + lonm / 60.0, z, &e, &n ) ;
      printf ( "[%s][%05ld][%05ld]\n", z, e, n ) ;
      break ;
    case LLMINSEC :
      lats = 60.0 * modf ( latm, &latt ) ;
      printf ( "lat: %3d%c%02d\'%06.3f\"\n", latd, ( latd < 0 ) ? 'S' : 'N', ( int ) latt, lats ) ;
      lons = 60.0 * modf ( lonm, &lont ) ;
      printf ( "lon: %3d%c%02d\'%06.3f\"\n", lond, ( lond < 0 ) ? 'W' : 'E', ( int ) lont, lons ) ;
      break ;
    case LLMINDEC :
      printf ( "lat: %3d%c%8.4f\'\n", abs ( latd ), ( latd < 0 ) ? 'S' : 'N', latm ) ;
      printf ( "lon: %3d%c%8.4f\'\n", abs ( lond ), ( lond < 0 ) ? 'W' : 'E', lonm ) ;
      break ;
    case LLDECIMAL :
      printf ( "lat: %+10.5f\n", latd + latm / 60.0 ) ;
      printf ( "lon: %+10.5f\n", lond + lonm / 60.0 ) ;
      break ;
    default :
      // What happend here?
      break ;
    }
  free ( raw ) ;
  // That's all, folks!
  return EXIT_SUCCESS ;
  }


// VIM formatting info.
// vim:ts=2:sw=2:tw=150:fo=tcnq2b:foldmethod=indent
