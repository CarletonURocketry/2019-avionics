/**
 * @file gnss-xa1110.c
 * @desc XA1110 GNSS Receiver Driver
 * @author Tauheed ELahee
 * @date 2019-04-05
 * @last-edit 2019-07-25
 */

#include <stdlib.h>
#include <string.h>

#include "gnss-xa1110.h"


#define I(x) ((x) - '0')


struct gnss gnss_xa1110_descriptor;


/**
 *  Parse the latitude and longitude from a NMEA sentence.
 *
 *  @param lat The latitude string
 *  @param north_sorth North-South indicator string
 *  @param lon The longitude string
 *  @param easr_west East-West indicator string
 *  @param desc GNSS descriptor in which coordinates should be stored
 */
static void gnss_parse_coordinates (char *lat, char *north_south, char *lon,
                                    char *east_west, struct gnss *desc)
{
    /* Latitude */
    // Whole degrees
    desc->latitude =  I(lat[0]) * 6000000;
    desc->latitude += I(lat[1]) * 600000;
    // Whole minutes
    desc->latitude += I(lat[2]) * 100000;
    desc->latitude += I(lat[3]) * 10000;
    // Fractional minutes
    desc->latitude += I(lat[5]) * 1000;
    desc->latitude += I(lat[6]) * 100;
    desc->latitude += I(lat[7]) * 10;
    desc->latitude += I(lat[8]) * 1;
    
    // North/South
    if (north_south[0] == 'S') {
        desc->latitude *= -1;
    }
    
    /* Longitude */
    // Whole degrees
    desc->longitude =  I(lon[0]) * 60000000;
    desc->longitude += I(lon[1]) * 6000000;
    desc->longitude += I(lon[2]) * 600000;
    // Whole minutes
    desc->longitude += I(lon[3]) * 100000;
    desc->longitude += I(lon[4]) * 10000;
    // Fractional minutes
    desc->longitude += I(lon[6]) * 1000;
    desc->longitude += I(lon[7]) * 100;
    desc->longitude += I(lon[8]) * 10;
    desc->longitude += I(lon[9]) * 1;
    
    // East/West
    if (east_west[0] == 'W') {
        desc->longitude *= -1;
    }
}

/**
 *  Determine if a year is a leap year.
 *
 *  @param year The year to check
 *
 *  @return 1 if the year is a leap year, 0 otherwise
 */
static inline uint8_t is_leap_year (uint32_t year)
{
    // Year is divisible by 4 and not divisible by 100 or year is divisible by
    // 400 (the year & 15 is checking if year is divisible by 16 since 16 is
    // a factor of 400 but not 100 or 200)
    // Algorithm is explained very well here:
    //      https://stackoverflow.com/a/11595914/10914765
    return ((year & 3) == 0) && (((year % 25) != 0) || ((year & 15) == 0));
}

/**
 *  Find the number of leap years between two years (not including the first or
 *  last year).
 *
 *  @param first Lower year
 *  @param last Higher year
 *
 *  @return The number of leap years between first and last (not inclusive)
 */
static inline uint32_t leap_years_between (uint32_t first, uint32_t last)
{
    if (last == first || last == 0) {
        return 0;
    }
    last--;
    return (((last / 4) - (last / 100) + (last / 400)) -
            ((first / 4) - (first / 100) + (first / 400)));
}

static const uint16_t month_add[] = {0, 31, 31 + 28, 2*31 + 28, 2*31 + 30 + 28,
    3*31 + 30 + 28, 3*31 + 2*30 + 28, 4*31 + 2*30 + 28, 5*31 + 2*30 + 28,
    5*31 + 3*30 + 28, 6*31 + 3*30 + 28, 6*31 + 4*30 + 28};

/**
 *  Parse the time from a NMEA sentence.
 *
 *  @param date The date string
 *  @param time The time string
 *
 *  @return Unix time representation of string time
 */
