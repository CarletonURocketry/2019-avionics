/**
 * @file debug-commmands.c
 * @desc Functions to be run from CLI for debuging purposes.
 * @author Samuel Dewan
 * @date 2019-09-22
 * Last Author: Samuel Dewan
 * Last Edited On: 2019-07-25
 */

#include "debug-commands.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "global.h"
#include "config.h"

#include "wdt.h"
#include "adc.h"
#include "dac.h"

#include "sercom-i2c.h"
#include "sercom-spi.h"

#include "mcp23s17-registers.h"
#include "gnss-xa1110.h"
#include "ms5611.h"

#include "telemetry-format.h"
#include "telemetry.h"

static void print_fixed_point (struct console_desc_t *console, int32_t value,
                               uint8_t decimal_places)
{
    int32_t scale = pow(10, decimal_places);
    
    char str[10];
    int32_t whole = value / scale;
    
    if ((whole == 0) && (value < 0)) {
        console_send_str(console, "-0.");
    } else {
        itoa(whole, str, 10);
        console_send_str(console, str);
        console_send_str(console, ".");
    }
    
    int32_t frac = abs(value - (whole * scale));
    itoa(frac, str, 10);
    for (int i = strlen(str); i < decimal_places; i++) {
        console_send_str(console, "0");
    }
    console_send_str(console, str);
}


#define DEBUG_VERSION_NAME  "version"
#define DEBUG_VERSION_HELP  "Get software version information."

static void debug_version (uint8_t argc, char **argv,
                           struct console_desc_t *console)
{
    console_send_str(console, VERSION_STRING);
    console_send_str(console, BUILD_STRING);
    console_send_str(console, "Configuration: ");
    console_send_str(console, CONFIG_STRING);
}



#define DEBUG_DID_NAME  "did"
#define DEBUG_DID_HELP  "Get device identification information."

static void debug_did (uint8_t argc, char **argv,
                       struct console_desc_t *console)
{
    char str[11];
    
    str[0] = '0';
    str[1] = 'x';
    
    console_send_str(console, "Serial Number: ");
    utoa(*((uint32_t*)0x0080A00C), str + 2, 16);
    console_send_str(console, str);
    utoa(*((uint32_t*)0x0080A040), str + 2, 16);
    console_send_str(console, str+2);
    utoa(*((uint32_t*)0x0080A044), str + 2, 16);
    console_send_str(console, str+2);
    utoa(*((uint32_t*)0x0080A048), str + 2, 16);
    console_send_str(console, str+2);
    
    console_send_str(console, "\n\nDevice Identification: ");
    utoa(DSU->DID.reg, str + 2, 16);
    console_send_str(console, str);
    
    console_send_str(console, "\n\tPROCESSOR: ");
    utoa(DSU->DID.bit.PROCESSOR, str + 2, 16);
    console_send_str(console, str);
    
    console_send_str(console, "\n\tFAMILY: ");
    utoa(DSU->DID.bit.FAMILY, str + 2, 16);
    console_send_str(console, str);
    
    console_send_str(console, "\n\tSERIES: ");
    utoa(DSU->DID.bit.SERIES, str + 2, 16);
    console_send_str(console, str);
    
    console_send_str(console, "\n\tDIE: ");
    utoa(DSU->DID.bit.DIE, str + 2, 16);
    console_send_str(console, str);
    
    console_send_str(console, "\n\tREVISION: ");
    utoa(DSU->DID.bit.REVISION, str + 2, 16);
    console_send_str(console, str);
    str[2] = ' ';
    str[3] = '(';
    str[4] = (char)(DSU->DID.bit.REVISION + 'A');
    str[5] = ')';
    str[6] = 0;
    console_send_str(console, str + 2);
    
    console_send_str(console, "\n\tDEVSEL: ");
    utoa(DSU->DID.bit.DEVSEL, str + 2, 16);
    console_send_str(console, str);
    
    console_send_str(console, "\nCPUID: ");
    utoa(SCB->CPUID, str + 2, 16);
    console_send_str(console, str);
    
    console_send_str(console, "\n");
}


#define DEBUG_I2C_SCAN_NAME  "i2c-scan"
#define DEBUG_I2C_SCAN_HELP  "Scan for devices on the I2C bus."

static void debug_i2c_scan (uint8_t argc, char **argv,
                           struct console_desc_t *console)
{
    // Start a scan of the I2C bus
    uint8_t i2c_t_id;
    sercom_i2c_start_scan(&i2c_g, &i2c_t_id);
    
    // Wait for scan to complete
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t_id)) wdt_pat();
    
    // Check if scan completed successfully
    switch (sercom_i2c_transaction_state(&i2c_g, i2c_t_id)) {
        case I2C_STATE_BUS_ERROR:
            console_send_str(console, "Scan failed: Bus Error\n");
            sercom_i2c_clear_transaction(&i2c_g, i2c_t_id);
            return;
        case I2C_STATE_ARBITRATION_LOST:
            console_send_str(console, "Scan failed: Arbitration Lost\n");
            sercom_i2c_clear_transaction(&i2c_g, i2c_t_id);
            return;
        case I2C_STATE_SLAVE_NACK:
            console_send_str(console, "Scan failed: Slave NACK\n");
            sercom_i2c_clear_transaction(&i2c_g, i2c_t_id);
            return;
        default:
            // Success!
            break;
    }
    
    // Print scan results
    char str[8];
    
    console_send_str(console, "Avaliable Devices:\n");
    for (int i = 0; i < 128; i++) {
        if (sercom_i2c_device_available(&i2c_g, i2c_t_id, i)) {
            console_send_str(console, "0b");
            utoa(i, str, 2);
            for (int j = strlen(str); j < 7; j++) {
                console_send_str(console, "0");
            }
            console_send_str(console, str);
            
            console_send_str(console, " (0x");
            utoa(i, str, 16);
            if (strlen(str) == 1) {
                console_send_str(console, "0");
            }
            console_send_str(console, str);
            console_send_str(console, ")\n");
        }
    }
    
    // Clear transaction
    sercom_i2c_clear_transaction(&i2c_g, i2c_t_id);
}


#define DEBUG_ALT_PROM_NAME  "alt-prom"
#define DEBUG_ALT_PROM_HELP  "Read data from altimeter PROM."

static void debug_alt_prom (uint8_t argc, char **argv,
                            struct console_desc_t *console)
{
    uint8_t i2c_t;
    uint16_t data;
    
    char str[6];
    
    // 0: Factory data and setup
    uint8_t cmd = 0b10100000;
    sercom_i2c_start_generic(&i2c_g, &i2c_t, 0b1110110, &cmd, 1, (uint8_t*)&data, 2);
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t)) {
        sercom_i2c_service(&i2c_g);
        wdt_pat();
    }
    console_send_str(console, "0: 0x");
    utoa(__builtin_bswap16(data), str, 16);
    console_send_str(console, str);
    console_send_str(console, " (Factory data and setup)\n");
    sercom_i2c_clear_transaction(&i2c_g, i2c_t);
    
    // C1: Pressure Sensitivity
    //sercom_i2c_start_reg_read(&i2c_g, &i2c_t, 0b1110110, 0b10100100, (uint8_t*)&data, 2);
    cmd = 0b10100010;
    sercom_i2c_start_generic(&i2c_g, &i2c_t, 0b1110110, &cmd, 1, (uint8_t*)&data, 2);
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t)) {
        sercom_i2c_service(&i2c_g);
        wdt_pat();
    }
    console_send_str(console, "C1: ");
    utoa(__builtin_bswap16(data), str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Pressure sensitivity)\n");
    sercom_i2c_clear_transaction(&i2c_g, i2c_t);
    
    // C2: Pressure offset
    sercom_i2c_start_reg_read(&i2c_g, &i2c_t, 0b1110110, 0b10100100, (uint8_t*)&data, 2);
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t)) {
        sercom_i2c_service(&i2c_g);
        wdt_pat();
    }
    console_send_str(console, "C2: ");
    utoa(__builtin_bswap16(data), str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Pressure offset)\n");
    sercom_i2c_clear_transaction(&i2c_g, i2c_t);
    
    // C3: Temperature coefficient of pressure sensitivity
    sercom_i2c_start_reg_read(&i2c_g, &i2c_t, 0b1110110, 0b10100110, (uint8_t*)&data, 2);
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t)) {
        sercom_i2c_service(&i2c_g);
        wdt_pat();
    }
    console_send_str(console, "C3: ");
    utoa(__builtin_bswap16(data), str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Temperature coefficient of pressure sensitivity)\n");
     sercom_i2c_clear_transaction(&i2c_g, i2c_t);
    
    // C4: Temperature coefficient of pressure offset
    sercom_i2c_start_reg_read(&i2c_g, &i2c_t, 0b1110110, 0b10101000, (uint8_t*)&data, 2);
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t)) {
        sercom_i2c_service(&i2c_g);
        wdt_pat();
    }
    console_send_str(console, "C4: ");
    utoa(__builtin_bswap16(data), str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Temperature coefficient of pressure offset)\n");
    sercom_i2c_clear_transaction(&i2c_g, i2c_t);
    
    // C5: Reference temperature
    sercom_i2c_start_reg_read(&i2c_g, &i2c_t, 0b1110110, 0b10101010, (uint8_t*)&data, 2);
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t)) {
        sercom_i2c_service(&i2c_g);
        wdt_pat();
    }
    console_send_str(console, "C5: ");
    utoa(__builtin_bswap16(data), str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Reference temperature)\n");
    sercom_i2c_clear_transaction(&i2c_g, i2c_t);
    
    // C6: Temperature coefficient of the temperature
    sercom_i2c_start_reg_read(&i2c_g, &i2c_t, 0b1110110, 0b10101100, (uint8_t*)&data, 2);
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t)) {
        sercom_i2c_service(&i2c_g);
        wdt_pat();
    }
    console_send_str(console, "C6: ");
    utoa(__builtin_bswap16(data), str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Temperature coefficient of the temperature)\n");
    sercom_i2c_clear_transaction(&i2c_g, i2c_t);
    
    // 7: Serial code and CRC
    cmd = 0b10100111;
    sercom_i2c_start_generic(&i2c_g, &i2c_t, 0b1110110, &cmd, 1, (uint8_t*)&data, 2);
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t)) {
        sercom_i2c_service(&i2c_g);
        wdt_pat();
    }
    console_send_str(console, "7: 0x");
    utoa(__builtin_bswap16(data), str, 16);
    console_send_str(console, str);
    console_send_str(console, " (Serial code and CRC)\n");
    sercom_i2c_clear_transaction(&i2c_g, i2c_t);
}


