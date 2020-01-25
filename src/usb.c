/**
 * @file usb.c
 * @desc USB driver
 * @author Samuel Dewan
 * @date 2019-11-09
 * Last Author: Samuel Dewan
 * Last Edited On: 2019-12-27
 */

// Much of this code is based on Kevin Mehall's USB stack
// https://github.com/kevinmehall/usb

#include "usb.h"

#include "config.h"

#include <string.h> // for memcpy

// MARK: Static variables

/** Endpoint configurations */
static UsbDeviceDescriptor usb_ep_descriptors_g[USB_EPT_NUM];

#define USB_EP_0_LENGTH USB_ENDPOINT_SIZE_8
#define USB_EP_0_IN_BUF_SIZE 80
/** Buffer for data to be sent on endpoint 0 */
static uint8_t usb_ep_0_in_buf_g[USB_EP_0_IN_BUF_SIZE]
                                            __attribute__((__aligned__(4)));
/** Buffer for data received on endpoint 0 */
static uint8_t usb_ep_0_out_buf_g[USB_EP_0_LENGTH]
                                            __attribute__((__aligned__(4)));

/** Transmit complete callback functions */
static void (*usb_in_callbacks_g[8])(void);
/** Receive complete callback functions */
static void (*usb_out_callbacks_g[8])(uint16_t length);
/** Enable configuration callback */
static void (*usb_enable_config_g)(void);
/** Disable configuration callback */
static void (*usb_disable_config_g)(void);
/** Class request callback */
static uint8_t (*usb_class_request_g)(struct usb_setup_packet *, uint16_t *,
                                      const uint8_t **);

/** Number of bytes remaining in current endpoint 0 in transaction */
static uint16_t usb_ep_0_in_size;
/** Pointer to data for current endpoint 0 in transaction */
static const uint8_t *usb_ep_0_in_p;

/** Currently selected configuration number */
static uint8_t usb_current_config_g;

/** Pointer to configuration descriptor */
static const struct usb_configuration_descriptor *usb_config_desc_g;

// MARK: Descriptors

/** USB device descriptor */
__attribute__((__aligned__(4))) static const struct usb_device_descriptor
                                    usb_device_descriptor_g = {
    .bLength = sizeof(struct usb_device_descriptor),
    .bDescriptorType = USB_DESC_TYPE_DEVICE,
    
    .bcdUSB                 = 0x0200,
    // Special device class, subclass and protocol to indicate that the device
    // has Interface Association Descriptors
    .bDeviceClass           = 0xEF,
    .bDeviceSubClass        = 0x02,
    .bDeviceProtocol        = 0x01,
    
    .bMaxPacketSize0        = USB_EP_0_LENGTH,
    // This is Atmel's VID, with a PID set aside for use in a LUFA demo project.
    // Sorry Atmel.
    // We should consider trying to find a better VID/PID pair.
    .idVendor               = 0x03EB,
    .idProduct              = 0x2044,
    .bcdDevice              = 0x0100,
    
    .iManufacturer          = 0x01,
    .iProduct               = 0x02,
    .iSerialNumber          = 0x00,//0x03,
    
    .bNumConfigurations     = 1
};

/** String descriptor which lists supported languages */
static const struct usb_string_descriptor_zero usb_string_0_g = {
    .bLength = sizeof(struct usb_string_descriptor_zero) + 2,
    .bDescriptorType = USB_DESC_TYPE_STRING,
    .wLangid = { USB_LANGUAGE_EN_US }
};

/** String description of manufacturer */
static const struct usb_string_descriptor usb_string_manufacturer = {
    .bLength = (sizeof(struct usb_string_descriptor) +
                sizeof(USB_MANUFACTURER_STRING)),
    .bDescriptorType = USB_DESC_TYPE_STRING,
    .bString = USB_MANUFACTURER_STRING
};

/** String description of product */
static const struct usb_string_descriptor usb_string_product = {
    .bLength = (sizeof(struct usb_string_descriptor) +
                sizeof(USB_PRODUCT_STRING)),
    .bDescriptorType = USB_DESC_TYPE_STRING,
    .bString = USB_PRODUCT_STRING
};