static uint32_t gnss_parse_time (char *date, char *time)
{
    /* Start at January 1st 2000 */
    uint32_t unix_time = 946684800;
    
    /* Date */
    // Convert date to days since January 1st 2000
    // Days
    uint32_t days = (I(date[0]) * 10) + I(date[1]) - 1;
    // Months
    uint8_t month = (I(date[2]) * 10) + I(date[3]) - 1;
    days += month_add[month];
    // Years since 2000
    uint32_t years_since_2000 = (I(date[4]) * 10) + I(date[5]);
    days += years_since_2000 * 365;
    // Leap years since 2000 (including 2000 its self, not including the current
    // year)
    days += leap_years_between(1999, 2000 + years_since_2000);
    // Add an extra day if the current year is a leap year and it is after
    // February
    days += ((month > 1) && is_leap_year(2000 + years_since_2000));
    // Add number of days to unix time
    unix_time += days * 24 * 60 * 60;
    
    /* Time */
    // Hours
    unix_time += I(time[0]) * 60 * 60 * 10;
    unix_time += I(time[1]) * 60 * 60;
    // Minutes
    unix_time += I(time[2]) * 60 * 10;
    unix_time += I(time[3]) * 60;
    // Whole Seconds
    unix_time += I(time[4]) * 10;
    unix_time += I(time[5]);
    
    return unix_time;
}

/**
 *  Parse a floating point number an integer with a given scaling factor.
 *  The number will be parsed up to log(scale) decimal places. The returned
 *  value is scale times the value in the string.
 *
 *  @param str The string containing the floating point number
 *  @param scale The scaling factor, must be a power of 10
 *
 *  @return The parsed integer
 */
static int32_t gnss_parse_fp (char *str, uint32_t scale)
{
    int32_t out = 0;
    
    /* Find decimal place */
    char *decimal = str;
    for (; (*decimal != '.') && (*decimal != '\0'); decimal++);
    
    /* Whole part */
    int32_t weight = scale;
    char *c = decimal - 1;
    
    for (; (*c != '\0') && (*c != '-'); c--) {
        out += I(*c) * weight;
        weight *= 10;
    }
    
    int8_t negation = (*c == '-' ? -1 : 1);
    
    /* Fractional part */
    if (*decimal == '\0') {
        // No fractional part
        return out * negation;
    }
    
    weight = scale;
    c = decimal + 1;
    
    for (; (*c != '\0') && (weight != 1); c++) {
        weight /= 10;
        out += I(*c) * weight;
    }
    
    return out * negation;
}




static void gnss_parse_gga (uint8_t argc, char **argv, struct gnss *desc)
{
    // 1: UTC Time (ignored)
    
    // 2: Latitude
    // 3: North/South
    // 4: Longitude
    // 5: East/West
    //gnss_parse_coordinates(argv[2], argv[3], argv[4], argv[5], desc);
    
    // 6: Position Fix Indicator
    desc->fix_quality = strtoul(argv[6], NULL, 10);
    
    // 7: Number of satellites used
    desc->num_sats_in_use = strtoul(argv[7], NULL, 10);
    
    // 8: Horizontal Dilution of Precision
    //desc->hdop = (uint16_t)gnss_parse_fp(argv[8], 100);
    
    // 9: Alititude
    desc->altitude = gnss_parse_fp(argv[9], 1000);
    
    // 10: Altitude Units (ignored)
    
    // 11/12: Geoidal Separation (ignored)
    
    // 13: Age of Differential Correction (ignored)
    
    // All done!
}

#ifdef GNSS_STORE_IN_USE_SAT_SVS
/**
 *  Parse a comma separated list of satalite PRNs into a bit field.
 *
 *  @param num_chans The number of channels to parse
 *  @param offset Lowest possible satellite number
 *  @param str Array of strings for each channel
 *  @param field Bit field in which present channels should be stored
 */
static void gnss_parse_sat_list (uint8_t num_chans, uint8_t offset,
                                 char **str, uint32_t *field)
{
    *field = 0;
    for (uint8_t i = 0; i < num_chans; i++) {
        if (str[i][0] != '\0') {
            *field |= (1 << (strtoul(str[i], NULL, 10) - offset));
        }
    }
}
#endif