#define DEBUG_IMU_WAI_NAME  "imu-wai"
#define DEBUG_IMU_WAI_HELP  "Read IMU Who Am I register."

static void debug_imu_wai (uint8_t argc, char **argv,
                           struct console_desc_t *console)
{
    uint8_t i2c_t;
    uint8_t data;
    char str[9];
    
    // Who Am I
    sercom_i2c_start_reg_read(&i2c_g, &i2c_t, 0b1101000, 0x75, &data, 2);
    while (!sercom_i2c_transaction_done(&i2c_g, i2c_t)) {
        sercom_i2c_service(&i2c_g);
        wdt_pat();
    }
    console_send_str(console, "Who Am I: 0x");
    utoa(data, str, 16);
    console_send_str(console, str);
    console_send_str(console, " (Gyroscope and Accelerometer)\n");
    sercom_i2c_clear_transaction(&i2c_g, i2c_t);
}

#define DEBUG_IO_EXP_REGS_NAME  "io-exp-regs"
#define DEBUG_IO_EXP_REGS_HELP  "Read MCP23S17 registers.\n"\
                                "Usage: io-exp-regs [address]"

static void debug_print_byte_with_pad (struct console_desc_t *console,
                                       const char *line_start, uint8_t byte,
                                       const char *line_end)
{
    char str[9];
    
    console_send_str(console, line_start);
    
    utoa(byte, str, 2);
    for (uint8_t i = strlen(str); i < 8; i++) {
        console_send_str(console, "0");
    }
    console_send_str(console, str);
    
    console_send_str(console, line_end);
}

static void debug_io_exp_regs (uint8_t argc, char **argv,
                               struct console_desc_t *console)
{
    uint8_t address = 0;
    
    if (argc > 2) {
        console_send_str(console, DEBUG_IO_EXP_REGS_HELP);
        return;
    } else if (argc == 2) {
        char* end;
        
        uint32_t addr = strtoul(argv[1], &end, 0);
        if ((*end != '\0') || (addr > 7)) {
            console_send_str(console, DEBUG_IO_EXP_REGS_HELP);
            return;
        }
        
        address = (uint8_t)addr;
    }
    
    uint8_t command[] = {((MCP23S17_ADDR | address) << 1) | 1, 0};
    struct mcp23s17_register_map registers;
    
    
    uint8_t t_id;
    uint8_t s = sercom_spi_start(&spi_g, &t_id, 8000000UL,
                                 IO_EXPANDER_CS_PIN_GROUP,
                                 IO_EXPANDER_CS_PIN_MASK, command, 2,
                                 (uint8_t*)&registers, 10);
    
    if (s) {
        console_send_str(console, "Failed to queue SPI transaction.\n");
        return;
    }
    
    while (!sercom_spi_transaction_done(&spi_g, t_id)) wdt_pat();
    sercom_spi_clear_transaction(&spi_g, t_id);
    
    command[1] = 0x0B;
    s = sercom_spi_start(&spi_g, &t_id, 8000000UL, IO_EXPANDER_CS_PIN_GROUP,
                         IO_EXPANDER_CS_PIN_MASK, command, 2,
                         (uint8_t*)&registers.IOCON, 11);
    
    if (s) {
        console_send_str(console, "Failed to queue SPI transaction.\n");
        return;
    }
    
    debug_print_byte_with_pad(console, "   IODIRA: 0b", registers.IODIR[0].reg, "\n");
    debug_print_byte_with_pad(console, "   IODIRB: 0b", registers.IODIR[1].reg, "\n\n");
    debug_print_byte_with_pad(console, "    IPOLA: 0b", registers.IPOL[0].reg, "\n");
    debug_print_byte_with_pad(console, "    IPOLB: 0b", registers.IPOL[1].reg, "\n\n");
    wdt_pat();
    debug_print_byte_with_pad(console, " GPINTENA: 0b", registers.GPINTEN[0].reg, "\n");
    debug_print_byte_with_pad(console, " GPINTENB: 0b", registers.GPINTEN[1].reg, "\n\n");
    debug_print_byte_with_pad(console, "  DEFVALA: 0b", registers.DEFVAL[0].reg, "\n");
    debug_print_byte_with_pad(console, "  DEFVALB: 0b", registers.DEFVAL[1].reg, "\n\n");
    wdt_pat();
    debug_print_byte_with_pad(console, "  INTCONA: 0b", registers.INTCON[0].reg, "\n");
    debug_print_byte_with_pad(console, "  INTCONB: 0b", registers.INTCON[1].reg, "\n\n");
    
    while (!sercom_spi_transaction_done(&spi_g, t_id)) wdt_pat();
    sercom_spi_clear_transaction(&spi_g, t_id);
    
    debug_print_byte_with_pad(console, "   IOCON: 0b", registers.IOCON.reg, "\n\n");
    debug_print_byte_with_pad(console, "   GPPUA: 0b", registers.GPPU[0].reg, "\n");
    debug_print_byte_with_pad(console, "   GPPUB: 0b", registers.GPPU[1].reg, "\n\n");
    wdt_pat();
    debug_print_byte_with_pad(console, "   INTFA: 0b", registers.INTF[0].reg, "\n");
    debug_print_byte_with_pad(console, "   INTFB: 0b", registers.INTF[1].reg, "\n\n");
    debug_print_byte_with_pad(console, " INTCAPA: 0b", registers.INTCAP[0].reg, "\n");
    wdt_pat();
    debug_print_byte_with_pad(console, " INTCAPB: 0b", registers.INTCAP[1].reg, "\n\n");
    debug_print_byte_with_pad(console, "   GPIOA: 0b", registers.GPIO[0].reg, "\n");
    debug_print_byte_with_pad(console, "   GPIOB: 0b", registers.GPIO[1].reg, "\n\n");
    wdt_pat();
    debug_print_byte_with_pad(console, "   OLATA: 0b", registers.OLAT[0].reg, "\n");
    debug_print_byte_with_pad(console, "   OLATB: 0b", registers.OLAT[1].reg, "\n");
}

#define DEBUG_TEMP_NAME  "temp"
#define DEBUG_TEMP_HELP  "Read internal temperature sensor and the NVM "\
                         "temperature log row."

static void debug_temp (uint8_t argc, char **argv,
                        struct console_desc_t *console)
{
    // Read tempurature log values from
    uint8_t room_temp_val_int = (uint8_t)(((*((uint32_t*)
                                    NVMCTRL_FUSES_ROOM_TEMP_VAL_INT_ADDR)) &
                                        NVMCTRL_FUSES_ROOM_TEMP_VAL_INT_Msk) >>
                                          NVMCTRL_FUSES_ROOM_TEMP_VAL_INT_Pos);
    uint8_t room_temp_val_dec = (uint8_t)(((*((uint32_t*)
                                    NVMCTRL_FUSES_ROOM_TEMP_VAL_DEC_ADDR)) &
                                        NVMCTRL_FUSES_ROOM_TEMP_VAL_DEC_Msk) >>
                                          NVMCTRL_FUSES_ROOM_TEMP_VAL_DEC_Pos);
    uint8_t hot_temp_val_int = (uint8_t)(((*((uint32_t*)
                                        NVMCTRL_FUSES_HOT_TEMP_VAL_INT_ADDR)) &
                                        NVMCTRL_FUSES_HOT_TEMP_VAL_INT_Msk) >>
                                          NVMCTRL_FUSES_HOT_TEMP_VAL_INT_Pos);
    uint8_t hot_temp_val_dec = (uint8_t)(((*((uint32_t*)
                                        NVMCTRL_FUSES_HOT_TEMP_VAL_DEC_ADDR)) &
                                        NVMCTRL_FUSES_HOT_TEMP_VAL_DEC_Msk) >>
                                          NVMCTRL_FUSES_HOT_TEMP_VAL_DEC_Pos);
    int8_t room_int1v_val = (int8_t)(((*((uint32_t*)
                                        NVMCTRL_FUSES_ROOM_INT1V_VAL_ADDR)) &
                                          NVMCTRL_FUSES_ROOM_INT1V_VAL_Msk) >>
                                         NVMCTRL_FUSES_ROOM_INT1V_VAL_Pos);
    int8_t hot_int1v_val = (int8_t)(((*((uint32_t*)
                                        NVMCTRL_FUSES_HOT_INT1V_VAL_ADDR)) &
                                          NVMCTRL_FUSES_HOT_INT1V_VAL_Msk) >>
                                         NVMCTRL_FUSES_HOT_INT1V_VAL_Pos);
    
    uint16_t room_adc_val = (uint16_t)(((*((uint32_t*)
                                         NVMCTRL_FUSES_ROOM_ADC_VAL_ADDR)) &
                                        NVMCTRL_FUSES_ROOM_ADC_VAL_Msk) >>
                                       NVMCTRL_FUSES_ROOM_ADC_VAL_Pos);
    uint16_t hot_adc_val = (uint16_t)(((*((uint32_t*)
                                           NVMCTRL_FUSES_HOT_ADC_VAL_ADDR)) &
                                        NVMCTRL_FUSES_HOT_ADC_VAL_Msk) >>
                                       NVMCTRL_FUSES_HOT_ADC_VAL_Pos);
    
    char str[8];
    
    console_send_str(console, "Temperature (course): ");
    int16_t t_course = adc_get_temp_course();
    print_fixed_point(console, t_course, 2);
    console_send_str(console, " C\nTemperature (fine): ");
    int16_t t_fine = adc_get_temp_fine();
    print_fixed_point(console, t_fine, 2);
    
