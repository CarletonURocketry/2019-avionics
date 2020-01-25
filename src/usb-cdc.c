/**
 * @file usb-cdc.c
 * @desc USB CDC interface
 * @author Samuel Dewan
 * @date 2019-11-17
 * Last Author: Samuel Dewan
 * Last Edited On: 2019-11-30
 */

#include "usb-cdc.h"

#include "usb-cdc-standard.h"
#include "usb.h"

#include "circular-buffer.h"

#include <ctype.h>


// MARK: Static variables
/** Buffers for notification endpoints */
__attribute__((__aligned__(4)))
static uint8_t notification_buffers_g[USB_CDC_HIGHEST_PORT + 1][8];
/** Buffers for data out endpoints */
__attribute__((__aligned__(4)))
static uint8_t out_buffers_g[USB_CDC_HIGHEST_PORT + 1][USB_CDC_DATA_EP_SIZE];
/** Buffers for data in endpoints */
__attribute__((__aligned__(4)))
static uint8_t align_buffers_g[USB_CDC_HIGHEST_PORT + 1][4];

/** Global flags */
static struct {
    uint8_t initialized:3;
    uint8_t in_ongoing:3;
    uint8_t echo:3;
} usb_cdc_flags_g;

/** Lengths by which buffer heads need to be moved in in complete callbacks */
static uint16_t in_lengths[USB_CDC_HIGHEST_PORT + 1];

#define USB_CDC_CIRC_BUFF_SIZE  128
/** Receive circual buffers */
static struct circular_buffer_t rx_circ_buffs_g[USB_CDC_HIGHEST_PORT + 1];
/** Transmit circulat buffers */
static struct circular_buffer_t tx_circ_buffs_g[USB_CDC_HIGHEST_PORT + 1];

/** Buffers for received data */
static uint8_t rx_buffs_g[USB_CDC_HIGHEST_PORT + 1][USB_CDC_CIRC_BUFF_SIZE];
/** Buffers for data to be transmitted */
__attribute__((__aligned__(4)))
static uint8_t tx_buffs_g[USB_CDC_HIGHEST_PORT + 1][USB_CDC_CIRC_BUFF_SIZE];

/** Callback functions for when CDC interface becomes ready */
static void (*usb_cdc_ready_callbacks[USB_CDC_HIGHEST_PORT + 1]) (void *);
/** Context information to be provided to CDC ready callbacks */
static void *usb_cdc_callback_contexts[USB_CDC_HIGHEST_PORT + 1];


// MARK: Service function
/**
 *  Service function which starts a new USB in transaction if there is data to
 *  be send.
 *
 *  @param port The port for which the service should be run
 */
static void usb_cdc_service (uint8_t port)
{
    if (!(usb_cdc_flags_g.in_ongoing & (1 << port))) {
        /* We are not currently sending data */
        // Find the head of the tx buffer
        uint8_t *head;
        uint16_t len = circular_buffer_get_head(tx_circ_buffs_g + port, &head);
        if (!len) {
            // No data to be sent
            return;
        }
        /* Check alignment of head */
        if ((uintptr_t)head & 0x3) {
            /* Head is not 4 byte aligned */
            // Copy however many bytes we need to to align the head into our
            // aligned buffer
            uint8_t i = 0;
            uint8_t result = 0;
            do {
                result = circular_buffer_pop(tx_circ_buffs_g + port,
                                             align_buffers_g[port] + i);
                (uintptr_t)head++;
                i += !result;
            } while (!result && ((uintptr_t)head & 0x3) && (i < len));
            // Make head point to our align buffer so that we will transmit
            // from there instead of from the circular buffer
            head = align_buffers_g[port];
            // Update len to the number of bytes that need to be sent from the
            // align buffer
            len = i;
            // Since we popped the bytes from the circular buffer, the head has
            // already been moved, it doesn't need to be moved again when the
            // in is complete
            in_lengths[port] = 0;
        } else {
            /* Head is 4 byte aligned */
            // Record by how much the head of the circular buffer needs to be
            // moved in the in complete callback
            in_lengths[port] = len;
        }
        usb_cdc_flags_g.in_ongoing |= (1 << port);
        /* Start transmitting data from head */
#ifdef ENABLE_USB_CDC_PORT_0
        if (port == 0) {
            usb_start_in(USB_CDC_DATA_IN_ENDPOINT_0, head, len, 1);
        }
#endif
#ifdef ENABLE_USB_CDC_PORT_1
        else if (port == 1) {
            usb_start_in(USB_CDC_DATA_IN_ENDPOINT_1, head, len, 1);
        }
#endif
#ifdef ENABLE_USB_CDC_PORT_2
        else if (port == 2) {
            usb_start_in(USB_CDC_DATA_IN_ENDPOINT_2, head, len, 1);
        }
#endif
    }
}

