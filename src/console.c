/**
 * @file console.c
 * @desc Abstracts over sercom-uart or usb-serial console.
 * @author Samuel Dewan
 * @date 2019-09-20
 * Last Author: Samuel Dewan
 * Last Edited On: 2019-11-30
 */

#include "console.h"

#define CONSOLE_IN_BUFFER_LEN 256

#ifdef ID_USB
#include "usb-cdc.h"
#endif

#ifdef ID_USB
static void console_usb_serial_ready_cb (void *context)
{
    struct console_desc_t *console = (struct console_desc_t *)context;
    
    if (console->init_callback) {
        console->init_callback(console, console->callback_context);
    }
}
#endif

void init_uart_console (struct console_desc_t *console,
                        struct sercom_uart_desc_t *uart, char line_delim)
{
    console->type = CONSOLE_TYPE_UART;
    console->interface.uart = uart;
    console->line_callback = NULL;
    console->callback_context = NULL;
    console->line_delimiter = line_delim;
}

#ifdef ID_USB
void init_usb_cdc_console (struct console_desc_t *console,
                           uint8_t usb_cdc_interface, const char line_delim)
{
    console->type = CONSOLE_TYPE_USB_CDC;
    console->interface.usb_cdc = usb_cdc_interface;
    console->line_callback = NULL;
    console->callback_context = NULL;
    console->line_delimiter = line_delim;
}
#endif

void console_send_str (struct console_desc_t *console, const char *str)
{
    if (console->type == CONSOLE_TYPE_UART) {
        // SERCOM UART
        sercom_uart_put_string_blocking(console->interface.uart, str);
    }
#ifdef ID_USB
    else if (console->type == CONSOLE_TYPE_USB_CDC) {
        // USB
        usb_cdc_put_string_blocking(console->interface.usb_cdc, str);
    }
#endif
}

uint16_t console_send_str_async (struct console_desc_t *console,
                                 const char *str)
{
    if (console->type == CONSOLE_TYPE_UART) {
        // SERCOM UART
        return sercom_uart_put_string(console->interface.uart, str);
    }
#ifdef ID_USB
    else if (console->type == CONSOLE_TYPE_USB_CDC) {
        // USB
        return usb_cdc_put_string(console->interface.usb_cdc, str);
    }
#endif
    return 0;
}

void console_send_bytes (struct console_desc_t *console,
                         const uint8_t *data, uint16_t length)
{
    if (console->type == CONSOLE_TYPE_UART) {
        // SERCOM UART
        sercom_uart_put_bytes_blocking(console->interface.uart, data, length);
    }
#ifdef ID_USB
    else if (console->type == CONSOLE_TYPE_USB_CDC) {
        // USB
        usb_cdc_put_bytes_blocking(console->interface.usb_cdc, data, length);
    }
#endif
}

uint16_t console_send_bytes_async (struct console_desc_t *console,
                                          const uint8_t *data, uint16_t length)
{
    if (console->type == CONSOLE_TYPE_UART) {
        // SERCOM UART
        return sercom_uart_put_bytes(console->interface.uart, data, length);
    }
#ifdef ID_USB
    else if (console->type == CONSOLE_TYPE_USB_CDC) {
        // USB
        return usb_cdc_put_bytes(console->interface.usb_cdc, data, length);
    }
#endif
    return 0;
}

void console_set_line_callback (struct console_desc_t *console,
                                void (*line_callback)(char*,
                                                      struct console_desc_t*,
                                                      void*), void *context)
{
    console->line_callback = line_callback;
    console->callback_context = context;
}

void console_set_init_callback (struct console_desc_t *console,
                                void (*init_callback)(struct console_desc_t*,
                                                      void*), void *context)
{
    console->init_callback = init_callback;
    console->callback_context = context;
    
    if (console->type == CONSOLE_TYPE_UART) {
        // If this is a SERCOM UART based console it is always ready
        console->init_callback(console, console->callback_context);
    }
#ifdef ID_USB
    else if (console->type == CONSOLE_TYPE_USB_CDC) {
        // If this is a USB CDC based console then we need to use the USB
        // CDC ready callback.
        usb_cdc_set_ready_callback(console->interface.usb_cdc,
                                   console_usb_serial_ready_cb, (void*)console);
    }
#endif
}

void console_service (struct console_desc_t *console)
{
    static char console_in_buffer[CONSOLE_IN_BUFFER_LEN];
    
    uint8_t got_line = 0;
    
    if (console->type == CONSOLE_TYPE_UART) {
        // SERCOM UART
        if ((console->line_delimiter != '\0') && sercom_uart_has_delim(
                                                    console->interface.uart,
                                                    console->line_delimiter)) {
            sercom_uart_get_line_delim(console->interface.uart,
                                       console->line_delimiter,
                                       console_in_buffer,
                                       CONSOLE_IN_BUFFER_LEN);
            got_line = 1;
        } else if ((console->line_delimiter == '\0') && sercom_uart_has_line(
                                                    console->interface.uart)) {
            sercom_uart_get_line(console->interface.uart, console_in_buffer,
                                 CONSOLE_IN_BUFFER_LEN);
            got_line = 1;
        }
    }
#ifdef ID_USB
    else if (console->type == CONSOLE_TYPE_USB_CDC) {
        // USB
        if ((console->line_delimiter != '\0') && usb_cdc_has_delim(
                                                console->interface.usb_cdc,
                                                console->line_delimiter)) {
            usb_cdc_get_line_delim(console->interface.usb_cdc,
                                   console->line_delimiter, console_in_buffer,
                                   CONSOLE_IN_BUFFER_LEN);
            got_line = 1;
        } else if ((console->line_delimiter == '\0') && usb_cdc_has_line(
                                                console->interface.usb_cdc)) {
            usb_cdc_get_line(console->interface.usb_cdc, console_in_buffer,
                             CONSOLE_IN_BUFFER_LEN);
            got_line = 1;
        }
    }
#endif
    
    if (got_line && console->line_callback) {
        console->line_callback(console_in_buffer, console,
                               console->callback_context);
    }
}