    wdt_pat();
    
    console_send_str(console, " C\n\nTemperature Log Row:");
    
    console_send_str(console, "\n     ROOM_TEMP_VAL: ");
    utoa(room_temp_val_int, str, 10);
    console_send_str(console, str);
    console_send_str(console, ".");
    utoa(room_temp_val_dec, str, 10);
    console_send_str(console, str);
    
    console_send_str(console, " C\n      HOT_TEMP_VAL: ");
    utoa(hot_temp_val_int, str, 10);
    console_send_str(console, str);
    console_send_str(console, ".");
    utoa(hot_temp_val_dec, str, 10);
    console_send_str(console, str);
    
    wdt_pat();
    
    console_send_str(console, " C\n    ROOM_INT1V_VAL: ");
    console_send_str(console, (room_int1v_val >= 0) ? "1." : "0.");
    utoa((room_int1v_val >= 0) ? ((unsigned int)room_int1v_val) :
         ((unsigned int)(1000 + room_int1v_val)), str, 10);
    console_send_str(console, str);
    
    console_send_str(console, " V\n     HOT_INT1V_VAL: ");
    console_send_str(console, (hot_int1v_val >= 0) ? "1." : "0.");
    utoa((hot_int1v_val >= 0) ? ((unsigned int)hot_int1v_val) :
         ((unsigned int)(1000 + hot_int1v_val)), str, 10);
    console_send_str(console, str);
    
    wdt_pat();
    
    console_send_str(console, " V\n      ROOM_ADC_VAL: 0x");
    utoa(room_adc_val, str, 16);
    console_send_str(console, str);
    
    console_send_str(console, "\n       HOT_ADC_VAL: 0x");
    utoa(hot_adc_val, str, 16);
    console_send_str(console, str);
    console_send_str(console, "\n");
}


#define DEBUG_ANALOG_NAME  "analog"
#define DEBUG_ANALOG_HELP  "Print values of analog inputs.\nUsage: analog "\
                           "[pin numbering]\nPin numbering should be one of "\
                           "internal or header."

static void debug_analog_print_channel (struct console_desc_t *console,
                                        uint8_t channel, int32_t parsed_value,
                                        uint8_t decimals, const char *name,
                                        const char *unit)
{
    char str[10];
    
    if (*name == '\0') {
        utoa(channel, str, 10);
        for (uint8_t i = strlen(str); i < 16; i++) {
            console_send_str(console, " ");
        }
        console_send_str(console, str);
    } else {
        for (uint8_t i = strlen(name); i < 16; i++) {
            console_send_str(console, " ");
        }
        console_send_str(console, name);
    }
    console_send_str(console, ": 0x");
    
    utoa(adc_get_value(channel), str, 16);
    console_send_str(console, str);
    
    if (*unit != '\0') {
        console_send_str(console, " (");
        
        print_fixed_point(console, parsed_value, decimals);
        
        console_send_str(console, " ");
        console_send_str(console, unit);
        console_send_str(console, ")\n");
    } else {
        console_send_str(console, "\n");
    }
}

static void debug_analog (uint8_t argc, char **argv,
                          struct console_desc_t *console)
{
    char str[16];
    
    console_send_str(console, "Last sweep was at ");
    utoa(adc_get_last_sweep_time(), str, 10);
    console_send_str(console, str);
    console_send_str(console, " (");
    utoa(millis - adc_get_last_sweep_time(), str, 10);
    console_send_str(console, str);
    console_send_str(console, " milliseconds ago)\n");
    
    uint32_t channel_mask = adc_get_channel_mask();
    
    if (channel_mask & ~0x1F000000) {
        // Use header pin numbers
        uint8_t analog_header_chans[] = HEADER_ANALOG_PINS;
        // Iterate over all of the analog header pins
        for (uint8_t i = 0; i < NUM_ANALOG_PINS; i++) {
            uint8_t chan = analog_header_chans[i];
            
            if (channel_mask & (1 << chan)) {
                uint16_t volts = adc_get_value_millivolts(
                                                        analog_header_chans[i]);
                
                utoa(i, str , 10);
                uint8_t p = strlen(str);
                if (chan < 10) {
                    str[p] = ' ';
                    p++;
                }
                str[p] = ' ';
                str[p + 1] = '(';
                utoa(chan, str + p + 2, 10);
                p = strlen(str);
                str[p] = ')';
                str[p + 1] = '\0';
                
                debug_analog_print_channel(console, analog_header_chans[i],
                                           (uint32_t)volts, 3, str, "V");
            }
        }
        
        wdt_pat();
        console_send_str(console, "\n");
    }
    // Iterate over enabled internal channels
    channel_mask &= 0x1F000000;
    
    while (channel_mask != 0) {
        uint32_t t = channel_mask & -channel_mask;
        int i = __builtin_ctzl(channel_mask);
        
        if (i == ADC_INPUTCTRL_MUXPOS_TEMP_Val) {
            int16_t temp = adc_get_temp_fine();
            debug_analog_print_channel(console, i, (uint32_t)temp, 2,
                                       "Temperature", "C");
        } else if (i == ADC_INPUTCTRL_MUXPOS_BANDGAP_Val) {
            uint16_t volts = adc_get_value_millivolts(i);
            debug_analog_print_channel(console, i, (uint32_t)volts, 3,
                                       "Bandgap", "V");
        } else if (i == ADC_INPUTCTRL_MUXPOS_SCALEDCOREVCC_Val) {
            int16_t volts = adc_get_core_vcc();
            debug_analog_print_channel(console, i, (uint32_t)volts, 3,
                                       "Core VCC", "V");
        } else if (i == ADC_INPUTCTRL_MUXPOS_SCALEDIOVCC_Val) {
            int16_t volts = adc_get_io_vcc();
            debug_analog_print_channel(console, i, (uint32_t)volts, 3,
                                       "IO VCC", "V");
        } else if (i == ADC_INPUTCTRL_MUXPOS_DAC_Val) {
            uint16_t volts = adc_get_value_millivolts(i);
            debug_analog_print_channel(console, i, (uint32_t)volts, 3,
                                       "DAC", "V");
        } else {
            uint16_t volts = adc_get_value_millivolts(i);
            debug_analog_print_channel(console, i, (uint32_t)volts, 3, "",
                                       "V");
        }
        
        channel_mask ^= t;
        wdt_pat();
    }
}

#define DEBUG_ALT_NAME  "alt-test"
#define DEBUG_ALT_HELP  "Print most recent values from altimeter."

static void debug_alt (uint8_t argc, char **argv,
                       struct console_desc_t *console)
{
#ifndef ENABLE_ALTIMETER
    console_send_str(console, "Altimeter is not enabled in compile time "
                        "configuration.\n");
    return;
#else
    char str[16];
    
    console_send_str(console, "PROM Values:\n");
    // C1: Pressure Sensitivity
    console_send_str(console, "C1: ");
    utoa(altimeter_g.prom_values[0], str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Pressure sensitivity)\n");
    
    // C2: Pressure offset
    console_send_str(console, "C2: ");
    utoa(altimeter_g.prom_values[1], str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Pressure offset)\n");
    
    // C3: Temperature coefficient of pressure sensitivity
    console_send_str(console, "C3: ");
    utoa(altimeter_g.prom_values[2], str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Temperature coefficient of pressure sensitivity)\n");
    
    wdt_pat();
    
    // C4: Temperature coefficient of pressure offset
    console_send_str(console, "C4: ");
    utoa(altimeter_g.prom_values[3], str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Temperature coefficient of pressure offset)\n");
    
    // C5: Reference temperature
    console_send_str(console, "C5: ");
    utoa(altimeter_g.prom_values[4], str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Reference temperature)\n");
    
    // C6: Temperature coefficient of the temperature
    console_send_str(console, "C6: ");
    utoa(altimeter_g.prom_values[5], str, 10);
    console_send_str(console, str);
    console_send_str(console, " (Temperature coefficient of the temperature)\n");
    
    wdt_pat();
    
    // Last reading time
    uint32_t last_reading_time = ms5611_get_last_reading_time(&altimeter_g);
    console_send_str(console, "\nLast reading at ");
    utoa(last_reading_time, str, 10);
    console_send_str(console, str);
    console_send_str(console, " (");
    utoa(millis - last_reading_time, str, 10);
    console_send_str(console, str);
    console_send_str(console, "  milliseconds ago)\n");
    
    // Pressure
    console_send_str(console, "Pressure: ");
    print_fixed_point(console, altimeter_g.pressure, 2);
    console_send_str(console, " mbar (");
    utoa(altimeter_g.d1, str, 10);
    console_send_str(console, str);
    console_send_str(console, ", p0 = ");
    print_fixed_point(console, (int32_t)(altimeter_g.p0 * 100), 2);
    
    wdt_pat();
    
    // Temperature
    console_send_str(console, " mbar)\nTemperature: ");
    print_fixed_point(console, altimeter_g.temperature, 2);
    console_send_str(console, " C (");
    utoa(altimeter_g.d2, str, 10);
    console_send_str(console, str);
    
    // Altitude
    int32_t altitude = (int32_t)(altimeter_g.altitude * 100);
    console_send_str(console, ")\nAltitude: ");
    print_fixed_point(console, altitude, 2);
    console_send_str(console, " m\n");
#endif
}

#define DEBUG_ALT_TARE_NOW_NAME  "alt-tare-now"
#define DEBUG_ALT_TARE_NOW_HELP  "Tare altimeter to most recently measured presure"

static void debug_alt_tare_now (uint8_t argc, char **argv,
                                struct console_desc_t *console)
{
#ifndef ENABLE_ALTIMETER
    console_send_str(console, "Altimeter is not enabled in compile time "
                     "configuration.\n");
    return;
#else
    ms5611_tare_now(&altimeter_g);
#endif
}

