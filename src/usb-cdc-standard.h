/**
 * @file usb-cdc-standard.h
 * @desc Definitions from Universal Serial Bus Class Definitions for
 *       Communications Devices Rev. 1.2 and Universal Serial Bus Communications
 *       Class Subclass Specification for PSTN Devices Rev. 1.2
 * @author Samuel Dewan
 * @date 2019-10-26
 * Last Author: Samuel Dewan
 * Last Edited On: 2019-12-27
 */

#ifndef usb_cdc_standard_h
#define usb_cdc_standard_h

// Ignore warnings in this file about inefficient alignment
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wpacked"

//
//
//  Communications Device Class
//  Class Specific Codes
//
//  USB Class Definitions for Communications Devices Rev. 1.2 - Section 4
//
//

#define USB_CDC_CLASS_CODE      0x02

#define USB_CDC_DATA_INTERFACE_CLASS    0x0A

/** Direct Line Control Model */
#define USB_CDC_SUBCLASS_DLC    0x01
/** Abstract Control Model */
#define USB_CDC_SUBCLASS_ACM    0x02
/** Telephone Control Model */
#define USB_CDC_SUBCLASS_TCM    0x03
/** Multi-Channel Control Model */
#define USB_CDC_SUBCLASS_MCCM   0x04
/** CAPI Control Model */
#define USB_CDC_SUBCLASS_CAPI   0x05
/** Ethernet Networking Control Model */
#define USB_CDC_SUBCLASS_ENCM   0x06
/** ATM Networking Control Model */
#define USB_CDC_SUBCLASS_ATM    0x07
/** Wireless Handset Control Model */
#define USB_CDC_SUBCLASS_WHCM   0x08
/** Device Management */
#define USB_CDC_SUBCLASS_DM     0x09
/** Mobile Direct Line Model */
#define USB_CDC_SUBCLASS_MDL    0x0A
/** OBEX */
#define USB_CDC_SUBCLASS_OBEX   0x0B
/** Ethernet Emulation Model */
#define USB_CDC_SUBCLASS_EM     0x0C
/** Network Control Model */
#define USB_CDC_SUBCLASS_NCM    0x0C


/**
 *  Protocol code values for control interface.
 */
enum usb_cdc_comm_iface_protocol {
    /** No class specific protocol required */
    USB_CDC_COMM_PROTOCOL_NONE = 0x00,
    /** AT Commands: V.250 etc */
    USB_CDC_COMM_PROTOCOL_AT_V250 = 0x01,
    /** AT Commands defined by PCCA-101 */
    USB_CDC_COMM_PROTOCOL_AT_PCCA = 0x02,
    /** AT Commands defined by PCCA-101 & Annex O */
    USB_CDC_COMM_PROTOCOL_AT_PCCA_ANNEX = 0x03,
    /** AT Commands defined by GSM 07.07 */
    USB_CDC_COMM_PROTOCOL_AT_GSM = 0x04,
    /** AT Commands defined by 3GPP 27.007 */
    USB_CDC_COMM_PROTOCOL_AT_3GPP = 0x05,
    /** AT Commands defined by TIA for CDMA */
    USB_CDC_COMM_PROTOCOL_AT_TIA = 0x06,
    /** Ethernet Emulation Model */
    USB_CDC_COMM_PROTOCOL_EEM = 0x07,
    /** External Protocol: Commands defined by Command Set Functional
        Descriptor */
    USB_CDC_COMM_PROTOCOL_EXTERNAL = 0xFE,
    /** Vendor-specific */
    USB_CDC_COMM_PROTOCOL_VENDOR_SPECIFIC = 0xFF
};

/**
 *  Protocol code values for data interface.
 */