// MARK: USB Callbacks

#ifdef ENABLE_USB_CDC_PORT_0
static void notification_0_out_complete (uint16_t length)
{
    // Ignore anything on the notification endpoint
    usb_start_out(USB_CDC_NOTIFICATION_ENDPOINT_0, notification_buffers_g[0],
                  8);
}
#endif

#ifdef ENABLE_USB_CDC_PORT_1
static void notification_1_out_complete (uint16_t length)
{
    // Ignore anything on the notification endpoint
    usb_start_out(USB_CDC_NOTIFICATION_ENDPOINT_0, notification_buffers_g[1],
                  8);
}
#endif

#ifdef ENABLE_USB_CDC_PORT_2
static void notification_2_out_complete (uint16_t length)
{
    // Ignore anything on the notification endpoint
    usb_start_out(USB_CDC_NOTIFICATION_ENDPOINT_0, notification_buffers_g[2],
                  8);
}
#endif

static void data_out_complete (uint8_t port, uint16_t length)
{
    // Copy data from out_buffers_g to rx_circ_buff_g and echo as required
    for (uint16_t i = 0; i < length; i++) {
        uint8_t data = out_buffers_g[port][i];
        uint8_t full = 0;
        
        if (usb_cdc_flags_g.echo & (1 << port)) {
            if (!iscntrl(data) || (data == '\r')) {
                // Should add byte to input buffer
                full = circular_buffer_try_push(rx_circ_buffs_g + port, data);
                
                if (!full && isprint(data)) {
                    // Echo
                    usb_cdc_put_char(port, (char)data);
                } else if (!full && (data == '\r')) {
                    // Echo newline
                    usb_cdc_put_char(port, '\n');
                }
            } else if (data == 127) {
                // Backspace
                uint8_t empty = circular_buffer_unpush(rx_circ_buffs_g + port);
                
                if (!empty) {
                    usb_cdc_put_string(port, "\x1B[1D\x1B[K");
                }
            }
        } else {
            // Add byte to input buffer, but do not echo
            circular_buffer_push(rx_circ_buffs_g + port, data);
        }
    }
    
#ifdef ENABLE_USB_CDC_PORT_0
    if (port == 0) {
        usb_start_out(USB_CDC_DATA_OUT_ENDPOINT_0, out_buffers_g[0],
                      USB_CDC_DATA_EP_SIZE);
    }
#endif
#ifdef ENABLE_USB_CDC_PORT_1
    else if (port == 1) {
        usb_start_out(USB_CDC_DATA_OUT_ENDPOINT_1, out_buffers_g[1],
                      USB_CDC_DATA_EP_SIZE);
    }
#endif
#ifdef ENABLE_USB_CDC_PORT_2
    else if (port == 2) {
        usb_start_out(USB_CDC_DATA_OUT_ENDPOINT_2, out_buffers_g[2],
                      USB_CDC_DATA_EP_SIZE);
    }
#endif
}

#ifdef ENABLE_USB_CDC_PORT_0
static void data_0_in_complete (void)
{
    if (in_lengths[0]) {
        circular_buffer_move_head(tx_circ_buffs_g + 0, in_lengths[0]);
        in_lengths[0] = 0;
    }
    usb_cdc_flags_g.in_ongoing &= ~(1 << 0);
    usb_cdc_service(0);
}