/** Number of string descriptors */
static const uint8_t usb_num_strings_g = 2;
/** Pointer to string descriptors */
static const struct usb_string_descriptor *const usb_string_descs_g[] = {
    &usb_string_manufacturer,
    &usb_string_product
};

// MARK: Endpoint 0 Functions
static void handle_ep_0_multi_packet_in (void)
{
    /* We have data to send in a multi-packet in transaction */
    // We can send up to USB_EP_0_LENGTH bytes at a time
    uint16_t size = ((usb_ep_0_in_size > USB_EP_0_IN_BUF_SIZE) ?
                                    USB_EP_0_IN_BUF_SIZE : usb_ep_0_in_size);
    // Copy the bytes we are going to send into our buffer. Copying, rather
    // than sending the data in place, allows the data to be located in
    // non-volatile memory since the USBs DMA hardware can only access RAM.
    // It also means that the data does not need to be aligned (though
    // the memcpy will be faster if it is).
    memcpy(usb_ep_0_in_buf_g, usb_ep_0_in_p, size);
    // Determine how many bytes are left after this packet
    usb_ep_0_in_size -= size;
    // Send the data, auto ZLP if this is the last packet
    usb_start_in(0, usb_ep_0_in_buf_g, size, (usb_ep_0_in_size == 0));
    // If this was the last block, clean up
    if (usb_ep_0_in_size == 0) {
        // Set pointer to NULL to indicate that all blocks have been sent
        usb_ep_0_in_p = NULL;
        // We can start our next out.
        //usb_start_out(0, usb_ep_0_out_buf_g, USB_EP_0_LENGTH);
    } else {
        // Update pointer for next block
        usb_ep_0_in_p += size;
    }
}

/**
 *  Callback for when an in transaction is completed on endpoint 0.
 */
static void endpoint_0_in_complete (void)
{
    // If there is an address to be enabled, enable it now.
    // This is done in the in complete callback because we need to reply to the
    // SetAddress() request before actually setting the address
    if (USB->DEVICE.DADD.reg && !USB->DEVICE.DADD.bit.ADDEN) {
        USB->DEVICE.DADD.bit.ADDEN = 0b1;
        return;
    }
    
    // Continue any ongoing in transaction
    if ((usb_ep_0_in_p != NULL) && usb_ep_0_in_size) {
        handle_ep_0_multi_packet_in();
    }
}

/**
 *  Configure both banks of endpoint 0 and start receiving data.
 */
static void config_endpoint_zero(void)
{
    /* Configure in bank */
    usb_enable_endpoint_in(0, USB_EP_0_LENGTH, USB_ENDPOINT_TYPE_CONTROL,
                           &endpoint_0_in_complete);
    /* Configure out bank */
    usb_enable_endpoint_out(0, USB_EP_0_LENGTH, USB_ENDPOINT_TYPE_CONTROL,
                            NULL);
    
    /* Enable endpoint 0 received setup interrupt */
    USB->DEVICE.DeviceEndpoint[0].EPINTENSET.reg = USB_DEVICE_EPINTENSET_RXSTP;
    
    /* Start out on endpoint 0 */
    usb_start_out(0, usb_ep_0_out_buf_g, USB_EP_0_LENGTH);
}

