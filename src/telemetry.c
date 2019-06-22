//
//  telemetry.c
//  index
//
//  Created by Samuel Dewan on 2019-06-11.
//  Copyright © 2019 Samuel Dewan. All rights reserved.
//

#include "telemetry.h"

#include "telemetry-format.h"




static struct rn2483_desc_t *telemetry_radio_g;

static struct ms5611_desc_t *telemetry_altimeter_g;

static uint32_t rate_g;
static uint32_t last_time_g;

uint8_t telemetry_paused = 0;

static struct telemetry_api_frame packet_g;


void init_telemetry_service (struct rn2483_desc_t *radio,
                             struct ms5611_desc_t *altimeter,
                             uint32_t telemetry_rate)
{
    telemetry_radio_g = radio;
    telemetry_altimeter_g = altimeter;
    rate_g = telemetry_rate;
    
    packet_g.start_delimiter = 0x52;
    packet_g.payload_type = 0;
    packet_g.length = sizeof packet_g.payload;
    packet_g.end_delimiter = 0xcc;
}

void telemetry_service (void)
{
    if (((millis - last_time_g) >= rate_g) && !telemetry_paused) {
        last_time_g = millis;
        
        packet_g.payload.mission_time = millis;
        packet_g.payload.altimeter_temp = ms5611_get_temperature(telemetry_altimeter_g);
        packet_g.payload.altimeter_altitude = ms5611_get_altitude(telemetry_altimeter_g);
        
        rn2483_send(telemetry_radio_g, (uint8_t*)(&packet_g), sizeof packet_g);
    }
}