static void data_0_out_complete (uint16_t length)
{
    data_out_complete(0, length);
}
#endif

#ifdef ENABLE_USB_CDC_PORT_1
static void data_1_in_complete (void)
{
    if (in_lengths[1]) {
        circular_buffer_move_head(tx_circ_buffs_g + 1, in_lengths[1]);
        in_lengths[1] = 0;
    }
    usb_cdc_flags_g.in_ongoing &= ~(1 << 1);
    usb_cdc_service(1);
}

static void data_1_out_complete (uint16_t length)
{
    data_out_complete(1, length);
}
#endif

#ifdef ENABLE_USB_CDC_PORT_2
static void data_2_in_complete (void)
{
    if (in_lengths[2]) {
        circular_buffer_move_head(tx_circ_buffs_g + 2, in_lengths[2]);
        in_lengths[2] = 0;
    }
    usb_cdc_flags_g.in_ongoing &= ~(1 << 2);
    usb_cdc_service(2);
}

static void data_2_out_complete (uint16_t length)
{
    data_out_complete(2, length);
}
#endif


void usb_cdc_enable_config_callback (void)
{
#ifdef ENABLE_USB_CDC_PORT_0
    /* Set echo flag */
#ifdef USB_CDC_PORT_0_ECHO
    usb_cdc_flags_g.echo |= (1 << 0);
#endif
    /* Initialize circular buffers */
    init_circular_buffer(rx_circ_buffs_g + 0, rx_buffs_g[0],
                         USB_CDC_CIRC_BUFF_SIZE);
    init_circular_buffer(tx_circ_buffs_g + 0, tx_buffs_g[0],
                         USB_CDC_CIRC_BUFF_SIZE);
    /* Enable endpoints for CDC ACM interface 0 */
    usb_enable_endpoint_out(USB_CDC_NOTIFICATION_ENDPOINT_0,
                            USB_CDC_NOTIFICATION_EP_SIZE,
                            USB_ENDPOINT_TYPE_INTERRUPT,
                            &notification_0_out_complete);
    usb_enable_endpoint_in(USB_CDC_DATA_IN_ENDPOINT_0, USB_CDC_DATA_EP_SIZE,
                           USB_ENDPOINT_TYPE_INTERRUPT, &data_0_in_complete);
    usb_enable_endpoint_out(USB_CDC_DATA_OUT_ENDPOINT_0, USB_CDC_DATA_EP_SIZE,
                            USB_ENDPOINT_TYPE_INTERRUPT, &data_0_out_complete);
    /* Start endpoints for interface 0 */
    usb_start_out(USB_CDC_NOTIFICATION_ENDPOINT_0, notification_buffers_g[0],
                  USB_CDC_NOTIFICATION_EP_SIZE);
    usb_start_out(USB_CDC_DATA_OUT_ENDPOINT_0, out_buffers_g[0],
                  USB_CDC_DATA_EP_SIZE);
    /* Mark port as initialized */
    usb_cdc_flags_g.initialized |= (1 << 0);
    /* Call ready callback for interface 0 */
    if (usb_cdc_ready_callbacks[0] != NULL) {
        usb_cdc_ready_callbacks[0](usb_cdc_callback_contexts[0]);
    }
#endif
#ifdef ENABLE_USB_CDC_PORT_1
    /* Set echo flag */
#ifdef USB_CDC_PORT_1_ECHO
    usb_cdc_flags_g.echo |= (1 << 1);
#endif
    /* Initialize circular buffers */
    init_circular_buffer(rx_circ_buffs_g + 1, rx_buffs_g[1],
                         USB_CDC_CIRC_BUFF_SIZE);
    init_circular_buffer(tx_circ_buffs_g + 1, tx_buffs_g[1],
                         USB_CDC_CIRC_BUFF_SIZE);
    /* Enable endpoints for CDC ACM interface 1 */
    usb_enable_endpoint_out(USB_CDC_NOTIFICATION_ENDPOINT_1, 8,
                            USB_ENDPOINT_TYPE_INTERRUPT,
                            &notification_1_out_complete);
    usb_enable_endpoint_in(USB_CDC_DATA_IN_ENDPOINT_1, USB_CDC_DATA_EP_SIZE,
                           USB_ENDPOINT_TYPE_INTERRUPT, &data_1_in_complete);
    usb_enable_endpoint_out(USB_CDC_DATA_OUT_ENDPOINT_1, USB_CDC_DATA_EP_SIZE,
                            USB_ENDPOINT_TYPE_INTERRUPT, &data_1_out_complete);
    /* Start endpoints for interface 1 */
    usb_start_out(USB_CDC_NOTIFICATION_ENDPOINT_1, notification_buffers_g[1],
                  8);
    usb_start_out(USB_CDC_DATA_OUT_ENDPOINT_1, out_buffers_g[1],
                  USB_CDC_DATA_EP_SIZE);
    /* Mark port as initialized */
    usb_cdc_flags_g.initialized |= (1 << 1);
    /* Call ready callback for interface 1 */
    if (usb_cdc_ready_callbacks[1] != NULL) {
        usb_cdc_ready_callbacks[1](usb_cdc_callback_contexts[1]);
    }
#endif
#ifdef ENABLE_USB_CDC_PORT_2
    /* Set echo flag */
#ifdef USB_CDC_PORT_2_ECHO
    usb_cdc_flags_g.echo |= (1 << 2);
#endif
    /* Initialize circular buffers */
    init_circular_buffer(rx_circ_buffs_g + 2, rx_buffs_g[2],
                         USB_CDC_CIRC_BUFF_SIZE);
    init_circular_buffer(tx_circ_buffs_g + 2, tx_buffs_g[2],
                         USB_CDC_CIRC_BUFF_SIZE);
    /* Enable endpoints for CDC ACM interface 2 */
    usb_enable_endpoint_out(USB_CDC_NOTIFICATION_ENDPOINT_2, 8,
                            USB_ENDPOINT_TYPE_INTERRUPT,
                            &notification_2_out_complete);
    usb_enable_endpoint_in(USB_CDC_DATA_IN_ENDPOINT_2, USB_CDC_DATA_EP_SIZE,
                           USB_ENDPOINT_TYPE_INTERRUPT, &data_2_in_complete);
    usb_enable_endpoint_out(USB_CDC_DATA_OUT_ENDPOINT_2, USB_CDC_DATA_EP_SIZE,
                            USB_ENDPOINT_TYPE_INTERRUPT, &data_2_out_complete);
    /* Start endpoints for interface 2 */
    usb_start_out(USB_CDC_NOTIFICATION_ENDPOINT_2, notification_buffers_g[2],
                  8);
    usb_start_out(USB_CDC_DATA_OUT_ENDPOINT_2, out_buffers_g[2],
                  USB_CDC_DATA_EP_SIZE);
    /* Mark port as initialized */
    usb_cdc_flags_g.initialized |= (1 << 2);
    /* Call ready callback for interface 2 */
    if (usb_cdc_ready_callbacks[2] != NULL) {
        usb_cdc_ready_callbacks[2](usb_cdc_callback_contexts[2]);
    }
#endif
}