// MARK: Externally Visible Functions
void init_usb(uint32_t clock_mask, enum usb_speed speed,
              void (*enable_config_cb)(void), void (*disable_config_cb)(void),
              uint8_t (*usb_class_request_cb)(struct usb_setup_packet *,
                                              uint16_t *, const uint8_t **),
              const struct usb_configuration_descriptor *config_desc)
{
    /* Store configuration callbacks */
    usb_enable_config_g = enable_config_cb;
    usb_disable_config_g = disable_config_cb;
    usb_class_request_g = usb_class_request_cb;
    /* Store descriptor pointers */
    usb_config_desc_g = config_desc;
    
    /* Enable the APBB clock for the USB */
    PM->APBAMASK.reg |= PM_AHBMASK_USB;
    
    /* Select the clock for the USB */
    GCLK->CLKCTRL.reg = (GCLK_CLKCTRL_CLKEN | clock_mask | GCLK_CLKCTRL_ID_USB);
    // Wait for synchronization
    while (GCLK->STATUS.bit.SYNCBUSY);
    
    /* Reset USB */
    USB->DEVICE.CTRLA.bit.SWRST = 0b1;
    // Wait for reset to complete
    while (USB->DEVICE.SYNCBUSY.bit.SWRST);
    
    /* Load pad calibration values from NVM Software Calibration Area */
    // Trimmable Output Driver Impedance N
    uint32_t transn = ((*((uint32_t*) USB_FUSES_TRANSN_ADDR) &
                        USB_FUSES_TRANSN_Msk) >> USB_FUSES_TRANSN_Pos);
    // Trimmable Output Driver Impedance P
    uint32_t transp = ((*((uint32_t*) USB_FUSES_TRANSP_ADDR) &
                        USB_FUSES_TRANSP_Msk) >> USB_FUSES_TRANSP_Pos);
    // Trim bits for DP/DM
    uint32_t trim = ((*((uint32_t*) USB_FUSES_TRIM_ADDR) &
                        USB_FUSES_TRIM_Msk) >> USB_FUSES_TRIM_Pos);
    USB->DEVICE.PADCAL.reg = (USB_PADCAL_TRANSN(transn) |
                              USB_PADCAL_TRANSP(transp) |
                              USB_PADCAL_TRIM(trim));
    
    /* Configure the USB for device mode */
    USB->DEVICE.CTRLA.reg = USB_CTRLA_MODE_DEVICE;
    
    /* Configure USB speed */
    USB->DEVICE.CTRLB.bit.SPDCONF = speed & 1;
    
    /* Enable interrupts from the USB */
    NVIC_EnableIRQ(USB_IRQn);
    
    /* Enable interrupt on end of reset condition */
    USB->DEVICE.INTENSET.reg = USB_DEVICE_INTENSET_EORST;
    
    /* Set descriptor base address */
    USB->DEVICE.DESCADD.reg = (uint32_t)&usb_ep_descriptors_g;
    
    /* Setup endpoint 0 */
    config_endpoint_zero();
    
    /* Enable USB */
    USB->DEVICE.CTRLA.reg |= USB_CTRLA_ENABLE;
    // Wait for enable to complete
    while (USB->DEVICE.SYNCBUSY.bit.ENABLE);
}

void usb_enable_endpoint_in (uint8_t ep, enum usb_endpoint_size size,
                             enum usb_endpoint_type type,
                             void (*callback)(void))
{
    /* Store callback */
    usb_in_callbacks_g[ep] = callback;
    /* Configure bank size */
    usb_ep_descriptors_g[ep].DeviceDescBank[1].PCKSIZE.bit.SIZE =
                                                    (__builtin_ctz(size) - 3);
    usb_ep_descriptors_g[ep].DeviceDescBank[1].PCKSIZE.bit.AUTO_ZLP = 0b1;
    /* Configure endpoint registers */
    // Configure bank type
    USB->DEVICE.DeviceEndpoint[ep].EPCFG.bit.EPTYPE1 = type;
    // Indicate that bank is not ready and clear data toggle
    USB->DEVICE.DeviceEndpoint[ep].EPSTATUSCLR.reg =
                                                (USB_DEVICE_EPSTATUSCLR_BK1RDY |
                                                 USB_DEVICE_EPSTATUSCLR_DTGLIN);
}

void usb_enable_endpoint_out (uint8_t ep, enum usb_endpoint_size size,
                              enum usb_endpoint_type type,
                              void (*callback)(uint16_t length))
{
    /* Store callback */
    usb_out_callbacks_g[ep] = callback;
    /* Configure bank size */
    usb_ep_descriptors_g[ep].DeviceDescBank[0].PCKSIZE.bit.SIZE =
                                                    (__builtin_ctz(size) - 3);
    /* Configure endpoint registers */
    // Configure bank type
    USB->DEVICE.DeviceEndpoint[ep].EPCFG.bit.EPTYPE0 = type;
    // Indicate that bank is not ready
    USB->DEVICE.DeviceEndpoint[ep].EPSTATUSSET.reg =
                                                USB_DEVICE_EPSTATUSSET_BK0RDY;
    // Clear data toggle
    USB->DEVICE.DeviceEndpoint[ep].EPSTATUSCLR.reg =
                                                USB_DEVICE_EPSTATUSCLR_DTGLOUT;
}

