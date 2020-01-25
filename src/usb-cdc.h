/**
 * @file usb-cdc.h
 * @desc USB CDC interface
 * @author Samuel Dewan
 * @date 2019-11-17
 * Last Author: Samuel Dewan
 * Last Edited On: 2019-12-27
 */

#ifndef usb_cdc_h
#define usb_cdc_h

#include "global.h"
#include "config.h"

#include "usb-standard.h"

#define USB_CDC_NOTIFICATION_EP_SIZE    64
#define USB_CDC_DATA_EP_SIZE            64

#ifdef ENABLE_USB_CDC_PORT_0
#define USB_CDC_FIRST_INTERFACE_0 0
#define USB_CDC_NOTIFICATION_ENDPOINT_0 1
#define USB_CDC_DATA_IN_ENDPOINT_0  2
#define USB_CDC_DATA_OUT_ENDPOINT_0 2
#endif

#ifdef ENABLE_USB_CDC_PORT_1
#define USB_CDC_FIRST_INTERFACE_1 2
#define USB_CDC_NOTIFICATION_ENDPOINT_1 3
#define USB_CDC_DATA_IN_ENDPOINT_1  4
#define USB_CDC_DATA_OUT_ENDPOINT_1 4
#endif

#ifdef ENABLE_USB_CDC_PORT_2
#define USB_CDC_FIRST_INTERFACE_2 4
#define USB_CDC_NOTIFICATION_ENDPOINT_2 5
#define USB_CDC_DATA_IN_ENDPOINT_2  6
#define USB_CDC_DATA_OUT_ENDPOINT_2 6
#endif

#ifdef ENABLE_USB_CDC_PORT_0
#ifdef ENABLE_USB_CDC_PORT_1
#ifdef ENABLE_USB_CDC_PORT_2
#define USB_CDC_NUM_PORTS  3       // 0, 1, 2
#else
#define USB_CDC_NUM_PORTS  2       // 0, 1
#endif // ENABLE_USB_CDC_INTERFACE_2
#else
#ifdef ENABLE_USB_CDC_PORT_2
#define USB_CDC_NUM_PORTS  2       // 0, 2
#else
#define USB_CDC_NUM_PORTS  1       // 0,
#endif // ENABLE_USB_CDC_INTERFACE_2
#endif // ENABLE_USB_CDC_INTERFACE_1
#else
#ifdef ENABLE_USB_CDC_PORT_1
#ifdef ENABLE_USB_CDC_PORT_2
#define USB_CDC_NUM_PORTS  2       // 1, 2
#else
#define USB_CDC_NUM_PORTS  1       // 1
#endif // ENABLE_USB_CDC_INTERFACE_2
#else
#ifdef ENABLE_USB_CDC_PORT_2
#define USB_CDC_NUM_PORTS  1       // 2
#else
#define USB_CDC_NUM_PORTS  0       //
#endif // ENABLE_USB_CDC_INTERFACE_2
#endif // ENABLE_USB_CDC_INTERFACE_1
#endif // ENABLE_USB_CDC_INTERFACE_0

#ifdef ENABLE_USB_CDC_PORT_2
#define USB_CDC_HIGHEST_PORT 2
#else
#ifdef ENABLE_USB_CDC_PORT_1
#define USB_CDC_HIGHEST_PORT 1
#else
#define USB_CDC_HIGHEST_PORT 0
#endif // ENABLE_USB_CDC_INTERFACE_1
#endif // ENABLE_USB_CDC_INTERFACE_2


/** Configuration descriptor for CDC interface. */
extern const struct usb_cdc_configuration_descriptor usb_cdc_config_descriptor;

/**
 *  Callback for when CDC configuration is enabled by host.
 */
extern void usb_cdc_enable_config_callback (void);

/**
 *  Callback for when CDC configuration is disabled by host.
 */
extern void usb_cdc_disable_config_callback (void);

/**
 *  Callback to handle class specific requests.
 *
 *  @param packet The setup packet containing the request
 *  @param response_length Pointer to where length of response will be placed
 *  @param response_buffer Pointer to where response buffer will be placed
 *
 *  @return 0 if successful, a non-zero value otherwise
 */
