/**
 * @file console.h
 * @desc Abstracts over sercom-uart or usb-serial console.
 * @author Samuel Dewan
 * @date 2019-09-20
 * Last Author: Samuel Dewan
 * Last Edited On: 2019-11-30
 */

#ifndef console_h
#define console_h

#include "sercom-uart.h"


enum console_type {
    CONSOLE_TYPE_UART,
#ifdef ID_USB
    CONSOLE_TYPE_USB_CDC
#endif
};

/**
 *  Descriptor for a console instance.
 */
struct console_desc_t {
    union {
        struct sercom_uart_desc_t *uart;
#ifdef ID_USB
        uint8_t usb_cdc;
#endif
    } interface;
    
    void (*line_callback)(char*, struct console_desc_t*, void*);
    void (*init_callback)(struct console_desc_t*, void*);
    void *callback_context;
    
    char line_delimiter;
    enum console_type type;
};


/**
 *  Initializes a console instance backed by a SERCOM UART.
 *
 *  @param console The console descriptor to be initialized
 *  @param uart Pointer to a sercom uart descriptor
 *  @param line_delim The delimiter for new lines received, if line_delim is
 *                    '\0' the sequence "\r\n" will be used as the line
 *                    delimiter
 */
extern void init_uart_console (struct console_desc_t *console,
                               struct sercom_uart_desc_t *uart,
                               const char line_delim);

#ifdef ID_USB
/**
 *  Initializes a console instance backed by a USB CDC interface.
 *
 *  @param console The console descriptor to be initialized
 *  @param usb_cdc_interface USB CDC interface number
 *  @param line_delim The delimiter for new lines received, if line_delim is
 *                    '\0' the sequence "\r\n" will be used as the line
 *                    delimiter
 */
extern void init_usb_cdc_console (struct console_desc_t *console,
                                  uint8_t usb_cdc_interface,
                                  const char line_delim);
#endif

/**
 *  Send a string on a console. This function will block until the whole string
 *  is copied in to the buffer even if the whole string does not initially fit
 *  in the available free space.
 *
 *  @param console The console on which the string should be sent
 *  @param str The string to be sent
 */
extern void console_send_str (struct console_desc_t *console, const char *str);

/**
 *  Send a string on a console. This function copies the string into the output
 *  buffer and then returns immediately. If there is not enough space, the whole
 *  string may not be copied. The number of bytes which could be copied to the
 *  buffer is returned.
 *
 *  @param console The console on which the string should be sent
 *  @param str The string to be sent
 *
 *  @return The number of characters which could be queued for transmission.
 */
extern uint16_t console_send_str_async (struct console_desc_t *console,
                                        const char *str);

/**
 *  Send a byte array on a console. This function will block until the whole
 *  array is copied in to the buffer even if the whole array does not initially
 *  fit in the available free space.
 *
 *  @param console The console on which the string should be sent
 *  @param data The array to be send
 *  @param length The number of bytes to be sent
 */
extern void console_send_bytes (struct console_desc_t *console,
                                const uint8_t *data, uint16_t length);

/**
 *  Send a string on a console. This function copies the array into the output
 *  buffer and then returns immediately. If there is not enough space, the whole
 *  array may not be copied. The number of bytes which could be copied to the
 *  buffer is returned.
 *
 *  @param console The console on which the string should be sent
 *  @param data The array to be send
 *  @param length The number of bytes to be sent
 *
 *  @return The number of bytes which could be queued for transmission.
 */
extern uint16_t console_send_bytes_async (struct console_desc_t *console,
                                          const uint8_t *data, uint16_t length);

/**
 *  Set the function which should be called when a full line has been received.
 *  The function will be passed the line as a string, a pointer to the console
 *  on which to the line was received and a pointer to an object which contains
 *  context information (if provided).
 *  If the the callback is not set or set to NULL then lines will be discarded
 *  when they as they are received.
 *
 *  @param console The console for which the callback should be set
 *  @param line_callback The function to be called when a complete line has been
 *                       received
 *  @param context A point which will be passed to the callback
 */
extern void console_set_line_callback (struct console_desc_t *console,
                                       void (*line_callback)(char*,
                                                        struct console_desc_t*,
                                                        void*), void *context);

/**
 *  Set the function which should be called when the console is ready to be
 *  initialized.
 *
 *  @param console The console for which the callback should be set
 *  @param init_callback The function to be called when the console is ready.
 *  @param context A point which will be passed to the callback
 */
extern void console_set_init_callback (struct console_desc_t *console,
                                       void (*init_callback)(
                                                        struct console_desc_t*,
                                                        void*), void *context);

/**
 *  Service to be run in each iteration of the main loop.
 *
 *  @param console The console for which the service should be run
 */
extern void console_service (struct console_desc_t *console);

#endif /* console_h */