void usb_disable_endpoint_in (uint8_t ep)
{
    /* Indicate that bank is not ready */
    USB->DEVICE.DeviceEndpoint[ep].EPSTATUSCLR.reg =
                                                USB_DEVICE_EPSTATUSCLR_BK1RDY;
    /* Set type to disabled */
    USB->DEVICE.DeviceEndpoint[ep].EPCFG.bit.EPTYPE1 =
                                                USB_ENDPOINT_TYPE_DISABLED;
}

void usb_disable_endpoint_out (uint8_t ep)
{
    /* Indicate that bank is not ready */
    USB->DEVICE.DeviceEndpoint[ep].EPSTATUSSET.reg =
                                                USB_DEVICE_EPSTATUSSET_BK0RDY;
    /* Set type to disabled */
    USB->DEVICE.DeviceEndpoint[ep].EPCFG.bit.EPTYPE0 =
                                                USB_ENDPOINT_TYPE_DISABLED;
}

void usb_start_in (uint8_t ep, const uint8_t *data, uint16_t length,
                   uint8_t zlp)
{
    /* Configure endpoint descriptor */
    usb_ep_descriptors_g[ep].DeviceDescBank[1].PCKSIZE.bit.AUTO_ZLP = zlp;
    usb_ep_descriptors_g[ep].DeviceDescBank[1].PCKSIZE.bit.MULTI_PACKET_SIZE =
                                                                            0;
    usb_ep_descriptors_g[ep].DeviceDescBank[1].PCKSIZE.bit.BYTE_COUNT = length;
    usb_ep_descriptors_g[ep].DeviceDescBank[1].ADDR.reg = (uint32_t)data;
    /* Configure endpoint registers */
    // Make sure we don't have any pending interrupts for bank 1
    USB->DEVICE.DeviceEndpoint[ep].EPINTFLAG.reg =
                                                (USB_DEVICE_EPINTFLAG_TRCPT1 |
                                                USB_DEVICE_EPINTFLAG_TRFAIL1);
    // Enable the transmission complete interrupt for bank 1
    USB->DEVICE.DeviceEndpoint[ep].EPINTENSET.reg =
                                                USB_DEVICE_EPINTENSET_TRCPT1;
    // Indicate that bank is full
    USB->DEVICE.DeviceEndpoint[ep].EPSTATUSSET.reg =
                                                USB_DEVICE_EPSTATUSSET_BK1RDY;
}

void usb_start_out (uint8_t ep, uint8_t *data, uint16_t length)
{
    /* Configure endpoint descriptor */
    usb_ep_descriptors_g[ep].DeviceDescBank[0].PCKSIZE.bit.MULTI_PACKET_SIZE =
                                                                        length;
    usb_ep_descriptors_g[ep].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT = 0;
    usb_ep_descriptors_g[ep].DeviceDescBank[0].ADDR.reg = (uint32_t)data;
    /* Configure endpoint registers */
    // Make sure we don't have any pending interrupts for bank 0
    USB->DEVICE.DeviceEndpoint[ep].EPINTFLAG.reg =
                                                (USB_DEVICE_EPINTFLAG_TRCPT0 |
                                                 USB_DEVICE_EPINTFLAG_TRFAIL0);
    // Enable the transmission complete interrupt for bank 0
    USB->DEVICE.DeviceEndpoint[ep].EPINTENSET.reg =
                                                USB_DEVICE_EPINTENSET_TRCPT0;
    // Indicate that bank is empty
    USB->DEVICE.DeviceEndpoint[ep].EPSTATUSCLR.reg =
                                                USB_DEVICE_EPSTATUSCLR_BK0RDY;
}

// MARK: ISR and Helpers
/**
 *  Get a descriptor by type and index.
 *
 *  @param type The type of the descriptor to be fetched
 *  @param index The index of the descriptor to be fetched
 *  @param buffer Pointer to where a pointer to the descriptor should be placed
 *
 *  @return The length of the descriptor or 0 if the type/index combination was
 *          not valid
 */