void usb_cdc_disable_config_callback (void)
{
#ifdef ENABLE_USB_CDC_PORT_0
    usb_disable_endpoint_out(USB_CDC_NOTIFICATION_ENDPOINT_0);
    usb_disable_endpoint_in(USB_CDC_DATA_IN_ENDPOINT_0);
    usb_disable_endpoint_out(USB_CDC_DATA_OUT_ENDPOINT_0);
#endif
#ifdef ENABLE_USB_CDC_PORT_1
    usb_disable_endpoint_out(USB_CDC_NOTIFICATION_ENDPOINT_1);
    usb_disable_endpoint_in(USB_CDC_DATA_IN_ENDPOINT_1);
    usb_disable_endpoint_out(USB_CDC_DATA_OUT_ENDPOINT_1);
#endif
#ifdef ENABLE_USB_CDC_PORT_2
    usb_disable_endpoint_out(USB_CDC_NOTIFICATION_ENDPOINT_2);
    usb_disable_endpoint_in(USB_CDC_DATA_IN_ENDPOINT_2);
    usb_disable_endpoint_out(USB_CDC_DATA_OUT_ENDPOINT_2);
#endif
    usb_cdc_flags_g.in_ongoing = 0;
    usb_cdc_flags_g.initialized = 0;
}

