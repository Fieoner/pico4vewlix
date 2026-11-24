#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE
#define CFG_TUD_ENDPOINT0_SIZE      64

// Enable 2 independent HID interfaces (Player 1 & Player 2)
#define CFG_TUD_HID                 2
#define CFG_TUD_HID_EP_BUFSIZE      16

#endif