#define DEBUG_ALT_TARE_NEXT_NAME  "alt-tare-next"
#define DEBUG_ALT_TARE_NEXT_HELP  "Tare altimeter to next measured presure"

static void debug_alt_tare_next (uint8_t argc, char **argv,
                                 struct console_desc_t *console)
{
#ifndef ENABLE_ALTIMETER
    console_send_str(console, "Altimeter is not enabled in compile time "
                     "configuration.\n");
    return;
#else
    ms5611_tare_next(&altimeter_g);
#endif
}

#define DEBUG_GNSS_NAME  "gnss"
#define DEBUG_GNSS_HELP  "Print GNSS info"

struct xtm
{
    uint32_t year;
    int8_t mon;
    int8_t day;
    int8_t hour;
    int8_t min;
    int8_t sec;
};

#define YEAR_TO_DAYS(y) ((y)*365 + (y)/4 - (y)/100 + (y)/400)

/**
 *  Convert unix time to time struct.
 *
 *  From: https://stackoverflow.com/a/1275638/10914765
 *
 *  @param unixtime Unix time to be converted
 *  @param tm The structure to store the result in
 */
static void untime(uint32_t unixtime, struct xtm *tm)
{
    /* First take out the hour/minutes/seconds - this part is easy. */
    
    tm->sec = unixtime % 60;
    unixtime /= 60;
    
    tm->min = unixtime % 60;
    unixtime /= 60;
    
    tm->hour = unixtime % 24;
    unixtime /= 24;
    
    /* unixtime is now days since 01/01/1970 UTC
     * Rebaseline to the Common Era
     */
    
    unixtime += 719499;
    
    /* Roll forward looking for the year.  This could be done more efficiently
     * but this will do.  We have to start at 1969 because the year we calculate
     * here runs from March - so January and February 1970 will come out as 1969
     * here.
     */
    for (tm->year = 1969; unixtime > YEAR_TO_DAYS(tm->year + 1) + 30;
         tm->year++);
    
    /* OK we have our "year", so subtract off the days accounted for by full
     * years.
     */
    unixtime -= YEAR_TO_DAYS(tm->year);
    
    /* unixtime is now number of days we are into the year (remembering that
     * March 1 is the first day of the "year" still).
     */
    
    /* Roll forward looking for the month.  1 = March through to 12 =
     * February.
     */
    for (tm->mon = 1; (tm->mon < 12 && unixtime >
                       (uint32_t)(367 * (tm->mon + 1) / 12)); tm->mon++);
    
    /* Subtract off the days accounted for by full months */
    unixtime -= 367 * tm->mon / 12;
    
    /* unixtime is now number of days we are into the month */
    
    /* Adjust the month/year so that 1 = January, and years start where we
     * usually expect them to.
     */
    tm->mon += 2;
    if (tm->mon > 12) {
        tm->mon -= 12;
        tm->year++;
    }
    
    tm->day = unixtime;
}

/**
 *  Print a time structure.
 *
 *  @param console The console on which the time should be printed
 *  @param time The time to be printed
 */
static void print_time (struct console_desc_t *console, struct xtm *time)
{
        char str[6];
    
        utoa(time->year, str, 10);
        console_send_str(console, str);
        console_send_str(console, time->mon < 9 ? "-0" : "-");
        utoa(time->mon, str, 10);
        console_send_str(console, str);
        console_send_str(console, time->day < 9 ? "-0" : "-");
        utoa(time->day, str, 10);
        console_send_str(console, str);
    
        console_send_str(console, " ");
    
        utoa(time->hour, str, 10);
        console_send_str(console, str);
        console_send_str(console, time->min < 9 ? ":0" : ":");
        utoa(time->min, str, 10);
        console_send_str(console, str);
        console_send_str(console, time->sec < 9 ? ":0" : ":");
        utoa(time->sec, str, 10);
        console_send_str(console, str);
}

static void debug_gnss (uint8_t argc, char **argv,
                        struct console_desc_t *console)
{
#ifndef ENABLE_GNSS
    console_send_str(console, "GNSS is not enabled in compile time "
                     "configuration.\n");
    return;
#endif
    
    char str[8];
    
    /* Timestamps */
    console_send_str(console, "Timestamps\n\tLast sentence at ");
    utoa(gnss_xa1110_descriptor.last_sentence, str, 10);
    console_send_str(console, str);
    console_send_str(console, " (");
    utoa(millis - gnss_xa1110_descriptor.last_sentence, str, 10);
    console_send_str(console, str);
    
    console_send_str(console, " milliseconds ago)\n");
    console_send_str(console, "\tLast fix at ");
    utoa(gnss_xa1110_descriptor.last_fix, str, 10);
    console_send_str(console, str);
    console_send_str(console, " (");
    utoa(millis - gnss_xa1110_descriptor.last_fix, str, 10);
    console_send_str(console, str);
    
    console_send_str(console, " milliseconds ago)\n\tLast metadata at ");
    utoa(gnss_xa1110_descriptor.last_meta, str, 10);
    console_send_str(console, str);
    console_send_str(console, " (");
    utoa(millis - gnss_xa1110_descriptor.last_meta, str, 10);
    console_send_str(console, str);
    
#ifdef GNSS_STORE_IN_VIEW_SAT_INFO
    console_send_str(console, " milliseconds ago)\n\tLast gsv at ");
    utoa(gnss_xa1110_descriptor.last_gsv, str, 10);
    console_send_str(console, str);
    console_send_str(console, " (");
    utoa(millis - gnss_xa1110_descriptor.last_gsv, str, 10);
    console_send_str(console, str);
#endif
    
    /* Fix */
    console_send_str(console, " milliseconds ago)\nGNSS Fix\n\t");
    int32_t lat_seconds = gnss_xa1110_descriptor.latitude;
    uint8_t lat_dir = lat_seconds >= 0;
    lat_seconds *= lat_dir ? 1 : -1;
    uint32_t lat_degrees = lat_seconds / 600000;
    lat_seconds -= lat_degrees * 600000;
    uint32_t lat_minutes = lat_seconds / 10000;
    lat_seconds -= lat_minutes * 10000;
    lat_seconds *= 6;
    
    int32_t lon_seconds = gnss_xa1110_descriptor.longitude;
    uint8_t lon_dir = lon_seconds >= 0;
    lon_seconds *= lon_dir ? 1 : -1;
    uint32_t lon_degrees = lon_seconds / 600000;
    lon_seconds -= lon_degrees * 600000;
    uint32_t lon_minutes = lon_seconds / 10000;
    lon_seconds -= lon_minutes * 10000;
    lon_seconds *= 6;
    
    utoa(lat_degrees, str, 10);
    console_send_str(console, str);
    console_send_str(console, "°");
    utoa(lat_minutes, str, 10);
    console_send_str(console, str);
    console_send_str(console, "'");
    print_fixed_point(console, lat_seconds, 3);
    console_send_str(console, lat_dir ? "\"N " : "\"S ");
    
    utoa(lon_degrees, str, 10);
    console_send_str(console, str);
    console_send_str(console, "°");
    utoa(lon_minutes, str, 10);
    console_send_str(console, str);
    console_send_str(console, "'");
    print_fixed_point(console, lon_seconds, 3);
    console_send_str(console, lon_dir ? "\"E (" : "\"W (");
    
    utoa(gnss_xa1110_descriptor.latitude, str, 10);
    console_send_str(console, str);
    console_send_str(console, ", ");
    utoa(gnss_xa1110_descriptor.longitude, str, 10);
    console_send_str(console, str);
    
    /* UTC Time */
    console_send_str(console, ")\nUTC Time\n\t");
    struct xtm time;
    untime(gnss_xa1110_descriptor.utc_time, &time);
    print_time(console, &time);
    
    /* Additional GNSS Data */
    console_send_str(console, "\nAdditional GNSS Data\n\tAltitude: ");
    print_fixed_point(console, gnss_xa1110_descriptor.altitude, 3);
    // Speed over ground
    console_send_str(console, " m\n\tSpeed over ground: ");
    print_fixed_point(console, gnss_xa1110_descriptor.speed, 2);
    // Course over ground
    console_send_str(console, " knots\n\tCourse over ground: ");
    print_fixed_point(console, gnss_xa1110_descriptor.course, 2);
    