enum usb_cdc_data_iface_protocol {
    /** No class specific protocol required */
    USB_CDC_DATA_PROTOCOL_NONE = 0x00,
    /** Network Transfer Block */
    USB_CDC_DATA_PROTOCOL_NTB = 0x01,
    /** Physical interface protocol for ISDN BRI */
    USB_CDC_DATA_PROTOCOL_ISDN_BRI = 0x30,
    /** HDLC */
    USB_CDC_DATA_PROTOCOL_HDLC = 0x31,
    /** Transparent */
    USB_CDC_DATA_PROTOCOL_TRANSPARENT = 0x32,
    /** Management protocol for Q.921 data link protocol */
    USB_CDC_DATA_PROTOCOL_Q921_MANAGEMENT = 0x50,
    /** Data link protocol for Q.931 */
    USB_CDC_DATA_PROTOCOL_Q931_DATA = 0X51,
    /** TEI-multiplexor for Q.921 data link protocol */
    USB_CDC_DATA_PROTOCOL_Q921_DATA = 0x52,
    /** Data compression procedures */
    USB_CDC_DATA_PROTOCOL_DCP = 0x90,
    /** Euro-ISDN protocol control */
    USB_CDC_DATA_PROTOCOL_EURO_ISDN = 0x91,
    /** V.24 rate adaptation to ISDN */
    USB_CDC_DATA_PROTOCOL_V24_ISDN = 0x92,
    /** CAPI Commands */
    USB_CDC_DATA_PROTOCOL_CAPI = 0x93,
    /** Host based driver */
    USB_CDC_DATA_PROTOCOL_HOST_BASED = 0xFD,
    /** Protocol(s) described using Protocol Unit Functional Descriptors on
        Communications Class Interface */
    USB_CDC_DATA_PROTOCOL_PUFD = 0xFE,
    /** Vendor-specific */
    USB_CDC_DATA_PROTOCOL_VENDOR_SPECIFIC = 0xFF
};

/**
 *  Class request codes for PTSN subclass specific requests
 */
enum usb_cdc_request {
    USB_CDC_REQ_SEND_ENCAPSULATED_COMMAND = 0x00,
    USB_CDC_REQ_GET_ENCAPSULATED_RESPONSE = 0x01,
    USB_CDC_REQ_SET_COMM_FEATURE = 0x02,
    USB_CDC_REQ_GET_COMM_FEATURE = 0x03,
    USB_CDC_REQ_CLEAR_COMM_FEATURE = 0x04,
    
    USB_CDC_REQ_SET_AUX_LINE_STATE = 0x10,
    USB_CDC_REQ_SET_HOOK_STATE = 0x11,
    USB_CDC_REQ_PULSE_SETUP = 0x12,
    USB_CDC_REQ_SEND_PULSE = 0x13,
    USB_CDC_REQ_SET_PULSE_TIME = 0x14,
    USB_CDC_REQ_RING_AUX_JACK = 0x15,
    
    USB_CDC_REQ_SET_LINE_CODING = 0x20,
    USB_CDC_REQ_GET_LINE_CODING = 0x21,
    USB_CDC_REQ_SET_CONTROL_LINE_STATE = 0x22,
    USB_CDC_REQ_SEND_BREAK = 0x23,
    
    USB_CDC_REQ_SET_RINGER_PARMS = 0x30,
    USB_CDC_REQ_GET_RINGER_PARMS = 0x31,
    USB_CDC_REQ_SET_OPERATION_PARMS = 0x32,
    USB_CDC_REQ_GET_OPERATION_PARMS = 0x33,
    USB_CDC_REQ_SET_LINE_PARMS = 0x34,
    USB_CDC_REQ_GET_LINE_PARMS = 0x35,
    USB_CDC_REQ_DIAL_DIGITS = 0x36,
    USB_CDC_REQ_SET_UNIT_PARAMETER = 0x37,
    USB_CDC_REQ_GET_UNIT_PARAMETER = 0x38,
    USB_CDC_REQ_CLEAR_UNIT_PARAMETER = 0x39,
    USB_CDC_REQ_GET_PROFILE = 0x3A,
    
    USB_CDC_REQ_SET_ETHERNET_MULTICAST_FILTERS = 0x40,
    USB_CDC_REQ_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER = 0x41,
    USB_CDC_REQ_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER = 0x42,
    USB_CDC_REQ_SET_ETHERNET_PACKET_FILTER = 0x43,
    USB_CDC_REQ_GET_ETHERNET_STATISTIC = 0x44,
    