static void gnss_parse_gsa (uint8_t argc, char **argv, struct gnss *desc)
{
#ifdef GNSS_STORE_IN_USE_SAT_SVS
    // Determine if this sentence contains info on GPS or GLONASS satellites
    uint8_t gps = !strcmp(argv[0] + 1, "GPGSA");
#endif
    
    // 1: Mode 1 (ignored)
    
    // 2: Mode 2
    switch (argv[2][0]) {
        case '1':
            desc->fix_type = GNSS_FIX_NOT_AVALIABLE;
            break;
        case '2':
            desc->fix_type = GNSS_FIX_2D;
            break;
        case '3':
            desc->fix_type = GNSS_FIX_3D;
            break;
        default:
            desc->fix_type = GNSS_FIX_UNKOWN;
            break;
    }
    
    // 3: Satellite on channel 1
    // 4: Satellite on channel 2
    // 5: Satellite on channel 3
    // 6: Satellite on channel 4
    // 7: Satellite on channel 5
    // 8: Satellite on channel 6
    // 9: Satellite on channel 7
    // 10: Satellite on channel 8
    // 11: Satellite on channel 9
    // 12: Satellite on channel 10
    // 13: Satellite on channel 11
    // 14: Satellite on channel 12
#ifdef GNSS_STORE_IN_USE_SAT_SVS
    gnss_parse_sat_list(12, gps ? GPS_SV_OFFSET : GLONASS_SV_OFFSET, argv + 3,
                        gps ? &desc->gps_sats_in_use :
                        &desc->glonass_sats_in_use);
#endif
    
    // 15: Position Dilution of Precision
    desc->pdop = (uint16_t)gnss_parse_fp(argv[15], 100);
    
    // 16: Horizontal Dilution of Precision
    desc->hdop = (uint16_t)gnss_parse_fp(argv[16], 100);
    
    // 17: Vertical Dilution of Precision
    desc->vdop = (uint16_t)gnss_parse_fp(argv[17], 100);
    
    // All done!
    desc->last_meta = millis;
}

static void gnss_parse_rmc (uint8_t argc, char **argv, struct gnss *desc)
{
    // 9: Date
    // 1: UTC Time
    desc->utc_time = gnss_parse_time(argv[9], argv[1]);
    
    // 2: Status
    if (argv[2][0] == 'V') {
        //desc->status = 0;
        return;
    }
    
    // 3: Latitude
    // 4: North/South
    // 5: Longitude
    // 6: East/West
    gnss_parse_coordinates(argv[3], argv[4], argv[5], argv[6], desc);
    
    // 7: Speed over ground
    desc->speed = (uint16_t)gnss_parse_fp(argv[7], 100);
    
    // 8: Coures over ground
    desc->course = (uint16_t)gnss_parse_fp(argv[8], 100);
    
    // 10/11: Magnetic Variation (ignored)
    
    // 12: Mode (ignored)
    
    // All done!
    //desc->status = 1;
    desc->last_fix = millis;
}

#ifdef GNSS_STORE_IN_VIEW_SAT_INFO
static void gnss_parse_gsv (uint8_t argc, char **argv, struct gnss *desc)
{
    // Determine if this sentence contains info on GPS or GLONASS satellites
    uint8_t gps = !strcmp(argv[0] + 1, "GPGSV");
    
    // 1: Number of Messages (ignored)
    
    // 2: Message Number
    uint8_t message_num = strtoul(argv[2], NULL, 10) - 1;
    
    // 3: Satellites in View
    uint8_t num_in_view = strtoul(argv[3], NULL, 10);
    if (gps) {
        desc->num_gps_sats_in_view = ((num_in_view > GNSS_MAX_SATS_IN_VIEW) ?
                                      GNSS_MAX_SATS_IN_VIEW : num_in_view);
    } else {
        desc->num_glonass_sats_in_view = ((num_in_view > GNSS_MAX_SATS_IN_VIEW)
                                          ? GNSS_MAX_SATS_IN_VIEW :
                                          num_in_view);
    }
    
    // 4 through (argc - 1): In view satellite info
    uint8_t num_sats = (argc - 4) / 4;
    for (uint8_t i = 0; i < num_sats; i++) {
        uint8_t sat = (4 * message_num) + i;
        
        uint16_t id = strtoul(argv[(4 * i) + 4], NULL, 10);
        uint8_t elevation = strtoul((argv[(4 * i) + 5]), NULL, 10);
        uint16_t azimuth = strtoul((argv[(4 * i) + 6]), NULL, 10);
        uint8_t snr = strtoul((argv[(4 * i) + 7]), NULL, 10);
        
        if (gps) {
            desc->in_view_gps_satellites[sat].prn = id - GPS_SV_OFFSET;
            desc->in_view_gps_satellites[sat].elevation = elevation;
            desc->in_view_gps_satellites[sat].azimuth = azimuth;
            desc->in_view_gps_satellites[sat].snr = snr;
        } else {
            desc->in_view_glonass_satellites[sat].sat_id = (id -
                                                            GLONASS_SV_OFFSET);
            desc->in_view_glonass_satellites[sat].elevation = elevation;
            desc->in_view_glonass_satellites[sat].azimuth = azimuth;
            desc->in_view_glonass_satellites[sat].snr = snr;
        }
    }
    
    // All done!
    desc->last_gsv = millis;
}
#endif

