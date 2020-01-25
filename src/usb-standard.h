/**
 * @file usb-standard.h
 * @desc Definitions from Universal Serial Bus Specification Rev. 2
 * @author Samuel Dewan
 * @date 2019-10-25
 * Last Author: Samuel Dewan
 * Last Edited On: 2019-12-27
 */

#ifndef usb_standard_h
#define usb_standard_h

// Ignore warnings in this file about inefficient alignment
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wpacked"

//
//
//  Setup Packet
//
//  See Universal Serial Bus Specification Revision 2 - Section 9.4
//
//

/**
 *  Data transfer direction
 */
enum usb_data_transfer_dir {
    USB_DATA_TRANS_HOST_TO_DEVICE = 0,
    USB_DATA_TRANS_DEVICE_TO_HOST = 1
};

/**
 *  Type of USB request
 */
enum usb_request_type {
    USB_REQ_TYPE_STANDARD = 0,
    USB_REQ_TYPE_CLASS = 1,
    USB_REQ_TYPE_VENDOR = 2
};

/**
 *  Recipient of USB request
 */
enum usb_request_recipient {
    USB_REQ_RECIPIENT_DEVICE = 0,
    USB_REQ_RECIPIENT_INTERFACE = 1,
    USB_REQ_RECIPIENT_ENDPOINT = 2,
    USB_REQ_RECIPIENT_OTHER = 3
};

/**
 *  Setup packet requests (refer to USB Spec. revision 2 - Table 9-3)
 */
enum usb_request {
    USB_REQ_GET_STATUS = 0,
    USB_REQ_CLEAR_FEATURE = 1,
    USB_REQ_SET_FEATURE = 3,
    USB_REQ_SET_ADDRESS = 5,
    USB_REQ_GET_DESCRIPTOR = 6,
    USB_REQ_SET_DESCRIPTOR = 7,
    USB_REQ_GET_CONFIGURATION = 8,
    USB_REQ_SET_CONFIGURATION = 9,
    USB_REQ_GET_INTERFACE = 10,
    USB_REQ_SET_INTERFACE = 11,
    USB_REQ_SYNCH_FRAME = 12
};

/**
 *  USB Setup Packet.
 */
struct usb_setup_packet {
    /** Characteristics of request */
    struct {
        /** Recipient */
        enum usb_request_recipient recipient: 5;
        /** Type */
        enum usb_request_type type: 2;
        /** Data transfer direction in second phase of control transfer */
        enum usb_data_transfer_dir data_transfer_dir: 1;
    } bmRequestType;
    
    /** Specific request (refer to USB Spec. revision 2 - Table 9-3) */
    enum usb_request bRequest: 8;
    
    /** Field that varies depending on request */
    union {
        uint16_t wValue;
        struct {
            /** Index for GetDescriptor packet */
            uint8_t descriptor_index;
            /** Type for GetDescriptor packet */
            uint8_t descriptor_type;
        };
    };
    /** Field that varies depending on request */
    union {
        struct {
            uint8_t RESERVED;
            enum usb_data_transfer_dir data_transfer_dir: 1;
            uint8_t RESERVED_2: 3;
            uint8_t endpoint_num: 4;
        };
        struct {
            uint8_t RESERVED_3;
            uint8_t interface_num;
        };
        uint16_t raw;
    } wIndex;
    /** Number of bytes to transfer if there is a data stage */
    uint16_t wLength;
    /** Data stage */
    uint8_t data[];
} __attribute__ ((packed));


//
//
//  Descriptors
//
//  See Universal Serial Bus Specification Revision 2 - Sections 9.4 to 9.6
//
//

#define USB_LANGUAGE_EN_US  0x0409

/**
 *  Type of USB descriptor
 */
enum usb_descriptor_type {
    USB_DESC_TYPE_DEVICE = 1,
    USB_DESC_TYPE_CONFIGURATION = 2,
    USB_DESC_TYPE_STRING = 3,
    USB_DESC_TYPE_INTERFACE = 4,
    USB_DESC_TYPE_ENDPOINT = 5,
    USB_DESC_TYPE_DEVICE_QUALIFIER = 6,
    USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION = 7,
    USB_DESC_TYPE_INTERFACE_POWER = 8,
    USB_DESC_TYPE_OTG = 9,
    USB_DESC_TYPE_DEBUG = 10,
    USB_DESC_TYPE_INTERFACE_ASSOCIATION = 11,
};