static inline uint16_t usb_get_descriptor (enum usb_descriptor_type type,
                                           uint8_t index,
                                           const uint8_t **buffer)
{
    switch (type) {
        case USB_DESC_TYPE_DEVICE:
            *buffer = (const uint8_t*)&usb_device_descriptor_g;
            return usb_device_descriptor_g.bLength;
        case USB_DESC_TYPE_CONFIGURATION:
            *buffer = (const uint8_t*)usb_config_desc_g;
            return usb_config_desc_g->wTotalLength;
        case USB_DESC_TYPE_STRING:
            if (index == 0) {
                // Language string
                *buffer = (const uint8_t*)&usb_string_0_g;
                return usb_string_0_g.bLength;
            } else if (index > usb_num_strings_g) {
                // Not a valid string index
                return 0;
            } else {
                // Valid string
                *buffer = (const uint8_t*)usb_string_descs_g[index - 1];
                return usb_string_descs_g[index - 1]->bLength;
            }
            break;
        case USB_DESC_TYPE_INTERFACE:
        case USB_DESC_TYPE_ENDPOINT:
            // Can't get an interface or endpoint descriptor directly
            return 0;
        case USB_DESC_TYPE_DEVICE_QUALIFIER:
            // Not supported
            return 0;
        case USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION:
            // Not supported
            return 0;
        case USB_DESC_TYPE_INTERFACE_POWER:
            // Can't get and interface power descriptor directly
            return 0;
        default:
            // Unknown descriptor type
            return 0;
    }
    return 0;
}

/**
 *  Handle a setup packet on endpoint 0.
 *
 *  @param packet The setup packet to be handled
 */
static inline void usb_handle_setup (struct usb_setup_packet *packet)
{
    uint16_t response_length = 0;
    const uint8_t *response_buffer = usb_ep_0_in_buf_g;
    
    if ((packet->bmRequestType.type == USB_REQ_TYPE_CLASS) &&
            (usb_class_request_g != NULL)) {
        uint8_t ret = usb_class_request_g(packet, &response_length,
                                          &response_buffer);
        if (ret) {
            // Request Error
            usb_stall(0, USB_ENDPOINT_STALL_BOTH);
            return;
        }
    } else if (packet->bmRequestType.type != USB_REQ_TYPE_STANDARD) {
        return;
    } else {
        switch (packet->bRequest) {
            case USB_REQ_GET_STATUS:
                // We will never have any flags set, no matter the contents of
                // the request, so we just send two 0 bytes
                usb_ep_0_in_buf_g[0] = 0;
                usb_ep_0_in_buf_g[1] = 0;
                response_length = 2;
                break;
            case USB_REQ_CLEAR_FEATURE:
            case USB_REQ_SET_FEATURE:
                // We don't care about any of the possible features
                // Send a 0 length response
                response_length = 0;
                break;
            case USB_REQ_SET_ADDRESS:
                // Set the device address (but don't enable it yet)
                USB->DEVICE.DADD.reg = packet->wValue & 0x7f;
                // Send a 0 length response
                response_length = 0;
                break;
            case USB_REQ_GET_DESCRIPTOR:
                response_length = usb_get_descriptor(packet->descriptor_type,
                                                     packet->descriptor_index,
                                                     &response_buffer);
                if (response_length == 0) {
                    // Requested descriptor not supported, Request Error
                    usb_stall(0, USB_ENDPOINT_STALL_BOTH);
                    return;
                } else if (response_length > packet->wLength) {
                    // Our descriptor is too big, only send as much as the host
                    // wants
                    response_length = packet->wLength;
                }
                break;
            case USB_REQ_SET_DESCRIPTOR:
                // Request Error
                usb_stall(0, USB_ENDPOINT_STALL_BOTH);
                return;
            case USB_REQ_GET_CONFIGURATION:
                // Return the current configuration
                usb_ep_0_in_buf_g[0] = usb_current_config_g;
                response_length = 1;
                break;
            case USB_REQ_SET_CONFIGURATION:
                // Only a single configuration is currently supported
                if ((packet->wValue == 0) && (usb_current_config_g != 0)) {
                    usb_current_config_g = 0;
                    // Call disable configuration callback
                    if (usb_disable_config_g) {
                        usb_disable_config_g();
                    }
                } else if ((packet->wValue ==
                            usb_config_desc_g->bConfigurationValue) &&
                           (usb_current_config_g !=
                               usb_config_desc_g->bConfigurationValue)) {
                    usb_current_config_g =
                                        usb_config_desc_g->bConfigurationValue;
                    // Call enable configuration callback
                    if (usb_enable_config_g) {
                        usb_enable_config_g();
                    }
                } else {
                    // Request Error
                    usb_stall(0, USB_ENDPOINT_STALL_BOTH);
                    return;
                }
                // Send a 0 length response
                response_length = 0;
                break;
            case USB_REQ_GET_INTERFACE:
                if (usb_current_config_g == 0) {
                    // Not configured (in address or default state)
                    // Request Error
                    usb_stall(0, USB_ENDPOINT_STALL_BOTH);
                    return;
                } else {
                    // TODO: Not clear what to do here, for now just return 0
                    usb_ep_0_in_buf_g[0] = 0;
                    response_length = 1;
                }
                break;
            case USB_REQ_SET_INTERFACE:
                // TODO: Not clear what to do here, for now just stall
                usb_stall(0, USB_ENDPOINT_STALL_BOTH);
                return;
            case USB_REQ_SYNCH_FRAME:
                // Not supported, Request Error
                usb_stall(0, USB_ENDPOINT_STALL_BOTH);
                return;
            default:
                // Unknown request, Request Error
                usb_stall(0, USB_ENDPOINT_STALL_BOTH);
                return;
        }
    }
    
    /* Send the response */
    if ((response_buffer == usb_ep_0_in_buf_g) || (response_length == 0)) {
        // We are sending directly from the in buffer or we are sending a zero
        // length response, let hardware handle transmission and ZLP
        usb_start_in(0, response_buffer, response_length, 1);
        // Be ready for another setup packet
        //usb_start_out(0, usb_ep_0_out_buf_g, USB_EP_0_LENGTH);
    } else {
        // We are sending from some unknown buffer, we don't know for sure so we
        // have to assume that it is in non-volatile memory and therefor not
        // accessible by the USBs DMA capability. In order to send the buffer we
        // need to copy it into our out buffer in blocks and handle ZLP
        // ourselves.
        
        // Set up global pointer and size variables
        usb_ep_0_in_p = response_buffer;
        usb_ep_0_in_size = response_length;
        // Start multi packet in transaction
        handle_ep_0_multi_packet_in();
    }
    usb_start_out(0, usb_ep_0_out_buf_g, USB_EP_0_LENGTH);
}