static void gnss_parse_pgack (uint8_t argc, char **argv, struct gnss *desc)
{
    if (!strncmp(argv[1], "SW_ANT_Internal", 15)) {
        // Using internal antenna
        desc->antenna = GNSS_ANTENNA_INTERNAL;
    } else if (!strncmp(argv[1], "SW_ANT_External", 15)) {
        // Using external antenna
        desc->antenna = GNSS_ANTENNA_EXTERNAL;
    }
}





/**
 *  Desciptor for a NMEA sentence parser
 */
struct gps_parser_t {
    void (*parse)(uint8_t, char**, struct gnss*);
    const char *type;
};

/**
 *  List of avaliable NMEA sentence parsers
 */
static const struct gps_parser_t nmea_parsers[] = {
    {.parse = gnss_parse_rmc, .type = "GNRMC"},
#ifdef GNSS_STORE_IN_VIEW_SAT_INFO
    {.parse = gnss_parse_gsv, .type = "GPGSV"},
    {.parse = gnss_parse_gsv, .type = "GLGSV"},
#endif
    {.parse = gnss_parse_gga, .type = "GNGGA"},
#ifdef GNSS_STORE_IN_USE_SAT_SVS
    // Don't need to parse both GSA sentences if we are not keeping track of
    // satellite numbers
    {.parse = gnss_parse_gsa, .type = "GLGSA"},
#endif
    {.parse = gnss_parse_gsa, .type = "GPGSA"},
    {.parse = gnss_parse_pgack, .type = "PGACK"}
};


/**
 *  Verify the checksum of a NMEA sentence. The first character of the sentence
 *  (should be '$') is skipped.
 *
 *  @param str The sentence to be verified
 *
 *  @return 0 if the checksum is incorrect or the sentence is otherwise invalid,
 *          a positive integer otherwise
 */
static uint8_t verify_checksum (char *str)
{
    uint8_t checksum = str[1];
    uint8_t i;
    
    for (i = 2; (str[i] != '*') && (str[i] != '\0'); i++) {
        checksum ^= str[i];
    }
    
    if (str[i] != '*') {
        // No checksum present
        return 0;
    }
    
    char* end;
    uint8_t chk = strtoul(str + i + 1, &end, 16);
    return (*end == '\0') ? (checksum == chk) : 0;
}


static void gnss_line_callback (char *line, struct console_desc_t *console,
                                void *context)
{
    if (!verify_checksum(line)) {
        return;
    }
    
    gnss_xa1110_descriptor.last_sentence = millis;
    
    /* Count the number of comma seperated tokens in the line */
    uint8_t num_args = 1;
    for (uint16_t i = 0; i < strlen(line); i++) {
        num_args += (line[i] == ',');
    }
    
    /* Create an array of pointers to each token */
    // Array is one large to leave room for NULL pointer
    char *args[num_args + 1];
    for (uint16_t i = 0; (args[i] = strsep(&line, ",")) != NULL; i++);
    
    /* Run parser for received sentence */
    uint8_t num_parsers = sizeof(nmea_parsers)/sizeof(nmea_parsers[0]);
    for (int i = 0; i < num_parsers; i++) {
        if (!strcasecmp(args[0] + 1, nmea_parsers[i].type)) {
            nmea_parsers[i].parse(num_args, args, &gnss_xa1110_descriptor);
            break;
        }
    }
}

static void gnss_init_callback (struct console_desc_t* console, void* context) {
    // Set output/fix rate to once per second
    console_send_str(console, "$PMTK220,1000*1F\r\n");
    // Set navigation mode to "avionic"
    console_send_str(console, "$PMTK886,2*2A\r\n");
    // Disable EPE information sentence
    console_send_str(console, "$PGCMD,231,1*5C\r\n");
#ifdef GNSS_STORE_IN_VIEW_SAT_INFO
    // Enable RMC every fix, GGA and GSA every 3 fixes and GSV every 5 fixes
    console_send_str(console, "$PMTK314,0,1,0,3,3,5,0,0,0,0,0,0,0,0,0,0,0,0,0"
                              "*08\r\n");
#else
    // Enable RMC every fix and GGA and GSA every 3 fixes
    console_send_str(console, "$PMTK314,0,1,0,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0"
                              "*0D\r\n");
#endif
    
    // Poll antenna advisor
    console_send_str(console, "$PGCMD,203*40\r\n");
}

uint8_t init_gnss_xa1110 (struct console_desc_t* console)
{
    console_set_line_callback(console, gnss_line_callback, NULL);
    console_set_init_callback(console, gnss_init_callback, NULL);
    return 0;
}