/**
 *  Selector used for enabling or setting features
 */
enum usb_feature_selector {
    USB_FEAT_ENDPOINT_HALT = 0,
    USB_FEAT_DEVICE_REMOTE_WAKEUP = 1,
    USB_FEAT_TEST_MODE = 2
};

/**
 *  Maximum packet size for a device
 */
enum usb_endpoint_size {
    USB_ENDPOINT_SIZE_8 = 8,
    USB_ENDPOINT_SIZE_16 = 16,
    USB_ENDPOINT_SIZE_32 = 32,
    USB_ENDPOINT_SIZE_64 = 64
};

/**
 *  Transfer type for endpoint
 */
enum usb_transfer_type {
    USB_TRANS_TYPE_CONTROL = 0b00,
    USB_TRANS_TYPE_ISOCHRONOUS = 0b01,
    USB_TRANS_TYPE_BULK = 0b10,
    USB_TRANS_TYPE_INTERRUPT = 0b11
};

/**
 *  Synchronization type for endpoint
 */
enum usb_synchronization_type {
    USB_SYNC_TYPE_NONE = 0b00,
    USB_SYNC_TYPE_ASYNCHRONOUS = 0b01,
    USB_SYNC_TYPE_ADAPTIVE = 0b10,
    USB_SYNC_TYPE_SYNCHRONOUS = 0b11
};

/**
 *  Usage type for endpoint
 */
enum usb_usage_type {
    USB_USAGE_TYPE_DATA = 0b00,
    USB_USAGE_TYPE_FEEDBACK = 0b01,
    USB_USAGE_TYPE_IMPLICIT_FEEDBACK = 0b10
};

/**
 *  Descriptor which provides general information about a USB device.
 */
struct usb_device_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_DESC_TYPE_DEVICE) */
    enum usb_descriptor_type bDescriptorType: 8;
    /** USB Specification release number in binary coded decimal */
    uint16_t bcdUSB;
    /** Class code (assigned by the USB-IF) */
    uint8_t bDeviceClass;
    /** Subclass code (assigned by the USB-IF) */
    uint8_t bDeviceSubClass;
    /** Protocol code (assigned by the USB-IF) */
    uint8_t bDeviceProtocol;
    /** Maximum packet size for endpoint zero */
    enum usb_endpoint_size bMaxPacketSize0: 8;
    /** Vendor ID (assigned by the USB-IF) */
    uint16_t idVendor;
    /** Product ID (assigned by the manufacturer) */
    uint16_t idProduct;
    /** Device release number in binary-coded decimal */
    uint16_t bcdDevice;
    /** Index of string descriptor describing manufacturer */
    uint8_t iManufacturer;
    /** Index of string descriptor describing product */
    uint8_t iProduct;
    /** Index of string descriptor describing the device’s serial number */
    uint8_t iSerialNumber;
    /** Number of possible configurations */
    uint8_t bNumConfigurations;
} __attribute__ ((packed));

/**
 *  Describes information about a specific device configuration.
 */
struct usb_configuration_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_DESC_TYPE_CONFIGURATION) */
    enum usb_descriptor_type bDescriptorType: 8;
    /** Total length of data returned for this configuration.
        Includes the combined length of all descriptors (configuration,
        interface, endpoint, and class- or vendor-specific) returned for this
        configuration. */
    uint16_t wTotalLength;
    /** Number of interfaces supported by this configuration */
    uint8_t bNumInterfaces;
    /** Value to use as an argument to the SetConfiguration() request to select
        this configuration */
    uint8_t bConfigurationValue;
    /** Index of string descriptor describing this configuration */
    uint8_t iConfiguration;
    /** Configuration characteristics */
    struct {
        /** Reserved (reset to zero) */
        uint8_t RESERVED_2: 5;
        /** Remote Wakeup */
        uint8_t remote_wakeup: 1;
        /** Self-powered */
        uint8_t self_powered: 1;
        /** Reserved (set to one) */
        uint8_t RESERVED: 1;
    } bmAttributes;
    /** Maximum power consumption of the USB device from the bus in this
        specific configuration when the device is fully operational. Expressed
        in 2 mA units (i.e., 50 = 100 mA). */
    uint8_t bMaxPower;
} __attribute__ ((packed));