    USB_CDC_REQ_SET_ATM_DATA_FORMAT = 0x50,
    USB_CDC_REQ_GET_ATM_DEVICE_STATISTICS = 0x51,
    USB_CDC_REQ_SET_ATM_DEFAULT_VC = 0x52,
    USB_CDC_REQ_GET_ATM_VC_STATISTICS = 0x53,
    
    USB_CDC_REQ_GET_NTB_PARAMETERS = 0x80,
    USB_CDC_REQ_GET_NET_ADDRESS = 0x81,
    USB_CDC_REQ_SET_NET_ADDRESS = 0x82,
    USB_CDC_REQ_GET_NTB_FORMAT = 0x83,
    USB_CDC_REQ_SET_NTB_FORMAT = 0x84,
    USB_CDC_REQ_GET_NTB_INPUT_SIZE = 0x85,
    USB_CDC_REQ_SET_NTB_INPUT_SIZE = 0x86,
    USB_CDC_REQ_GET_MAX_DATAGRAM_SIZE = 0x87,
    USB_CDC_REQ_SET_MAX_DATAGRAM_SIZE = 0x88,
    USB_CDC_REQ_GET_CRC_MODE = 0x89,
    USB_CDC_REQ_SET_CRC_MODE = 0x8A
};

//
//
//  Communications Device Class
//  Descriptors
//
//  USB Class Definitions for Communications Devices Rev. 1.2 - Section 5
//
//

/**
 *  Type of USB CDC descriptor
 */
enum usb_cdc_descriptor_type {
    USB_CDC_DESC_TYPE_CS_INTERFACE = 0x24,
    USB_CDC_DESC_TYPE_CS_ENDPOINT = 0x25
};

/**
 *  Subtype of USB CDC descriptor
 */
enum usb_cdc_descriptor_subtype {
    /** Header Functional Descriptor, which marks the beginning of the
        concatenated set of functional descriptors for the interface */
    USB_CDC_DESC_SUBTYPE_HEADER = 0x00,
    /** Call Management Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_CALL_MANAGMENT = 0x01,
    /** Abstract Control Management Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_ACM = 0x02,
    /** Direct Line Management Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_DLM = 0x03,
    /** Telephone Ringer Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_TELEPHONE_RINGER = 0x04,
    /** Telephone Call and Line State Reporting Capabilities Functional
        Descriptor */
    USB_CDC_DESC_SUBTYPE_TELEPHONE_CALL = 0x05,
    /** Union Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_UNION = 0x06,
    /** Country Selection Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_COUNTRY_SEL = 0x07,
    /** Telephone Operational Modes Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_TELEPHONE_OP_MODES = 0x08,
    /** USB Terminal Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_TERM = 0x09,
    /** Network Channel Terminal Descriptor */
    USB_CDC_DESC_SUBTYPE_NETWORK = 0x0A,
    /** Protocol Unit Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_PROTOCOL = 0x0B,
    /** Extension Unit Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_EXTENTION = 0x0C,
    /** Multi-Channel Management Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_MULTI_CHANNEL = 0x0D,
    /** CAPI Control Management Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_CAPI = 0x0E,
    /** Ethernet Networking Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_ETHERNET = 0x0F,
    /** ATM Networking Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_ATM = 0x10,
    /** Wireless Handset Control Model Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_WLESS_HANDSET = 0x11,
    /** Mobile Direct Line Model Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_MDL = 0x12,
    /** MDLM Detail Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_MDLM = 0x13,
    /** Device Management Model Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_DMM = 0x14,
    /** OBEX Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_OBEX = 0x15,
    /** Command Set Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_CMD_SET = 0x16,
    /** Command Set Detail Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_CMD_SET_DETAIL = 0x17,
    /** Telephone Control Model Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_TCM = 0x18,
    /** OBEX Service Identifier Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_OBEX_SERVICE = 0x19,
    /** NCM Functional Descriptor */
    USB_CDC_DESC_SUBTYPE_NCM = 0x1A,
};
 

/**
 *  Header Functional Descriptor
 */
