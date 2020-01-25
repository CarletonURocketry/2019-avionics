/**
 * @file usb.h
 * @desc USB driver
 * @author Samuel Dewan
 * @date 2019-11-09
 * Last Author: Samuel Dewan
 * Last Edited On: 2019-12-27
 */

#ifndef usb_h
#define usb_h

#include "global.h"

#include "usb-standard.h"


enum usb_speed {
    USB_SPEED_FULL = USB_DEVICE_CTRLB_SPDCONF_FS_Val,
    USB_SPEED_LOW = USB_DEVICE_CTRLB_SPDCONF_LS_Val
};

enum usb_endpoint_type {
    USB_ENDPOINT_TYPE_DISABLED = 0x0,
    USB_ENDPOINT_TYPE_CONTROL = 0x1,
    USB_ENDPOINT_TYPE_ISOCHRONOUS = 0x2,
    USB_ENDPOINT_TYPE_BULK = 0x3,
    USB_ENDPOINT_TYPE_INTERRUPT = 0x4,
    USB_ENDPOINT_TYPE_DUAL_BANK = 0x5,
};

enum usb_endpoint_stall_dir {
    USB_ENDPOINT_STALL_OUT = 0b01,
    USB_ENDPOINT_STALL_IN = 0b10,
    USB_ENDPOINT_STALL_BOTH = 0b11
};



extern void init_usb(uint32_t clock_mask, enum usb_speed speed,
                     void (*enable_config_cb)(void),
                     void (*disable_config_cb)(void),
                     uint8_t (*usb_class_request_cb)(struct usb_setup_packet *,
                                                     uint16_t *,
                                                     const uint8_t **),
                     const struct usb_configuration_descriptor *config_desc);

/**
 *  Attach the device to the USB.
 */
static inline void usb_attach (void)
{
    USB->DEVICE.CTRLB.bit.DETACH = 0b0;
}

/**
 *  Detach the device from the USB.
 */
static inline void usb_detach (void)
{
    USB->DEVICE.CTRLB.bit.DETACH = 0b1;
}


extern void usb_enable_endpoint_in (uint8_t ep, enum usb_endpoint_size size,
                                    enum usb_endpoint_type type,
                                    void (*callback)(void));

extern void usb_enable_endpoint_out (uint8_t ep, enum usb_endpoint_size size,
                                     enum usb_endpoint_type type,
                                     void (*callback)(uint16_t length));

extern void usb_disable_endpoint_in (uint8_t ep);

extern void usb_disable_endpoint_out (uint8_t ep);

/**
 *  Start a transaction from host to device.
 *
 *  @note The data buffer used with this function must be 4 byte aligned.
 *
 *  @param ep The endpoint on which the out transaction should be started
 *  @param data Pointer to buffer for received data
 *  @param length Length of data to be received
 */
extern void usb_start_out (uint8_t ep, uint8_t *data, uint16_t length);

/**
 *  Start a transaction from device to host.
 *
 *  @note The data buffer used with this function must be 4 byte aligned.
 *
 *  @param ep The endpoint on which the out transaction should be started
 *  @param data Pointer to the data to be sent
 *  @param length Length of data to be received
 *  @param zlp Perform automatic Zero Length Packet handshake
 */
extern void usb_start_in (uint8_t ep, const uint8_t *data, uint16_t length,
                          uint8_t zlp);

/**
 *  Stall an endpoint.
 *
 *  @param ep The endpoint to be stalled
 *  @param dir The direction in which the endpoint should be stalled
 */
static inline void usb_stall (uint8_t ep, enum usb_endpoint_stall_dir dir)
{
    // Stall both banks of endpoint
    USB->DEVICE.DeviceEndpoint[ep].EPSTATUSSET.reg =
                                            USB_DEVICE_EPSTATUS_STALLRQ(dir);
}


#endif /* usb_h */