    /* Metadata */
    console_send_str(console, "°\nMetadata\n\tNumber of satellites in "
                               "use: ");
    utoa(gnss_xa1110_descriptor.num_sats_in_use, str, 10);
    console_send_str(console, str);
#ifdef GNSS_STORE_IN_USE_SAT_SVS
    if (gnss_xa1110_descriptor.gps_sats_in_use) {
        console_send_str(console, "\n\t\tGPS PRNs: ");
    }
    for (uint8_t i = 0; i < 32; i++) {
        if (gnss_xa1110_descriptor.gps_sats_in_use & (1 << i)) {
            utoa(i + GPS_SV_OFFSET, str, 10);
            console_send_str(console, str);
            console_send_str(console, " ");
        }
    }
    if (gnss_xa1110_descriptor.glonass_sats_in_use) {
        console_send_str(console, "\n\t\tGLONASS SVs: ");
    }
    for (uint8_t i = 0; i < 32; i++) {
        if (gnss_xa1110_descriptor.glonass_sats_in_use & (1 << i)) {
            utoa(i + GLONASS_SV_OFFSET, str, 10);
            console_send_str(console, str);
            console_send_str(console, " ");
        }
    }
#endif
    console_send_str(console, "\n\tPDOP: ");
    print_fixed_point(console, gnss_xa1110_descriptor.pdop, 2);
    console_send_str(console, "\n\tHDOP: ");
    print_fixed_point(console, gnss_xa1110_descriptor.hdop, 2);
    console_send_str(console, "\n\tVDOP: ");
    print_fixed_point(console, gnss_xa1110_descriptor.vdop, 2);
    switch (gnss_xa1110_descriptor.antenna) {
        case GNSS_ANTENNA_UNKOWN:
            console_send_str(console, "\n\tAntenna: Unkown\n");
            break;
        case GNSS_ANTENNA_INTERNAL:
            console_send_str(console, "\n\tAntenna: Internal\n");
            break;
        case GNSS_ANTENNA_EXTERNAL:
            console_send_str(console, "\n\tAntenna: External\n");
            break;
    }
    switch (gnss_xa1110_descriptor.fix_type) {
        case GNSS_FIX_UNKOWN:
            console_send_str(console, "\tFix: Unkown\n");
            break;
        case GNSS_FIX_NOT_AVALIABLE:
            console_send_str(console, "\tFix: Not Avaliable\n");
            break;
        case GNSS_FIX_2D:
            console_send_str(console, "\tFix: 2D\n");
            break;
        case GNSS_FIX_3D:
            console_send_str(console, "\tFix: 3D\n");
            break;
    }
    switch (gnss_xa1110_descriptor.fix_quality) {
        case GNSS_QUALITY_INVALID:
            console_send_str(console, "\tQuality: Invalid\n");
            break;
        case GNSS_QUALITY_GPS_FIX:
            console_send_str(console, "\tQuality: GPS Fix\n");
            break;
        case GNSS_QUALITY_DGPS_FIX:
            console_send_str(console, "\tQuality: Differential GPS Fix\n");
            break;
        case GNSS_QUALITY_PPS_FIX:
            console_send_str(console, "\tQuality: PPS Fix\n");
            break;
        case GNSS_QUALITY_REAL_TIME_KINEMATIC:
            console_send_str(console, "\tQuality: Real Time Kinematic\n");
            break;
        case GNSS_QUALITY_FLOAT_RTK:
            console_send_str(console, "\tQuality: Float RTK\n");
            break;
        case GNSS_QUALITY_DEAD_RECKONING:
            console_send_str(console, "\tQuality: Dead Reckoning\n");
            break;
        case GNSS_QUALITY_MANUAL_INPUT:
            console_send_str(console, "\tQuality: Manual Input\n");
            break;
        case GNSS_QUALITY_SIMULATION:
            console_send_str(console, "\tQuality: Simulation\n");
            break;
    }

#ifdef GNSS_STORE_IN_VIEW_SAT_INFO
    console_send_str(console, "\tGPS satellites in view: ");
    utoa(gnss_xa1110_descriptor.num_gps_sats_in_view, str, 10);
    console_send_str(console, str);
    console_send_str(console, "\n");
    
    for (uint8_t i = 0; i < gnss_xa1110_descriptor.num_gps_sats_in_view; i++) {
        console_send_str(console, "\t\tSat ");
        utoa(i + 1, str, 10);
        console_send_str(console, str);
        console_send_str(console, ": (PRN: ");
        utoa((gnss_xa1110_descriptor.in_view_gps_satellites[i].prn +
              GPS_SV_OFFSET), str, 10);
        console_send_str(console, str);
        console_send_str(console, ", Elevation: ");
        utoa(gnss_xa1110_descriptor.in_view_gps_satellites[i].elevation, str,
             10);
        console_send_str(console, str);
        console_send_str(console, "°, Azimuth: ");
        utoa(gnss_xa1110_descriptor.in_view_gps_satellites[i].azimuth, str, 10);
        console_send_str(console, str);
        console_send_str(console, "°, SNR: ");
        utoa(gnss_xa1110_descriptor.in_view_gps_satellites[i].snr, str, 10);
        console_send_str(console, str);
        console_send_str(console, " dB-Hz)\n");
    }
    
    console_send_str(console, "\tGLONASS satellites in view: ");
    utoa(gnss_xa1110_descriptor.num_glonass_sats_in_view, str, 10);
    console_send_str(console, str);
    console_send_str(console, "\n");
    
    for (uint8_t i = 0; i < gnss_xa1110_descriptor.num_glonass_sats_in_view;
         i++) {
        console_send_str(console, "\t\tSat ");
        utoa(i + 1, str, 10);
        console_send_str(console, str);
        console_send_str(console, ": (ID: ");
        utoa((gnss_xa1110_descriptor.in_view_glonass_satellites[i].sat_id +
              GLONASS_SV_OFFSET), str, 10);
        console_send_str(console, str);
        console_send_str(console, ", Elevation: ");
        utoa(gnss_xa1110_descriptor.in_view_glonass_satellites[i].elevation,
             str, 10);
        console_send_str(console, str);
        console_send_str(console, "°, Azimuth: ");
        utoa(gnss_xa1110_descriptor.in_view_glonass_satellites[i].azimuth, str,
             10);
        console_send_str(console, str);
        console_send_str(console, "°, SNR: ");
        utoa(gnss_xa1110_descriptor.in_view_glonass_satellites[i].snr, str, 10);
        console_send_str(console, str);
        console_send_str(console, " dB-Hz)\n");
    }
#endif
}

#define DEBUG_LORA_VERSION_NAME  "lora-version"
#define DEBUG_LORA_VERSION_HELP  "Get the version of the LoRa radio firmware"

static void debug_lora_version (uint8_t argc, char **argv,
                                struct console_desc_t *console)
{
    sercom_uart_put_string_blocking(&uart1_g, "sys get ver\r\n");
    while (!sercom_uart_has_line(&uart1_g)) {
        wdt_pat();
    }
    char str[128];
    sercom_uart_get_line(&uart1_g, str, 128);
    console_send_str(console, str);
    console_send_str(console, "\n");
}

#define DEBUG_RADIO_SEND_NAME  "radio-send"
#define DEBUG_RADIO_SEND_HELP  "Send a message via the RN2483 radio.\n"\
                               "Usage: radio-send <message>\n"

static void debug_radio_send (uint8_t argc, char **argv,
                              struct console_desc_t *console)
{
#ifndef ENABLE_LORA_RADIO
    console_send_str(console, "LoRa Radio is not enabled in compile time "
                     "configuration.\n");
    return;
#endif
    
    if (argc < 2) {
        console_send_str(console, "No message specified.\n");
        return;
    } else if (argc > 2) {
        console_send_str(console, "Too many arguments.\n");
        return;
    }
    
    enum rn2483_operation_result result = rn2483_send(&rn2483_g,
                                                      (uint8_t*)argv[1],
                                                      strlen(argv[1]) + 1);
    
    if (result == RN2483_OP_SUCCESS) {
        console_send_str(console, "Sending.\n");
    } else if (result == RN2483_OP_BUSY) {
        console_send_str(console, "Radio busy.\n");
    } else if (result == RN2483_OP_TOO_LONG) {
        console_send_str(console, "String too long to send.\n");
    }
}

#define DEBUG_RADIO_COUNT_NAME  "radio-count"
#define DEBUG_RADIO_COUNT_HELP  "Send counts at an interval with the RN2483"\
                                " radio.\nUsage: radio-count [max] [interval]"

static void debug_radio_count (uint8_t argc, char **argv,
                               struct console_desc_t *console)
{
#ifndef ENABLE_LORA_RADIO
    console_send_str(console, "LoRa Radio is not enabled in compile time "
                     "configuration.\n");
    return;
#endif
    
    uint32_t max = UINT32_MAX;
    uint32_t interval = 100;
    uint32_t last_send;
    
    char *end;
    if (argc == 2) {
        max = strtoul(argv[1], &end, 0);
        if (*end != '\0') {
            console_send_str(console, "Invalid count.\n");
            return;
        }
    } else if (argc == 3) {
        interval = strtoul(argv[2], &end, 0);
        if (*end != '\0') {
            console_send_str(console, "Invalid interval.\n");
            return;
        }
    } else if (argc > 3) {
        console_send_str(console, "Too many arguments.\n");
        return;
    }
    
    char str[11];
    for (uint32_t i = 0; i < max; i++) {
        utoa(i, str, 10);
        uint8_t len = strlen(str);
        str[len] = '\n';
        str[len + 1] = '\0';
        enum rn2483_operation_result result =  rn2483_send(&rn2483_g,
                                                           (uint8_t*)str,
                                                           strlen(str) + 1);
        
        if (result == RN2483_OP_SUCCESS) {
            console_send_str(console, str);
            console_send_str(console, "\n");
        } else if (result == RN2483_OP_BUSY) {
            console_send_str(console, "Radio busy.\n");
        } else if (result == RN2483_OP_TOO_LONG) {
            console_send_str(console, "String too long to send.\n");
        }
        
        last_send = millis;
        
        while ((millis - last_send) < interval) {
            wdt_pat();
            rn2483_service(&rn2483_g);
        }
    }
}

#define DEBUG_RADIO_RECV_NAME  "radio-receive"
#define DEBUG_RADIO_RECV_HELP  "Receive a message with the RN2483 radio.\n"\
                               "Usage: radio-receive [count] [window]"

struct radio_recv_context {
    struct console_desc_t *console;
    uint8_t receive_complete:1;
};

static void debug_radio_recv_callback (struct rn2483_desc_t *inst,
                                       void *context, uint8_t *data,
                                       uint8_t length, int8_t snr)
{
    struct radio_recv_context *c = (struct radio_recv_context*)context;
    
    if (length) {
        // Received some data
        console_send_str(c->console, "Received: \"");
        console_send_str(c->console, (char*)data);
        console_send_str(c->console, "\"\nSNR: ");
        char str[5];
        itoa(snr, str, 10);
        console_send_str(c->console, str);
        console_send_str(c->console, "\n");
    } else {
        // Did not receive any data
        console_send_str(c->console, "Did not receive data within window.\n");
    }
    c->receive_complete = 1;
}

static void debug_radio_recv (uint8_t argc, char **argv,
                              struct console_desc_t *console)
{
#ifndef ENABLE_LORA_RADIO
    console_send_str(console, "LoRa Radio is not enabled in compile time "
                     "configuration.\n");
    return;
#endif
    
    uint32_t count = 1;
    uint32_t window = 500;
    