struct usb_cdc_header_functional_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_CDC_DESC_TYPE_CS_INTERFACE) */
    enum usb_cdc_descriptor_type bDescriptorType: 8;
    /** Functional descriptor subtype (must be USB_CDC_DESC_SUBTYPE_HEADER) */
    enum usb_cdc_descriptor_subtype bDescriptorSubtype:8;
    /** USB Class Definitions for Communications Devices Specification release
        number in binary-coded decimal. */
    uint16_t bcdCDC;
} __attribute__ ((packed));

/**
 *  Union Functional Descriptor
 */
struct usb_cdc_union_functional_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_CDC_DESC_TYPE_CS_INTERFACE) */
    enum usb_cdc_descriptor_type bDescriptorType: 8;
    /** Functional descriptor subtype (must be USB_CDC_DESC_SUBTYPE_UNION) */
    enum usb_cdc_descriptor_subtype bDescriptorSubtype:8;
    /** The interface number of the Communications or Data Class interface
        designated as the controlling interface for the union */
    uint8_t bControlInterface;
    /** Interface numbers of subordinate interface in union */
    uint8_t bSlaveInterface;
} __attribute__ ((packed));

/**
 *  Country Selection Functional Descriptor
 */
struct usb_cdc_country_sel_functional_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_CDC_DESC_TYPE_CS_INTERFACE) */
    enum usb_cdc_descriptor_type bDescriptorType: 8;
    /** Functional descriptor subtype (must be
        USB_CDC_DESC_SUBTYPE_COUNTRY_SEL) */
    enum usb_cdc_descriptor_subtype bDescriptorSubtype:8;
    /** Index of a string giving the release date for the implemented ISO 3166
        Country Codes. */
    uint8_t iCountryCodeRelDate;
    /** Country codes in the format as defined in [ISO3166] with release date as
        specified in iCountryCodeRelDate */
    uint16_t wCountryCodes[];
} __attribute__ ((packed));




//
//
//  Communications Class Subclass Specification for PSTN Devices
//  Descriptors
//
//  USB Communications Class Subclass Specification for PSTN Devices
//  Rev. 1.2 - Section 5
//
//

/**
 *  Call Management Functional Descriptor
 */
struct usb_cdc_call_management_functional_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_CDC_DESC_TYPE_CS_INTERFACE) */
    enum usb_cdc_descriptor_type bDescriptorType: 8;
    /** Functional descriptor subtype (must be
        USB_CDC_DESC_SUBTYPE_CALL_MANAGEMENT) */
    enum usb_cdc_descriptor_subtype bDescriptorSubtype:8;
    /** Capabilities that this configuration supports */
    struct {
        /** Whether device handles call management itself */
        uint8_t handles_call_management:1;
        /** Whether device can send/receive call management information over a
            Data Class interface */
        uint8_t use_data_interface:1;
        /** Reserved (Reset to zero) */
        uint8_t RESERVED:6;
    } bmCapabilities;
    /** Interface number for Data Class interface optionally used for call
        management */
    uint8_t bDataInterface;
} __attribute__ ((packed));

/**
 *  Abstract Control Management Functional Descriptor
 */
struct usb_cdc_acm_functional_descriptor {
    /** Size of this descriptor in bytes */
    uint8_t bLength;
    /** Descriptor type (must be USB_CDC_DESC_TYPE_CS_INTERFACE) */
    enum usb_cdc_descriptor_type bDescriptorType: 8;
    /** Functional descriptor subtype (must be USB_CDC_DESC_SUBTYPE_ACM) */
    enum usb_cdc_descriptor_subtype bDescriptorSubtype:8;
    /** Capabilities that this configuration supports */
    union {
        struct {
            /** Whether device supports Comm_Feature requests */
            uint8_t supports_comm_feature:1;
            /** Whether device supports setting line coding and serial state */
            uint8_t supports_line_coding:1;
            /** Whether device supports the request Send_Break */
            uint8_t supports_send_break:1;
            /** Whether device supports the notification Network_Connection */
            uint8_t supports_network_connection:1;
            /** Reserved (Reset to zero) */
            uint8_t RESERVED:4;
        };
        uint8_t raw;
    } bmCapabilities;
    
} __attribute__ ((packed));


// Stop ignoring warnings about inefficient alignment
#pragma GCC diagnostic pop

#endif /* usb_cdc_standard_h */
