#ifndef _USB_HUB_DEFS_H
#define _USB_HUB_DEFS_H

#include <common/types.h>

#define USB_HUB_REQUEST_GET_STATUS 0
#define USB_HUB_REQUEST_CLEAR_FEATURE 1
#define USB_HUB_REQUEST_GET_STATE 2
#define USB_HUB_REQUEST_SET_FEATURE 3
#define USB_HUB_REQUEST_GET_DESCRIPTOR 6
#define USB_HUB_REQUEST_SET_DESCRIPTOR 7

#define USB_HUB_FEATURE_C_HUB_LOCAL_POWER 0
#define USB_HUB_FEATURE_C_HUB_OVER_CURRENT 1
#define USB_HUB_FEATURE_PORT_CONNECTION 0
#define USB_HUB_FEATURE_PORT_ENABLE 1
#define USB_HUB_FEATURE_PORT_SUSPEND 2
#define USB_HUB_FEATURE_PORT_OVER_CURRENT 3
#define USB_HUB_FEATURE_PORT_RESET 4
#define USB_HUB_FEATURE_PORT_POWER 8
#define USB_HUB_FEATURE_PORT_LOW_SPEED 9
#define USB_HUB_FEATURE_C_PORT_CONNECTION 16
#define USB_HUB_FEATURE_C_PORT_ENABLE 17
#define USB_HUB_FEATURE_C_PORT_SUSPEND 18
#define USB_HUB_FEATURE_C_PORT_OVER_CURRENT 19
#define USB_HUB_FEATURE_C_PORT_RESET 20

#define USB_HUB_PORT_STATUS_CONNECTION (1 << 0)
#define USB_HUB_PORT_STATUS_ENABLE (1 << 1)
#define USB_HUB_PORT_STATUS_SUSPEND (1 << 2)
#define USB_HUB_PORT_STATUS_OVER_CURRENT (1 << 3)
#define USB_HUB_PORT_STATUS_RESET (1 << 4)
#define USB_HUB_PORT_STATUS_POWER (1 << 8)
#define USB_HUB_PORT_STATUS_LOW_SPEED (1 << 9)

#define USB_HUB_PORT_CHANGE_CONNECTION (1 << 0)
#define USB_HUB_PORT_CHANGE_ENABLE (1 << 1)
#define USB_HUB_PORT_CHANGE_SUSPEND (1 << 2)
#define USB_HUB_PORT_CHANGE_OVER_CURRENT (1 << 3)
#define USB_HUB_PORT_CHANGE_RESET (1 << 4)

struct USBHubDescriptor {
	uint8_t length;
	uint8_t descriptor_type;
	uint8_t num_ports;
	uint16_t hub_characteristics;
	uint8_t pwron2pwrgood;
	uint8_t max_current;
} PACKED;

#endif