    char *end;
    if (argc == 2) {
        count = strtoul(argv[1], &end, 0);
        if (*end != '\0') {
            console_send_str(console, "Invalid count.\n");
            return;
        }
    } else if (argc == 3) {
        window = strtoul(argv[2], &end, 0);
        if (*end != '\0') {
            console_send_str(console, "Invalid window.\n");
            return;
        }
    } else if (argc > 3) {
        console_send_str(console, "Too many arguments.\n");
        return;
    }
    
    struct radio_recv_context context;
    context.console = console;
    
    for (uint32_t i = 0; (i < count) || (count == 0); i++) {
        enum rn2483_operation_result result =  rn2483_receive(&rn2483_g, window,
                                                    debug_radio_recv_callback,
                                                              &context);
        
        if (result == RN2483_OP_BUSY) {
            console_send_str(console, "Radio busy.\n");
        } else {
            context.receive_complete = 0;
            while (!context.receive_complete) {
                wdt_pat();
                rn2483_service(&rn2483_g);
            }
        }
    }
}

#define DEBUG_TELEM_TEST_NAME  "telem-test"
#define DEBUG_TELEM_TEST_HELP  "Send a test telemetry packet"

static void debug_telem_test (uint8_t argc, char **argv,
                              struct console_desc_t *console)
{
    struct telemetry_api_frame packet;
    packet.start_delimiter = 0x52;
    packet.payload_type = 0;
    packet.length = sizeof packet.payload;
    packet.end_delimiter = 0xcc;
    
    packet.payload.mission_time = millis;
    
    packet.payload.altimeter_altitude = 1078.15f;
    packet.payload.altimeter_temp = 2367;
    
    packet.payload.gps_utc_time = 1560301659;
    packet.payload.gps_latitude = 453856;
    packet.payload.gps_longitude = -756973;
    packet.payload.gps_speed = 700;
    packet.payload.gps_course = 1200;
    
    enum rn2483_operation_result result =  rn2483_send(&rn2483_g,
                                                       (uint8_t*)&packet,
                                                       sizeof packet);
    
    if (result == RN2483_OP_SUCCESS) {
        console_send_str(console, "Sending.\n");
    } else if (result == RN2483_OP_BUSY) {
        console_send_str(console, "Radio busy.\n");
    } else if (result == RN2483_OP_TOO_LONG) {
        console_send_str(console, "String too long to send.\n");
    }
}

#define DEBUG_TELEM_PAUSE_NAME  "telem-pause"
#define DEBUG_TELEM_PAUSE_HELP  "Pause automatic transmition of telemetry"

static void debug_telem_pause (uint8_t argc, char **argv,
                               struct console_desc_t *console)
{
    telemetry_paused = 1;
}

#define DEBUG_TELEM_RESUME_NAME  "telem-resume"
#define DEBUG_TELEM_RESUME_HELP  "Resume automatic transmition of telemetry"

static void debug_telem_resume (uint8_t argc, char **argv,
                                struct console_desc_t *console)
{
    telemetry_paused = 0;
}

#define DEBUG_ADC_INIT_NAME  "adc-init"
#define DEBUG_ADC_INIT_HELP  "Initialize ADC"

#ifndef ENABLE_ADC

struct pin_t {
    uint8_t num:5;
    uint8_t port:1;
};

static const struct pin_t adc_pins[] = {
    {.port = 0, .num = 2},  // AIN[0]
    {.port = 0, .num = 3},  // AIN[1]
    {.port = 1, .num = 8},  // AIN[2]
    {.port = 1, .num = 9},  // AIN[3]
    {.port = 0, .num = 4},  // AIN[4]
    {.port = 0, .num = 5},  // AIN[5]
    {.port = 0, .num = 6},  // AIN[6]
    {.port = 0, .num = 7},  // AIN[7]
    {.port = 1, .num = 0},  // AIN[8]
    {.port = 1, .num = 1},  // AIN[9]
    {.port = 1, .num = 2},  // AIN[10]
    {.port = 1, .num = 3},  // AIN[11]
    {.port = 1, .num = 4},  // AIN[12]
    {.port = 1, .num = 5},  // AIN[13]
    {.port = 1, .num = 6},  // AIN[14]
    {.port = 1, .num = 7},  // AIN[15]
    {.port = 0, .num = 8},  // AIN[16]
    {.port = 0, .num = 9},  // AIN[17]
    {.port = 0, .num = 10}, // AIN[18]
    {.port = 0, .num = 11}  // AIN[19]
};

static void adc_set_pmux (uint8_t channel)
{
    struct pin_t pin = adc_pins[channel];
    
    if ((pin.num % 2) == 0) {
        PORT->Group[pin.port].PMUX[pin.num / 2].bit.PMUXE = 0x1;
    } else {
        PORT->Group[pin.port].PMUX[pin.num / 2].bit.PMUXO = 0x1;
    }
    PORT->Group[pin.port].PINCFG[pin.num].bit.PMUXEN = 0b1;
}
#endif

static void debug_adc_init (uint8_t argc, char **argv,
                            struct console_desc_t *console)
{
#ifdef ENABLE_ADC
    console_send_str(console, "ADC driver is enabled. Use \"analog\" command "
                     "instead\n");
#else
    adc_set_pmux(18);
    adc_set_pmux(17);
    adc_set_pmux(16);
    adc_set_pmux(15);
    
    /* Enable the APBC clock for the ADC */
    PM->APBCMASK.reg |= PM_APBCMASK_ADC;
    
    /* Select the core clock for the ADC */
    GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK3 | GCLK_CLKCTRL_ID_ADC);
    // Wait for synchronization
    while (GCLK->STATUS.bit.SYNCBUSY);
    
    /* Reset ADC */
    ADC->CTRLA.bit.SWRST = 1;
    // Wait for reset to complete
    while (ADC->CTRLA.bit.SWRST | ADC->STATUS.bit.SYNCBUSY);
    
    ADC->SAMPCTRL.bit.SAMPLEN = 17;
    
    /* Use internal 1.0 V reference with reference buffer offset compensation */
    ADC->REFCTRL.reg = /*ADC_REFCTRL_REFCOMP |*/ ADC_REFCTRL_REFSEL_INT1V;
    
    /* 256x oversampling and decimation for 16 bit effective resolution */
    ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_256 | ADC_AVGCTRL_ADJRES(0);
    
    /* Configure Control B register */
    // Calculate prescaler value
    uint32_t prescaler = 30 - __builtin_clz((8000000UL - 1) / 2100000UL);
    // Make sure prescaler is at most 512
    prescaler &= 0x1FF;
    
    // Set prescaler, 16 bit output, freerunning mode
    ADC->CTRLB.reg = (ADC_CTRLB_PRESCALER(prescaler) |
                      ADC_CTRLB_RESSEL_16BIT);
    // Wait for synchronization
    while (ADC->STATUS.bit.SYNCBUSY);
    
    /* Enable temparature reference */
    SYSCTRL->VREF.reg |= SYSCTRL_VREF_TSEN;
    
    ADC->INTENSET.bit.RESRDY = 1;
    // Enable ADC interrupt in NVIC
    NVIC_SetPriority(ADC_IRQn, 3);
    NVIC_EnableIRQ(ADC_IRQn);
    
    /* Enable ADC */
    ADC->CTRLA.bit.ENABLE = 1;
    // Wait for synchronization
    while (ADC->STATUS.bit.SYNCBUSY);
#endif
}

#define DEBUG_ADC_READ_NAME  "adc-read"
#define DEBUG_ADC_READ_HELP  "Read ADC\n"\
                             "Usage: adc-read <scan start> [scan end]"

#ifndef ENABLE_ADC
static volatile uint16_t adc_results[0x1C];
static volatile uint32_t adc_result_ready;
static volatile uint8_t adc_current_chan;
static volatile uint8_t adc_last_chan;

static void print_adc_result (struct console_desc_t *console, uint8_t channel)
{
    char str[8];
    
    while (!(adc_result_ready & (1 << channel))) {
        wdt_pat();
    }
    adc_result_ready &= ~(1 << channel);
    
    console_send_str(console, "Result (0x");
    utoa(channel, str, 16);
    console_send_str(console, str);
    console_send_str(console, "): 0x");
    utoa(adc_results[channel], str, 16);
    console_send_str(console, str);
    console_send_str(console, "\n");
}
#endif

static void debug_adc_read (uint8_t argc, char **argv,
                            struct console_desc_t *console)
{
#ifdef ENABLE_ADC
    console_send_str(console, "ADC driver is enabled. Use \"analog\" command "
                     "instead\n");
#else
    
    if ((argc < 2) || (argc > 3)) {
        console_send_str(console, DEBUG_ADC_READ_HELP);
        console_send_str(console, "\n");
        return;
    }
    
    char* end;
    
    uint32_t scan_start = strtoul(argv[1], &end, 0);
    if ((*end != '\0') || (scan_start > 0x1C)) {
        console_send_str(console, DEBUG_ADC_READ_HELP);
        console_send_str(console, "\n");
        return;
    }
    
    uint32_t scan_end;
    if (argc == 3) {
        scan_end = strtoul(argv[2], &end, 0);
        if ((*end != '\0') || (scan_end > 0x1C) || (scan_end < scan_start)) {
            console_send_str(console, DEBUG_ADC_READ_HELP);
            console_send_str(console, "\n");
            return;
        }
    } else {
        scan_end = scan_start;
    }
    adc_last_chan = scan_end;
    
    ADC->INPUTCTRL.reg = (ADC_INPUTCTRL_GAIN_1X |
                          ADC_INPUTCTRL_INPUTOFFSET(0) |
                          ADC_INPUTCTRL_INPUTSCAN(scan_end - scan_start) |
                          ADC_INPUTCTRL_MUXNEG_GND |
                          ADC_INPUTCTRL_MUXPOS(scan_start));
    
    adc_current_chan = scan_start;
    
    ADC->CTRLB.bit.FREERUN = 0b1;
    // Wait for synchronization
    while (ADC->STATUS.bit.SYNCBUSY);
    
    for (uint8_t i = scan_start; i <= scan_end; i++) {
        print_adc_result(console, i);
    }
#endif
}