uint8_t usb_cdc_class_request_callback (struct usb_setup_packet *packet,
                                        uint16_t *response_length,
                                        const uint8_t **response_buffer)
{
    switch ((enum usb_cdc_request)packet->bRequest) {
        case USB_CDC_REQ_SET_LINE_CODING:
            *response_length = 0;
            break;
        case USB_CDC_REQ_GET_LINE_CODING:
            // Not supported
            return 1;
        case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
            *response_length = 0;
            break;
        case USB_CDC_REQ_SEND_BREAK:
            *response_length = 0;
            break;
        default:
            // Unknown request, Request Error
            return 1;
    }
    
    return 0;
}


// MARK: External functions

void usb_cdc_set_ready_callback (uint8_t port, void (*callback)(void*),
                                 void *context)
{
    usb_cdc_ready_callbacks[port] = callback;
    usb_cdc_callback_contexts[port] = context;
    
    if (usb_cdc_flags_g.initialized & (1 << port)) {
        if (usb_cdc_ready_callbacks[port] != NULL) {
            usb_cdc_ready_callbacks[port](usb_cdc_callback_contexts[port]);
        }
    }
}

uint16_t usb_cdc_put_string(uint8_t port, const char *str)
{
    uint16_t i = 0;
    for (; str[i] != '\0'; i++) {
        // Make sure that we have enough space for the next character, or two
        // characters if the next character is a newline since we need to insert
        // a carriage return as well.
        uint16_t unused = circular_buffer_unused(tx_circ_buffs_g + port);
        if ((unused < 1) || ((str[i] == '\n') && (unused < 2))) {
            break;
        }
        
        circular_buffer_push(tx_circ_buffs_g + port, (uint8_t)str[i]);
        
        if (str[i] == '\n') {
            // Add carriage return as some terminal emulators seem to think that
            // they are typewriters.
            circular_buffer_push(tx_circ_buffs_g + port, (uint8_t)'\r');
        }
    }
    
    // Make sure that we start transmission right away if there is no
    // transmission already in progress.
    usb_cdc_service(port);
    
    return i;
}

void usb_cdc_put_string_blocking(uint8_t port, const char *str)
{
    uint8_t carriage_return = 0;
    
    for (const char *i = str; *i != '\0';) {
        // Wait for a character worth of space to become avaliable in the buffer
        while (circular_buffer_is_full(tx_circ_buffs_g + port)) {
            // Make sure that we aren't waiting for a transaction which is not
            // in progress.
            usb_cdc_service(port);
        }
        
        if (carriage_return) {
            // Push a carriage return
            circular_buffer_push(tx_circ_buffs_g + port, (uint8_t)'\r');
        } else {
            // Push the next character
            circular_buffer_push(tx_circ_buffs_g + port, (uint8_t)*i);
        }
        
        if (*i == '\n' && !carriage_return) {
            // Add carriage return after newlines
            carriage_return = 1;
        } else {
            i++;
            carriage_return = 0;
        }
    }
    
    // Make sure that we start transmission right away if there is no
    // transmission already in progress.
    usb_cdc_service(port);
}

uint16_t usb_cdc_put_bytes(uint8_t port, const uint8_t *bytes, uint16_t length)
{
    uint16_t i = 0;
    for (; i < length; i++) {
        if (circular_buffer_is_full(tx_circ_buffs_g + port)) {
            break;
        }
        
        circular_buffer_push(tx_circ_buffs_g + port, (uint8_t)bytes[i]);
    }
    
    // Make sure that we start transmission right away if there is no
    // transmission already in progress.
    usb_cdc_service(port);
    
    return i;
}