extern uint8_t usb_cdc_class_request_callback (struct usb_setup_packet *packet,
                                               uint16_t *response_length,
                                               const uint8_t **response_buffer);


/**
 *  Configure a callback to be called when the USB serial interface is ready.
 *
 *  @param port The CDC port for which the callback should be configured
 *  @param callback The function to be called
 *  @param context Pointer to be provided to callback
 */
extern void usb_cdc_set_ready_callback (uint8_t port, void (*callback)(void*),
                                        void *context);

/**
 *  Queue a string to be written to a CDC interface.
 *
 *  @param port Index of the CDC interface
 *  @param str The string to be written.
 *
 *  @return The number of characters which could be queued for transmission.
 */
extern uint16_t usb_cdc_put_string(uint8_t port, const char *str);

/**
 *  Queue a string to be written to a CDC port. If there is not enough
 *  space for the string in the buffer, block until there is.
 *
 *  @param port Index of the CDC port
 *  @param str The string to be written.
 */
extern void usb_cdc_put_string_blocking(uint8_t port, const char *str);

/**
 *  Queue a byte array to be written to a CDC interface.
 *
 *  @param port Index of the CDC port
 *  @param bytes The array to be written
 *  @param length The number of bytes to be written
 *
 *  @return The number of bytes which could be added to the queue.
 */
extern uint16_t usb_cdc_put_bytes(uint8_t port, const uint8_t *bytes,
                                  uint16_t length);

/**
 *  Queue a string to be written to a CDC interface. If there is not enough
 *  space for the string in the buffer, block until there is.
 *
 *  @param port Index of the CDC port
 *  @param bytes The array to be written
 *  @param length The number of bytes to be written
 */
extern void usb_cdc_put_bytes_blocking(uint8_t port, const uint8_t *bytes,
                                        uint16_t length);

/**
 *  Write a character to a CDC port.
 *
 *  @param port Index of the CDC port
 *  @param c The character to be written
 */
extern void usb_cdc_put_char (uint8_t port, const char c);

/**
 *  Get string from a CDC port input buffer.
 *
 *  @param port Index of the CDC port
 *  @param str The string in which the data should be stored.
 *  @param len The maximum number of chars to be read from the buffer.
 */
extern void usb_cdc_get_string (uint8_t port, char *str, uint16_t len);

/**
 *  Determine if there is a full line, as delimited by the provided char,
 *  available to be read from a CDC port buffer.
 *
 *  @param port Index of the CDC port
 *  @param delim The delimiter for new lines (ie. '\n').
 *
 *  @return 0 if there is no line available, 1 if a line is available.
 */
extern uint8_t usb_cdc_has_delim (uint8_t port, char delim);

/**
 *  Read a string from the input buffer up to the next occurrence of a
 *  delimiter.
 *
 *  @param port Index of the CDC port
 *  @param delim The delimiter for new lines (ie. '\n').
 *  @param str The string in which the data should be stored.
 *  @param len The maximum number of chars to be read from the input buffer.
 */
extern void usb_cdc_get_line_delim (uint8_t port, char delim, char *str,
                                    uint16_t len);

/**
 *  Determine if there is a full line, delimited by the sequence "\r\n",
 *  available to be read from a CDC port buffer.
 *
 *  @param port Index of the CDC port
 *
 *  @return 0 if there is no line available, 1 if a line is available.
 */
extern uint8_t usb_cdc_has_line (uint8_t port);

/**
 *  Read a string from the input buffer up to the next occurrence of a
 *  the sequence "\r\n".
 *
 *  @param port Index of the CDC port
 *  @param str The string in which the data should be stored.
 *  @param len The maximum number of chars to be read from the input buffer.
 */
extern void usb_cdc_get_line (uint8_t port, char *str, uint16_t len);

/**
 *  Get a character from a CDC port input buffer.
 *
 *  @param port Index of the CDC port
 *
 *  @return The least recently received character in the input buffer
 */
extern char usb_cdc_get_char (uint8_t port);

/**
 *  Determine if the out buffer of a CDC port is empty.
 *
 *  @param port Index of the CDC port
 */
extern uint8_t usb_cdc_out_buffer_empty (uint8_t port);


#endif /* usb_cdc_h */