// MARK: ISR
void USB_Handler (void)
{
    if (USB->DEVICE.INTFLAG.bit.EORST) {
        /* Reset */
        // Clear interrupt flag
        USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTFLAG_EORST;
        // Reconfigure endpoint 0
        config_endpoint_zero();
        // Don't bother checking for endpoint interrupts since we just reset
        return;
    }
    
    if (USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.bit.RXSTP) {
        /* Received setup packet on endpoint 0 */
        usb_handle_setup((struct usb_setup_packet*)usb_ep_0_out_buf_g);
        // Clear interrupt flag
        USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.reg =
                                                USB_DEVICE_EPINTFLAG_RXSTP;
    }
    
    while (USB->DEVICE.EPINTSMRY.reg) {
        uint8_t i = __builtin_ctz(USB->DEVICE.EPINTSMRY.reg);
        /* Interrupt on endpoint i */
        if (USB->DEVICE.DeviceEndpoint[i].EPINTFLAG.bit.TRCPT0) {
            /* Out complete */
            if (usb_out_callbacks_g[i] != NULL) {
                usb_out_callbacks_g[i](
                    usb_ep_descriptors_g[i].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT);
            }
            // Clear interrupt flag
            USB->DEVICE.DeviceEndpoint[i].EPINTFLAG.reg =
                                                USB_DEVICE_EPINTFLAG_TRCPT0;
        }
        if (USB->DEVICE.DeviceEndpoint[i].EPINTFLAG.bit.TRCPT1) {
            /* In complete */
            if (usb_in_callbacks_g[i] != NULL) {
                usb_in_callbacks_g[i]();
            }
            // Clear interrupt flag
            USB->DEVICE.DeviceEndpoint[i].EPINTFLAG.reg =
                                                USB_DEVICE_EPINTFLAG_TRCPT1;
        }
        
        if (USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.bit.RXSTP) {
            /* Got another setup packet on endpoint 0 */
            // Return to get out of the loop, ISR will be called again right
            // away.
            return;
        }
    }
}


#include "usb-standard.h"
#include "usb-cdc-standard.h"
