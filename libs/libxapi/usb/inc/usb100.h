/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * <usb100.h> - USB 1.1 standard descriptor layouts and spec constants used by
 * the Xbox USB stack (hcdi.h / usb.h / usbd). These are the fixed, byte-packed
 * descriptor structures and enumerated values from the USB 1.1 specification;
 * the Xbox-specific request/class machinery lives in hcdi.h. This header was
 * previously supplied implicitly by the zig toolchain's bundled Windows headers;
 * it is vendored here so RXDK is self-contained under the LLVM toolchain, which
 * provides no such header. See docs/llvm-toolchain-plan.md.
 *
 * Assumes UCHAR/USHORT/WCHAR are already in scope (as in the Xbox DDK header),
 * which the including translation units pull in ahead of <usb100.h>.
 */
#ifndef __USB100_H__
#define __USB100_H__

#include <pshpack1.h>

/* wire-format descriptors are byte-packed */

typedef struct _USB_DEVICE_DESCRIPTOR {
    UCHAR  bLength;
    UCHAR  bDescriptorType;
    USHORT bcdUSB;
    UCHAR  bDeviceClass;
    UCHAR  bDeviceSubClass;
    UCHAR  bDeviceProtocol;
    UCHAR  bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR  iManufacturer;
    UCHAR  iProduct;
    UCHAR  iSerialNumber;
    UCHAR  bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;

typedef struct _USB_ENDPOINT_DESCRIPTOR {
    UCHAR  bLength;
    UCHAR  bDescriptorType;
    UCHAR  bEndpointAddress;
    UCHAR  bmAttributes;
    USHORT wMaxPacketSize;
    UCHAR  bInterval;
} USB_ENDPOINT_DESCRIPTOR, *PUSB_ENDPOINT_DESCRIPTOR;

typedef struct _USB_CONFIGURATION_DESCRIPTOR {
    UCHAR  bLength;
    UCHAR  bDescriptorType;
    USHORT wTotalLength;
    UCHAR  bNumInterfaces;
    UCHAR  bConfigurationValue;
    UCHAR  iConfiguration;
    UCHAR  bmAttributes;
    UCHAR  MaxPower;
} USB_CONFIGURATION_DESCRIPTOR, *PUSB_CONFIGURATION_DESCRIPTOR;

typedef struct _USB_INTERFACE_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bInterfaceNumber;
    UCHAR bAlternateSetting;
    UCHAR bNumEndpoints;
    UCHAR bInterfaceClass;
    UCHAR bInterfaceSubClass;
    UCHAR bInterfaceProtocol;
    UCHAR iInterface;
} USB_INTERFACE_DESCRIPTOR, *PUSB_INTERFACE_DESCRIPTOR;

typedef struct _USB_STRING_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    WCHAR bString[1];
} USB_STRING_DESCRIPTOR, *PUSB_STRING_DESCRIPTOR;

typedef struct _USB_COMMON_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
} USB_COMMON_DESCRIPTOR, *PUSB_COMMON_DESCRIPTOR;

typedef struct _USB_DEVICE_QUALIFIER_DESCRIPTOR {
    UCHAR  bLength;
    UCHAR  bDescriptorType;
    USHORT bcdUSB;
    UCHAR  bDeviceClass;
    UCHAR  bDeviceSubClass;
    UCHAR  bDeviceProtocol;
    UCHAR  bMaxPacketSize0;
    UCHAR  bNumConfigurations;
    UCHAR  bReserved;
} USB_DEVICE_QUALIFIER_DESCRIPTOR, *PUSB_DEVICE_QUALIFIER_DESCRIPTOR;

/* USB 1.1 hub class descriptor (bDescriptorType 0x29) */
typedef struct _USB_HUB_DESCRIPTOR {
    UCHAR  bDescriptorLength;
    UCHAR  bDescriptorType;
    UCHAR  bNumberOfPorts;
    USHORT wHubCharacteristics;
    UCHAR  bPowerOnToPowerGood;
    UCHAR  bHubControlCurrent;
    UCHAR  bRemoveAndPowerMask[64];
} USB_HUB_DESCRIPTOR, *PUSB_HUB_DESCRIPTOR;

#include <poppack.h>

/* bDescriptorType values */
#define USB_DEVICE_DESCRIPTOR_TYPE                    0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE             0x02
#define USB_STRING_DESCRIPTOR_TYPE                    0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE                 0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE                  0x05
#define USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE          0x06
#define USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE 0x07
#define USB_INTERFACE_POWER_DESCRIPTOR_TYPE           0x08
#define USB_HUB_DESCRIPTOR_TYPE                       0x29

/* build a (type<<8 | index) value for GET_DESCRIPTOR */
#define USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(d, i) \
    ((USHORT)((USHORT)(d) << 8 | (i)))

/* endpoint bEndpointAddress / bmAttributes fields */
#define USB_ENDPOINT_DIRECTION_MASK    0x80
#define USB_ENDPOINT_DIRECTION_OUT(a)  (!((a) & USB_ENDPOINT_DIRECTION_MASK))
#define USB_ENDPOINT_DIRECTION_IN(a)   ((a) & USB_ENDPOINT_DIRECTION_MASK)

#define USB_ENDPOINT_TYPE_MASK         0x03
#define USB_ENDPOINT_TYPE_CONTROL      0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS  0x01
#define USB_ENDPOINT_TYPE_BULK         0x02
#define USB_ENDPOINT_TYPE_INTERRUPT    0x03

/* configuration bmAttributes */
#define USB_CONFIG_POWERED_MASK        0xC0
#define USB_CONFIG_BUS_POWERED         0x80
#define USB_CONFIG_SELF_POWERED        0x40
#define USB_CONFIG_REMOTE_WAKEUP       0x20

/* bDeviceClass / bInterfaceClass values */
#define USB_DEVICE_CLASS_RESERVED           0x00
#define USB_DEVICE_CLASS_AUDIO              0x01
#define USB_DEVICE_CLASS_COMMUNICATIONS    0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE   0x03
#define USB_DEVICE_CLASS_MONITOR           0x04
#define USB_DEVICE_CLASS_PHYSICAL_INTERFACE 0x05
#define USB_DEVICE_CLASS_POWER             0x06
#define USB_DEVICE_CLASS_PRINTER           0x07
#define USB_DEVICE_CLASS_STORAGE           0x08
#define USB_DEVICE_CLASS_HUB               0x09
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC   0xFF

#define MAXIMUM_USB_STRING_LENGTH          255

/* standard device requests (bRequest), USB 1.1 spec table 9-4 */
#define USB_REQUEST_GET_STATUS         0x00
#define USB_REQUEST_CLEAR_FEATURE      0x01
#define USB_REQUEST_SET_FEATURE        0x03
#define USB_REQUEST_SET_ADDRESS        0x05
#define USB_REQUEST_GET_DESCRIPTOR     0x06
#define USB_REQUEST_SET_DESCRIPTOR     0x07
#define USB_REQUEST_GET_CONFIGURATION  0x08
#define USB_REQUEST_SET_CONFIGURATION  0x09
#define USB_REQUEST_GET_INTERFACE      0x0A
#define USB_REQUEST_SET_INTERFACE      0x0B
#define USB_REQUEST_SYNC_FRAME         0x0C

/* standard feature selectors (wValue for SET_FEATURE/CLEAR_FEATURE) */
#define USB_FEATURE_ENDPOINT_STALL     0x0000
#define USB_FEATURE_REMOTE_WAKEUP      0x0001

#endif /* __USB100_H__ */
