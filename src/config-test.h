/**
 * @file config-test.h
 * @desc Hardware configuration for testing purposes
 * @author Samuel Dewan
 * @date 2019-06-11
 * Last Author:
 * Last Edited On:
 */

#ifndef config_test_h
#define config_test_h

#include "sercom-spi.h"
#include "sercom-i2c.h"
#include "sercom-uart.h"

#include "gpio.h"

#include "ms5611.h"
#include "rn2483.h"

//
//
//  General
//
//

/* String to identify this configuration */
#define CONFIG_STRING "Test Config\n"
/* Globally enable DMA if defined */
#define ENABLE_DMA
/* Enable Micro Trace Buffer if defined */
#define ENABLE_MTB

//
//
//  SPI
//
//

/* SERCOM instance to be used for SPI, SPI is disabled if not defined */
#define SPI_SERCOM_INST SERCOM4
/* DMA Channel used for SPI receive, DMA not used if not defined or defined as -1 */
#define SPI_RX_DMA_CHAN 4
/* DMA Channel used for SPI transmit, DMA not used if not defined or defined as -1 */
#define SPI_TX_DMA_CHAN 5
/* SPI Instance */
extern struct sercom_spi_desc_t spi_g;


//
//
//  I2C
//
//

/* SERCOM instance to be used for I2C, I2C is disabled if not defined */
#define I2C_SERCOM_INST SERCOM5
/* DMA Channel used for I2C, DMA not used if not defined or defined as -1 */
//#define I2C_DMA_CHAN 6
/* I2C Speed, defaults to I2C_MODE_STANDARD (100 KHz) if not defined */
#define I2C_SPEED I2C_MODE_FAST
/* I2C Instance */
extern struct sercom_i2c_desc_t i2c_g;


//
//
//  UARTs
//
//

// UART0
/* SERCOM instance to be used for UART, UART is disabled if not defined */
#define UART0_SERCOM_INST SERCOM0
/* DMA Channel used for UART TX, DMA not used if not defined or defined as -1 */
#define UART0_DMA_CHAN 7
/* Baud rate for UART */
#define UART0_BAUD 57600UL
/* Define as one if UART should echo received bytes and provide line editing,
 0 otherwise */
#define UART0_ECHO 0
/* UART Instance */
extern struct sercom_uart_desc_t uart0_g;

// UART1
/* SERCOM instance to be used for UART, UART is disabled if not defined */
#define UART1_SERCOM_INST SERCOM1
/* DMA Channel used for UART TX, DMA not used if not defined or defined as -1 */
#define UART1_DMA_CHAN 8
/* Baud rate for UART */
#define UART1_BAUD 57600UL
/* Define as one if UART should echo received bytes and provide line editing,
 0 otherwise */
#define UART1_ECHO 0
/* UART Instance */
extern struct sercom_uart_desc_t uart1_g;

// UART2
/* SERCOM instance to be used for UART, UART is disabled if not defined */
#define UART2_SERCOM_INST SERCOM2
/* DMA Channel used for UART TX, DMA not used if not defined or defined as -1 */
#define UART2_DMA_CHAN 9
/* Baud rate for UART */
#define UART2_BAUD 115200UL
/* Define as one if UART should echo received bytes and provide line editing,
 0 otherwise */
#define UART2_ECHO 0
/* UART Instance */
extern struct sercom_uart_desc_t uart2_g;

// UART3
/* SERCOM instance to be used for UART, UART is disabled if not defined */
#define UART3_SERCOM_INST SERCOM3
/* DMA Channel used for UART TX, DMA not used if not defined or defined as -1 */
#define UART3_DMA_CHAN 10
/* Baud rate for UART */
#define UART3_BAUD 9600UL
/* Define as one if UART should echo received bytes and provide line editing,
 0 otherwise */
#define UART3_ECHO 0
/* UART Instance */
extern struct sercom_uart_desc_t uart3_g;

//
//
//  USB
//
//

/* USB enabled if defined */
#define ENABLE_USB
/* Define to enable USB CDC port 0 */
#define ENABLE_USB_CDC_PORT_0
/* Define to enable echo on USB CDC port 0 */
#define USB_CDC_PORT_0_ECHO
/* Define to enable USB CDC port 1 */
//#define ENABLE_USB_CDC_PORT_1
/* Define to enable echo on USB CDC port 1 */
//#define USB_CDC_PORT_1_ECHO
/* Define to enable USB CDC port 2 */
//#define ENABLE_USB_CDC_PORT_2
/* Define to enable echo on USB CDC port 2 */
//#define USB_CDC_PORT_2_ECHO
// Note: USB CDC ports should be enabled in order to minimize memory usage

//
//
// Console
//
//

/* Debugging console enabled if defined */
#define ENABLE_CONSOLE
/* UART instance to be used for console, USB is used if not defined (and USB is
 enabled) */
//#define CONSOLE_UART uart3_g
/* USB CDC port to be used for debugging console, this is ignored if
   CONSOLE_UART is defined */
#define CONSOLE_CDC_PORT 0

//
//
//  MCP23S17 GPIO Expander
//
//

/* IO Expander enabled if defined */
#define ENABLE_IO_EXPANDER
/* IO Expander Instance */
extern struct mcp23s17_desc_t io_expander_g;

//
//
//  Analog
//
//

/* ADC enabled if defined */
#define ENABLE_ADC
/* Period between ADC sweeps in milliseconds */
#define ADC_PERIOD 2000
/* DMA Channel used for ADC results, DMA not used if not defined or defined
 as -1 */
#define ADC_DMA_CHAN 11
/* Maximum impedance of source in ohms, see figure 37-5 in SAMD21 datasheet */
#define ADC_SOURCE_IMPEDANCE 100000

//
//
//  Debug CLI
//
//

/* Debug CLI enabled if defined */
#define ENABLE_DEBUG_CLI
/* Prompt for DEBUG CLI */


//
//
//  Radio
//
//
/* RN2483 enabled if defined */
#define ENABLE_LORA_RADIO
/*  The uart instance used to communicate with the radio */
#define LORA_UART uart1_g
/*  Centre frequency in hertz, from 433050000 to 434790000 */
#define LORA_FREQ 433050000
/*  Power level in dBm, from -3 to 14 */
#define LORA_POWER 14
/*  LoRa spreading factor */
#define LORA_SPREADING_FACTOR RN2483_SF_SF9
/*  LoRa coding rate */
#define LORA_CODING_RATE RN2483_CR_4_7
/*  Bandwidth */
#define LORA_BANDWIDTH RN2483_BW_500
/*  Whether a CRC should be added to the data */
#define LORA_CRC 0
/*  Whether the I and Q streams should be inverted */
#define LORA_INVERT_IQ 0
/*  Sync word */
#define LORA_SYNC_WORD 0x43
#ifdef ENABLE_LORA_RADIO
extern struct rn2483_desc_t rn2483_g;
#endif

//
//
//  Altimeter
//
//

/* Altimeter enabled if defined */
#define ENABLE_ALTIMETER
/* Altimeter CSB setting */
#define ALTIMETER_CSB 0
/* Altimeter sample period in milliseconds */
#define ALTIMETER_PERIOD 1000
extern struct ms5611_desc_t altimeter_g;

//
//
// GNSS
//
//

/* GNSS enabled if defined */
#define ENABLE_GNSS
/* UART used to communicate with GNSS */
#define GNSS_UART uart2_g

//
//
// SD
//
//

#define ENABLE_SD
#define SD_BAUDRATE 12000000 //12M baud

#endif /* config_test_h */