/**
 *  Describes a specific interface within a configuration.
 */
struct usb_interface_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_DESC_TYPE_INTERFACE) */
    enum usb_descriptor_type bDescriptorType: 8;
    /** Number of this interface. Zero-based value identifying the index in the
        array of concurrent interfaces supported by this configuration. */
    uint8_t bInterfaceNumber;
    /** Value used to select this alternate setting for the interface identified
        in the prior field */
    uint8_t bAlternateSetting;
    /** Number of endpoints used by this interface (excluding endpoint zero). */
    uint8_t bNumEndpoints;
    /** Class code (assigned by the USB-IF) */
    uint8_t bInterfaceClass;
    /** Subclass code (assigned by the USB-IF */
    uint8_t bInterfaceSubClass;
    /** Protocol code (assigned by the USB-IF) */
    uint8_t bInterfaceProtocol;
    /** Index of string descriptor describing this interface */
    uint8_t iInterface;
} __attribute__ ((packed));

/**
 *  Describes an endpoint within an interface.
 */
struct usb_endpoint_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_DESC_TYPE_ENDPOINT) */
    enum usb_descriptor_type bDescriptorType: 8;
    /** The address of the endpoint on the USB device */
    struct {
        /** The endpoint number */
        uint8_t endpoint_number: 4;
        /** Reserved, reset to zero */
        uint8_t RESERVED: 3;
        /** Direction, ignored for control endpoints */
        enum usb_data_transfer_dir direction: 1;
    } bEndpointAddress;
    /** The endpoint’s attributes when it is configured */
    struct {
        enum usb_transfer_type transfer_type: 2;
        enum usb_synchronization_type sync_type: 2;
        enum usb_usage_type usage_type: 2;
        /** Reserved, reset to zero */
        uint8_t RESERVED: 2;
    } bmAttributes;
    /** Maximum packet size this endpoint is capable of sending or receiving */
    uint16_t wMaxPacketSize;
    /** Interval for polling endpoint for data transfers in frames or
        microframes depending on device speed */
    uint8_t bInterval;
} __attribute__ ((packed));

/**
 *  First string descriptor, specifies languages supported by the device.
 */
struct usb_string_descriptor_zero {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_DESC_TYPE_STRING) */
    enum usb_descriptor_type bDescriptorType: 8;
    /** Codes for supported languages, must have ((length - 2) / 2) elements */
    uint16_t wLangid[];
} __attribute__ ((packed));

/**
 *  String descriptor
 */
struct usb_string_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_DESC_TYPE_STRING) */
    enum usb_descriptor_type bDescriptorType: 8;
    /** UTF-16LE encoded string (see Unicode ECN), must have ((length - 2) / 2)
        characters */
    __CHAR16_TYPE__ bString[];
} __attribute__ ((packed));



/**
 *  Interface association descriptor
 *
 *  See Universal Serial Bus Specification Revision 2 - Interface Association
 *  Descriptor Engineering Change Notice
 */
struct usb_interface_association_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_DESC_TYPE_INTERFACE_ASSOCIATION) */
    enum usb_descriptor_type bDescriptorType: 8;
    /** Interface number of first interface associated with this function */
    uint8_t bFirstInterface;
    /** Number of contiguous interfaces associated with this function */
    uint8_t bInterfaceCount;
    /** Class code (assigned by USB-IF) */
    uint8_t bFunctionClass;
    /** Subclass code (assigned by USB-IF) */
    uint8_t bFunctionSubClass;
    /** Protocol code (assigned by USB-IF) */
    uint8_t bFunctionProtocol;
    /** Index of string descriptor describing this function */
    uint8_t iFunction;
} __attribute__ ((packed));


// Stop ignoring warnings about inefficient alignment
#pragma GCC diagnostic pop

#endif /* usb_standard_h */