#ifndef ENABLE_ADC
void ADC_Handler (void)
{
    if (ADC->INTFLAG.bit.RESRDY) {
        adc_results[adc_current_chan] = ADC->RESULT.reg;
        adc_result_ready |= (1 << adc_current_chan);
        
        adc_current_chan++;
        
        if (adc_current_chan > adc_last_chan) {
            ADC->CTRLB.bit.FREERUN = 0b0;
            ADC->SWTRIG.bit.FLUSH = 0b1;
        }
    }
}
#endif


#define DEBUG_DAC_NAME  "dac"
#define DEBUG_DAC_HELP  "Control DAC.\nUsage: dac [value] [volts/raw]"

enum debug_dac_mode {
    DEBUG_DAC_MODE_VOLTS,
    DEBUG_DAC_MODE_RAW
};

static uint16_t parse_value(char *str, char **end, uint8_t *has_decimal)
{
    uint16_t value = (uint16_t)strtoul(str, end, 0);
    
    if (**end == '.') {
        // Number has decimal
        *has_decimal = 1;
        value *= 1000;
        
        char *s = *end + 1;
        
        if (*s == '\0') {
            return 0;
        }
        
        for (uint16_t weight = 100; weight != 0; weight /= 10) {
            if (isdigit((unsigned char)*s)) {
                value += weight * (*s - '0');
            } else {
                break;
            }
            s++;
        }
        *end = s;
    }
    
    return value;
}

static void debug_dac (uint8_t argc, char **argv,
                       struct console_desc_t *console)
{
    static uint8_t dac_initialized;
    
    uint8_t has_decimal = 0;
    uint16_t value = 0;
    enum debug_dac_mode mode = DEBUG_DAC_MODE_RAW;
    
    if (!dac_initialized && (argc != 3)) {
        console_send_str(console, "DAC not initialize, run 'dac init 1V' or "\
                                  "'dac init 3.3V'.\n");
        return;
    }
    
    if (argc == 2) {
        // One argument provided, it should be a value
        
        char* end;
        value = parse_value(argv[1], &end, &has_decimal);
        
        if (!strcasecmp(end, "v")) {
            mode = DEBUG_DAC_MODE_VOLTS;
        } else if (*end != '\0') {
            console_send_str(console, DEBUG_DAC_HELP);
            console_send_str(console, "\n");
            return;
        }
    } else if (argc == 3) {
        if (!strncasecmp(argv[1], "init", 4)) {
            if (!strcasecmp(argv[2], "1v")) {
                init_dac(GCLK_CLKCTRL_GEN_GCLK0, DAC_REF_1V, 1, 1);
            } else if (!strcasecmp(argv[2], "3.3v")) {
                init_dac(GCLK_CLKCTRL_GEN_GCLK0, DAC_REF_AVCC, 1, 1);
            } else {
                console_send_str(console, "DAC not initialize, run 'dac init "\
                                          "1V' or 'dac init 3.3V'.\n");
                return;
            }
            dac_initialized = 1;
            return;
        } else if (dac_initialized) {
            // Two arguments provided, the first should be a value and the
            // second should be a unit
            char* end;
            value = parse_value(argv[1], &end, &has_decimal);
            
            if (*end != '\0') {
                console_send_str(console, DEBUG_DAC_HELP);
                console_send_str(console, "\n");
                return;
            }
            
            if (!strcasecmp(argv[2], "v") || !strcasecmp(argv[2], "volts")) {
                mode = DEBUG_DAC_MODE_VOLTS;
            } else if (!strcasecmp(argv[2], "raw")) {
                mode = DEBUG_DAC_MODE_RAW;
            } else {
                console_send_str(console, DEBUG_DAC_HELP);
                console_send_str(console, "\n");
                return;
            }
        } else {
            console_send_str(console, "DAC not initialize, run 'dac init 1V' "\
                             "or 'dac init 3.3V'.\n");
            return;
        }
    } else if (argc > 3) {
        console_send_str(console, DEBUG_DAC_HELP);
        return;
    }
    
    // Check that if a value with a decimal was provided, it is a voltage
    if (has_decimal && (mode == DEBUG_DAC_MODE_RAW)) {
        console_send_str(console, "Raw value must be an integer.\n");
        return;
    }
    
    // Update DAC Value
    if (argc != 1) {
        if (mode == DEBUG_DAC_MODE_VOLTS) {
            if (!has_decimal) {
                value *= 1000;
            }
            dac_set_millivolts(value);
        } else if (mode == DEBUG_DAC_MODE_RAW) {
            dac_set(value);
        }
    }
    
    char str[8];
    
    // Print current DAC value
    console_send_str(console, "DAC value: ");
    utoa(dac_get_value(), str, 10);
    console_send_str(console, str);
    console_send_str(console, " (");
    print_fixed_point(console, dac_get_value_millivolts(), 3);
    console_send_str(console, ")\n");
}

#define DEBUG_GPIO_NAME  "gpio"
#define DEBUG_GPIO_HELP  "Control gpio pins."\
                         "\nUsage: gpio mode <pin> <input/output/strong/pull>"\
                         "\n       gpio pull <pin> <high/low/none>"\
                         "\n       gpio out <pin> <high/low>"\
                         "\n       gpio status <pin>"\
                         "\n       gpio in <pin>"\
                         "\n       gpio poll <pin>"\
                         "\n<pin> = g<0 to 23>           : header gpio pin"\
                         "\n<pin> = p<a/b><0 to 31>      : internal pin"\
                         "\n<pin> = e<a/b><0 to 7>       : IO expander pin"\
                         "\n<pin> = r<radio #>.<0 to 17> : RN2483 pin"

#define NUM_HEADER_PINS 24
static const union gpio_pin_t HEADER_PINS[] = {
                                                GPIO_0,  GPIO_1,  GPIO_2,
                                                GPIO_3,  GPIO_4,  GPIO_5,
                                                GPIO_6,  GPIO_7,  GPIO_8,
                                                GPIO_9,  GPIO_10, GPIO_11,
                                                GPIO_12, GPIO_13, GPIO_14,
                                                GPIO_15, GPIO_16, GPIO_17,
                                                GPIO_18, GPIO_19, GPIO_20,
                                                GPIO_21, GPIO_22, GPIO_23
                                              };

static void debug_gpio (uint8_t argc, char **argv,
                        struct console_desc_t *console)
{
    if ((argc < 3) || (argc > 4)) {
        // Not enough arguments
        console_send_str(console, DEBUG_GPIO_HELP);
        console_send_str(console, "\n");
        return;
    }
    
    char str[10];
    
    // Create pin with invalid value
    union gpio_pin_t pin = { .raw = 0xFFFF };
    
    /* Parse pin */
    char* end;
    if (argv[2][0] == 'g' || isdigit((unsigned char)argv[2][0])) {
        // Try and parse pin as a header pin number
        uint32_t pin_num = strtoul(argv[2] + (argv[2][0] == 'g'), &end, 10);
        if ((*end == '\0') && (pin_num < NUM_HEADER_PINS)) {
            pin = HEADER_PINS[pin_num];
        }
    } else if ((argv[2][0] == 'p') && ((argv[2][1] == 'a') || (argv[2][1] == 'b'))) {
        // Try and parse pin as an internal pin number
        uint32_t pin_num = strtoul(argv[2] + 2, &end, 10);
        if ((*end == '\0') && (pin_num < 32)) {
            pin.type = GPIO_INTERNAL_PIN;
            pin.internal.port = (argv[2][1] == 'b');
            pin.internal.pin = pin_num;
        }
    } else if ((argv[2][0] == 'e') && ((argv[2][1] == 'a') || (argv[2][1] == 'b'))) {
        // Try and parse pin as an IO expander pin number
        uint32_t pin_num = strtoul(argv[2] + 2, &end, 10);
        if ((*end == '\0') && (pin_num < 8)) {
            pin.type = GPIO_MCP23S17_PIN;
            pin.mcp23s17.port = (argv[2][1] == 'a') ? MCP23S17_PORT_A : MCP23S17_PORT_B;
            pin.mcp23s17.pin = pin_num;
        }
    } else if (argv[2][0] == 'r') {
        // Try and parse pin as a RN2483 pin number
        uint32_t radio_num = strtoul(argv[2] + 1, &end, 10);
        if ((*end == '.') && (radio_num < 8)) {
            uint32_t pin_num = strtoul(end + 1, &end, 10);
            if ((*end == '\0') && (pin_num < RN2483_NUM_PINS)) {
                pin.type = GPIO_RN2483_PIN;
                pin.rn2483.radio = radio_num;
                pin.rn2483.pin = pin_num;
            }
        }
    }
#ifdef DEBUG0_LED_PIN
    else if (!strcmp(argv[2], "DEBUG0")) {
        pin = DEBUG0_LED_PIN;
    }
#endif
#ifdef DEBUG1_LED_PIN
    else if (!strcmp(argv[2], "DEBUG1")) {
        pin = DEBUG1_LED_PIN;
    }
#endif
#ifdef STAT_R_LED_PIN
    else if (!strcmp(argv[2], "STAT_R")) {
        pin = STAT_R_LED_PIN;
    }
#endif
#ifdef STAT_G_LED_PIN
    else if (!strcmp(argv[2], "STAT_G")) {
        pin = STAT_G_LED_PIN;
    }
#endif
#ifdef STAT_B_LED_PIN
    else if (!strcmp(argv[2], "STAT_B")) {
        pin = STAT_B_LED_PIN;
    }
#endif
    
    if (pin.raw == 0xFFFF) {
        // Pin was not valid
        console_send_str(console, "\"");
        console_send_str(console, argv[2]);
        console_send_str(console, "\" is not a valid pin.\n");
        return;
    }
    
