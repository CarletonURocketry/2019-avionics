/**
 * @file sercom-uart.h
 * @desc SERCOM UART mode driver which allows interrupt or DMA driven transfers.
 * @author Samuel Dewan
 * @date 2018-12-27
 * Last Author:
 * Last Edited On:
 */

#ifndef sercom_uart_h
#define sercom_uart_h

#include "global.h"

#include "circular-buffer.h"
#include "dma.h"

/** The length of the circular output buffer for SERCOM UART instances */
#define SERCOM_UART_OUT_BUFFER_LEN  256
/** The length of the circular input buffer for SERCOM UART instances */
#define SERCOM_UART_IN_BUFFER_LEN  256

/**
 *  Descriptor for the a SERCOM UART driver instance
 */
struct sercom_uart_desc_t {
    Sercom *sercom;
    
    /** Circular buffer for data to be transmitted */
    char out_buffer_mem[SERCOM_UART_OUT_BUFFER_LEN];
    struct circular_buffer_t out_buffer;
    /** Circular buffer for received data */
    char in_buffer_mem[SERCOM_UART_IN_BUFFER_LEN];
    struct circular_buffer_t in_buffer;
    
    uint8_t sercom_instnum;
    
    /** DMA channel for data transmission */
    uint8_t dma_chan:4;
    uint8_t use_dma:1;
    
    uint8_t echo:1;
    
    /** Flag used to unsure that the service function is not executed in an
     interrupt while it is already being run in the main thread */
    uint8_t service_lock:1;
    
    struct dma_circ_transfer_t dma_tran;
};

/**
 *  Initialize a SERCOM instance for use as a serial console
 *
 *  @param descriptor The descriptor to be populated for this console instance.
 *  @param sercom Pointer to the SERCOM instance's registers.
 *  @param baudrate Desired baudrate for communication.
 *  @param core_freq The frequency of the core clock for the SERCOM instance.
 *  @param core_clock_mask The mask for the generator to be used for the SERCOM
 *                         core clock;
 *  @param dma_channel The DMA channel to be used for transmission or a negative
 *                     value for interrupt driven communication.
 *  @param echo If true bytes received will be treated as characters and
 *              echoed and simple line editing (backspace) will be possible.
 */
extern void init_sercom_uart(struct sercom_uart_desc_t *descriptor,
                             Sercom *sercom, uint32_t baudrate,
                             uint32_t core_freq, uint32_t core_clock_mask,
                             int8_t dma_channel, uint8_t echo);

/**
 *  Queue a string to be written to the UART.
 *
 *  @param uart The UART to which the string should be written.
 *  @param str The string to be written.
 *
 *  @return The number of characters which could be added to the queue.
 */
extern uint16_t sercom_uart_put_string(struct sercom_uart_desc_t *uart,
                                       const char *str);

/**
 *  Queue a string to be written to the UART. If there is not enough space
 *  for the string in the buffer, block until there is.
 *
 *  @param uart The UART to which the string should be written.
 *  @param str The string to be written.
 */
extern void sercom_uart_put_string_blocking(struct sercom_uart_desc_t *uart,
                                            const char *str);

/**
 *  Queue a byte array to be written to the UART.
 *
 *  @param uart The UART to which the array should be written.
 *  @param bytes The array to be written
 *  @param length The number of bytes to be written
 *
 *  @return The number of bytes which could be added to the queue.
 */
extern uint16_t sercom_uart_put_bytes(struct sercom_uart_desc_t *uart,
                                      const uint8_t *bytes, uint16_t length);

/**
 *  Queue a string to be written to the UART. If there is not enough space
 *  for the string in the buffer, block until there is.
 *
 *  @param uart The UART to which the string should be written.
 *  @param bytes The array to be written
 *  @param length The number of bytes to be written
 */
extern void sercom_uart_put_bytes_blocking(struct sercom_uart_desc_t *uart,
                                           const uint8_t *bytes,
                                           uint16_t length);

/**
 *  Write a character to a UART.
 *
 *  @param uart The UART to which the character should be written.
 *  @param c The character to be written
 */
extern void sercom_uart_put_char (struct sercom_uart_desc_t *uart, char c);

/**
 *  Get string from UART input buffer.
 *
 *  @param uart The UART from which the string should be retrieved.
 *  @param str The string in which the data should be stored.
 *  @param len The maximum number of chars to be read from the buffer.
 */
extern void sercom_uart_get_string (struct sercom_uart_desc_t *uart, char *str,
                                    uint16_t len);

/**
 *  Determine if there is a full line, as delimited by the provided char,
 *  available to be read from the UART buffer.
 *
 *  @param uart The UART for which the availability of a line should be
 *         determined.
 *  @param delim The delimiter for new lines (ie. '\n').
 *
 *  @return 0 if there is no line available, 1 if a line is available.
 */
extern uint8_t sercom_uart_has_delim (struct sercom_uart_desc_t *uart,
                                      char delim);

/**
 *  Read a string from the input buffer up to the next occurrence of a
 *  delimiter.
 *
 *  @param uart The UART from which the line should be retrieved.
 *  @param delim The delimiter for new lines (ie. '\n').
 *  @param str The string in which the data should be stored.
 *  @param len The maximum number of chars to be read from the input buffer.
 */
extern void sercom_uart_get_line_delim (struct sercom_uart_desc_t *uart,
                                        char delim, char *str, uint16_t len);

/**
 *  Determine if there is a full line, delimited by "\r\n", available to be read
 *  from the UART buffer.
 *
 *  @param uart The UART for which the availability of a line should be
 *         determined.
 *
 *  @return 0 if there is no line available, 1 if a line is available.
 */
extern uint8_t sercom_uart_has_line (struct sercom_uart_desc_t *uart);

/**
 *  Read a string from the input buffer up to the next occurrence of "\r\n".
 *
 *  @param uart The UART from which the line should be retrieved.
 *  @param str The string in which the data should be stored.
 *  @param len The maximum number of chars to be read from the input buffer.
 */
extern void sercom_uart_get_line (struct sercom_uart_desc_t *uart, char *str,
                                  uint16_t len);

/**
 *  Get a character from the UART input buffer.
 *
 *  @param uart The UART from which the character should be retrieved.
 *
 *  @return The least recently received character in the input buffer
 */
extern char sercom_uart_get_char (struct sercom_uart_desc_t *uart);

/**
 *  Determine if the out buffer of a UART is empty.
 *
 *  @param uart The UART for which the empty-ness of the out buffer should
 *                 be determined.
 */
extern uint8_t sercom_uart_out_buffer_empty (struct sercom_uart_desc_t *uart);

#endif /* sercom_uart_h */
