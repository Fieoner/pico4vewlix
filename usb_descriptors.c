#include <string.h>
#include "tusb.h"

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0xFF,
    .bDeviceSubClass    = 0xFF,
    .bDeviceProtocol    = 0xFF,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x045E,
    .idProduct          = 0x028E,
    .bcdDevice          = 0x0101,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

enum {
    ITF_NUM_P1 = 0,
    ITF_NUM_P2,
    ITF_NUM_TOTAL
};

#define XINPUT_ITF_LEN    (9 + 17 + 7 + 7)
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + (2 * XINPUT_ITF_LEN))

// One Xbox 360 wired controller interface (class FF / subclass 5D / protocol 01)
#define XINPUT_ITF_DESCRIPTOR(itf, ep_in, ep_out) \
    9, TUSB_DESC_INTERFACE, (itf), 0, 2, 0xFF, 0x5D, 0x01, 0, \
    /* vendor-specific XInput descriptor */ \
    17, 0x21, 0x00, 0x01, 0x01, 0x25, (ep_in), 0x14, 0x00, 0x00, 0x00, 0x00, 0x13, (ep_out), 0x08, 0x00, 0x00, \
    7, TUSB_DESC_ENDPOINT, (ep_in), TUSB_XFER_INTERRUPT, 32, 0, 1, \
    7, TUSB_DESC_ENDPOINT, (ep_out), TUSB_XFER_INTERRUPT, 32, 0, 8

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 500),
    XINPUT_ITF_DESCRIPTOR(ITF_NUM_P1, 0x81, 0x01),
    XINPUT_ITF_DESCRIPTOR(ITF_NUM_P2, 0x82, 0x02)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 }, // 0: English
    "Custom Arcade",               // 1: Manufacturer
    "Dual Arcade 2P",              // 2: Product
    "000002",                      // 3: Serial
    "Arcade Player 1",             // 4: Interface 0
    "Arcade Player 2",             // 5: Interface 1
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;
    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) return NULL;
        const char* str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) _desc_str[1 + i] = str[i];
    }
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