void usb_cdc_put_bytes_blocking(uint8_t port, const uint8_t *bytes,
                                uint16_t length)
{
    for (uint16_t i = 0; i < length; i++) {
        // Wait for a character worth of space to become avaliable in the buffer
        while (circular_buffer_is_full(tx_circ_buffs_g + port)) {
            // Make sure that we aren't waiting for a transaction which is not
            // in progress.
            usb_cdc_service(port);
        }
        
        circular_buffer_push(tx_circ_buffs_g + port, bytes[i]);
    }
    
    // Make sure that we start transmission right away if there is no
    // transmission already in progress.
    usb_cdc_service(port);
}

void usb_cdc_put_char (uint8_t port, const char c)
{
    circular_buffer_push(tx_circ_buffs_g + port, (uint8_t)c);
    
    if (c == '\n') {
        // Add carriage return as some terminal emulators seem to think that
        // they are typewriters.
        circular_buffer_push(tx_circ_buffs_g + port, (uint8_t)'\r');
    }
    
    // Make sure that we start transmition right away if there is no transmition
    // already in progress.
    usb_cdc_service(port);
}

void usb_cdc_get_string (uint8_t port, char *str, uint16_t len)
{
    for (uint16_t i = 0; i < (len - 1); i++) {
        uint8_t pop_failed = circular_buffer_pop(rx_circ_buffs_g + port,
                                                 (uint8_t*)(str + i));
        
        if (pop_failed) {
            str[i] = '\0';
            return;
        }
    }
    // Make sure that string is terminated.
    str[len - 1] = '\0';
}

uint8_t usb_cdc_has_delim (uint8_t port, char delim)
{
    return ((usb_cdc_flags_g.initialized & (1 << port)) &&
            circular_buffer_has_char(rx_circ_buffs_g + port, delim));
}

void usb_cdc_get_line_delim (uint8_t port, char delim, char *str, uint16_t len)
{
    for (uint16_t i = 0; i < (len - 1); i++) {
        uint8_t pop_failed = circular_buffer_pop(rx_circ_buffs_g + port,
                                                 (uint8_t*)(str + i));
        
        if (pop_failed || str[i] == delim) {
            str[i] = '\0';
            return;
        }
    }
    // Make sure that string is terminated.
    str[len - 1] = '\0';
}

uint8_t usb_cdc_has_line (uint8_t port)
{
    return ((usb_cdc_flags_g.initialized & (1 << port)) &&
            circular_buffer_has_line(rx_circ_buffs_g + port));
}

void usb_cdc_get_line (uint8_t port, char *str, uint16_t len)
{
    uint8_t last_char_cr = 0;
    for (uint16_t i = 0; i < (len - 1); i++) {
        uint8_t pop_failed = circular_buffer_pop(rx_circ_buffs_g + port,
                                                 (uint8_t*)(str + i));
        
        if (pop_failed) {
            str[i] = '\0';
            return;
        } else if (last_char_cr && str[i] == '\n') {
            str[i - 1] = '\0';
            return;
        }
        
        last_char_cr = str[i] == '\r';
    }
    
    // We ran out of space in the buffer to pop the next character, we might
    // have just popped a carriage return, and the next character might be a
    // newline, in which case we can pop the newline even though the buffer is
    // full since we don't need to put it in our buffer
    uint8_t c;
    if (last_char_cr && !circular_buffer_peak(rx_circ_buffs_g + port, &c)) {
        if (c == '\n') {
            circular_buffer_pop(rx_circ_buffs_g + port, &c);
        }
    }
    
    // Make sure that string is terminated.
    str[len - 1] = '\0';
}

char usb_cdc_get_char (uint8_t port)
{
    char c = '\0';
    circular_buffer_pop(rx_circ_buffs_g + port, (uint8_t*)&c);
    return c;
}

uint8_t usb_cdc_out_buffer_empty (uint8_t port)
{
    return circular_buffer_is_empty(tx_circ_buffs_g + port);
}