    // Run command
    if (!strcmp(argv[1], "mode")) {
        if (argc < 4) {
            // Not enough arguments
            console_send_str(console, "Mode command requires an arguemnt.\n");
            return;
        }
        
        enum gpio_pin_mode mode = GPIO_PIN_DISABLED;
        
        if (!strcmp(argv[3], "input") || !strcmp(argv[3], "in")) {
            mode = GPIO_PIN_INPUT;
        } else if (!strcmp(argv[3], "output") || !strcmp(argv[3], "out")) {
            mode = GPIO_PIN_OUTPUT_TOTEM;
        } else if (!strcmp(argv[3], "strong")) {
            mode = GPIO_PIN_OUTPUT_STRONG;
        } else if (!strcmp(argv[3], "pull")) {
            mode = GPIO_PIN_OUTPUT_PULL;
        } else {
            console_send_str(console, "\"");
            console_send_str(console, argv[3]);
            console_send_str(console, "\" is not a valid pin mode.\n");
            return;
        }
        
        if (gpio_set_pin_mode(pin, mode)) {
            console_send_str(console, "Could not set pin mode.\n");
        }
    } else if (!strcmp(argv[1], "pull")) {
        if (argc < 4) {
            // Not enough arguments
            console_send_str(console, "Pull command requires an arguemnt.\n");
            return;
        }
        
        enum gpio_pull_mode pull;
        
        if (!strcmp(argv[3], "high")) {
            pull = GPIO_PULL_HIGH;
        } else if (!strcmp(argv[3], "low")) {
            pull = GPIO_PULL_LOW;
        } else if (!strcmp(argv[3], "none")) {
            pull = GPIO_PULL_NONE;
        } else {
            console_send_str(console, "\"");
            console_send_str(console, argv[3]);
            console_send_str(console, "\" is not a valid pull type.\n");
            return;
        }
        
        if (gpio_set_pull(pin, pull)) {
            console_send_str(console, "Could not set pull.\n");
        }
    } else if (!strcmp(argv[1], "out")) {
        if (argc < 4) {
            // Not enough arguments
            console_send_str(console, "Out command requires an arguemnt.\n");
            return;
        }
        
        uint8_t value = 0;
        
        if (!strcmp(argv[3], "high") || !strcmp(argv[3], "1")) {
            value = 1;
        } else if (!strcmp(argv[3], "low") || !strcmp(argv[3], "0")) {
            value = 0;
        } else {
            console_send_str(console, "\"");
            console_send_str(console, argv[3]);
            console_send_str(console, "\" is not a valid pin value.\n");
            return;
        }
        
        if (gpio_set_output(pin, value)) {
            console_send_str(console, "Could not set output.\n");
        }
    } else if (!strcmp(argv[1], "status")) {
        if (argc > 3) {
            // Too many arguments
            console_send_str(console, "Too many arguments for status command.\n");
            return;
        }
        
        uint8_t out_val = 0;
        
        switch (pin.type) {
            case GPIO_INTERNAL_PIN:
                console_send_str(console, "Internal pin: Port ");
                console_send_str(console, pin.internal.port ? "B" : "A");
                console_send_str(console, " Pin ");
                utoa(pin.internal.pin, str, 10);
                console_send_str(console, str);
                
                out_val = !!(PORT->Group[pin.internal.port].OUT.reg & (1 << pin.internal.pin));
                break;
            case GPIO_MCP23S17_PIN:
                console_send_str(console, "IO expander pin: Port ");
                console_send_str(console, pin.mcp23s17.port ? "B" : "A");
                console_send_str(console, " Pin ");
                utoa(pin.mcp23s17.pin, str, 10);
                console_send_str(console, str);
                
                out_val = !!(io_expander_g.registers.OLAT[pin.mcp23s17.port].reg & (1 << pin.mcp23s17.pin));
                break;
            case GPIO_RN2483_PIN:
                console_send_str(console, "RN2483 pin: Radio ");
                utoa(pin.rn2483.radio, str, 10);
                console_send_str(console, str);
                console_send_str(console, " Pin ");
                utoa(pin.rn2483.pin, str, 10);
                console_send_str(console, str);
                
                out_val = rn2483_g.pins[pin.rn2483.pin].value;
                break;
            case GPIO_RFM69HCW_PIN:
                // Not yet supported
                break;
        }
        
        console_send_str(console, "\n  Mode: ");
        
        switch (gpio_get_pin_mode(pin)) {
            case GPIO_PIN_DISABLED:
                console_send_str(console, "disabled\n");
                break;
            case GPIO_PIN_OUTPUT_PULL:
                console_send_str(console, "output - pull\n");
                break;
            case GPIO_PIN_OUTPUT_TOTEM:
                console_send_str(console, "output - totem\n");
                break;
            case GPIO_PIN_OUTPUT_STRONG:
                console_send_str(console, "output - strong\n");
                break;
            case GPIO_PIN_INPUT:
                console_send_str(console, "input\n");
                break;
        }
        
        if (gpio_get_pin_mode(pin) != GPIO_PIN_DISABLED) {
            console_send_str(console, "  Value: ");
            if (gpio_get_pin_mode(pin) == GPIO_PIN_INPUT) {
                utoa(gpio_get_input(pin), str, 10);
            } else {
                utoa(out_val, str, 10);
            }
            console_send_str(console, str);
            console_send_str(console, "\n");
        }
    } else if (!strcmp(argv[1], "in")) {
        if (argc > 3) {
            // Too many arguments
            console_send_str(console, "Too many arguments for in command.\n");
            return;
        }
        
        console_send_str(console, "Value: ");
        utoa(gpio_get_input(pin), str, 10);
        console_send_str(console, str);
        console_send_str(console, "\n");
    } else if (!strcmp(argv[1], "poll")) {
        if (argc > 3) {
            // Too many arguments
            console_send_str(console, "Too many arguments for poll command.\n");
            return;
        }
        
        switch (pin.type) {
            case GPIO_INTERNAL_PIN:
                // Does not need to be polled
                break;
            case GPIO_MCP23S17_PIN:
                mcp23s17_poll(&io_expander_g);
                while (mcp23s17_poll_in_progress(&io_expander_g)) {
                    mcp23s17_service(&io_expander_g);
                }
                break;
            case GPIO_RN2483_PIN:
                rn2483_poll_gpio_pin(&rn2483_g, pin.rn2483.pin);
                while (rn2483_poll_gpio_pin_in_progress(&rn2483_g, pin.rn2483.pin)) {
                    rn2483_service(&rn2483_g);
                }
                break;
            case GPIO_RFM69HCW_PIN:
                // Not yet supported
                break;
        }
        
        console_send_str(console, "Value: ");
        utoa(gpio_get_input(pin), str, 10);
        console_send_str(console, str);
        console_send_str(console, "\n");
    } else {
        // Not a valid command
        console_send_str(console, "\"");
        console_send_str(console, argv[1]);
        console_send_str(console, "\" is not a valid command.\n");
    }
}


const uint8_t debug_commands_num_funcs = 23;
const struct cli_func_desc_t debug_commands_funcs[] = {
    {.func = debug_version, .name = DEBUG_VERSION_NAME, .help_string = DEBUG_VERSION_HELP},
    {.func = debug_did, .name = DEBUG_DID_NAME, .help_string = DEBUG_DID_HELP},
    {.func = debug_i2c_scan, .name = DEBUG_I2C_SCAN_NAME, .help_string = DEBUG_I2C_SCAN_HELP},
    {.func = debug_alt_prom, .name = DEBUG_ALT_PROM_NAME, .help_string = DEBUG_ALT_PROM_HELP},
    {.func = debug_imu_wai, .name = DEBUG_IMU_WAI_NAME, .help_string = DEBUG_IMU_WAI_HELP},
    {.func = debug_io_exp_regs, .name = DEBUG_IO_EXP_REGS_NAME, .help_string = DEBUG_IO_EXP_REGS_HELP},
    {.func = debug_temp, .name = DEBUG_TEMP_NAME, .help_string = DEBUG_TEMP_HELP},
    {.func = debug_analog, .name = DEBUG_ANALOG_NAME, .help_string = DEBUG_ANALOG_HELP},
    {.func = debug_alt, .name = DEBUG_ALT_NAME, .help_string = DEBUG_ALT_HELP},
    {.func = debug_alt_tare_now, .name = DEBUG_ALT_TARE_NOW_NAME, .help_string = DEBUG_ALT_TARE_NOW_HELP},
    {.func = debug_alt_tare_next, .name = DEBUG_ALT_TARE_NEXT_NAME, .help_string = DEBUG_ALT_TARE_NEXT_HELP},
    {.func = debug_gnss, .name = DEBUG_GNSS_NAME, .help_string = DEBUG_GNSS_HELP},
    {.func = debug_lora_version, .name = DEBUG_LORA_VERSION_NAME, .help_string = DEBUG_LORA_VERSION_HELP},
    {.func = debug_radio_send, .name = DEBUG_RADIO_SEND_NAME, .help_string = DEBUG_RADIO_SEND_HELP},
    {.func = debug_radio_count, .name = DEBUG_RADIO_COUNT_NAME, .help_string = DEBUG_RADIO_COUNT_HELP},
    {.func = debug_radio_recv, .name = DEBUG_RADIO_RECV_NAME, .help_string = DEBUG_RADIO_RECV_HELP},
    {.func = debug_telem_test, .name = DEBUG_TELEM_TEST_NAME, .help_string = DEBUG_TELEM_TEST_HELP},
    {.func = debug_telem_pause, .name = DEBUG_TELEM_PAUSE_NAME, .help_string = DEBUG_TELEM_PAUSE_HELP},
    {.func = debug_telem_resume, .name = DEBUG_TELEM_RESUME_NAME, .help_string = DEBUG_TELEM_RESUME_HELP},
    {.func = debug_adc_init, .name = DEBUG_ADC_INIT_NAME, .help_string = DEBUG_ADC_INIT_HELP},
    {.func = debug_adc_read, .name = DEBUG_ADC_READ_NAME, .help_string = DEBUG_ADC_READ_HELP},
    {.func = debug_dac, .name = DEBUG_DAC_NAME, .help_string = DEBUG_DAC_HELP},
    {.func = debug_gpio, .name = DEBUG_GPIO_NAME, .help_string = DEBUG_GPIO_HELP}
};
