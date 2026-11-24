#ifndef XINPUT_DRIVER_H
#define XINPUT_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#define XINPUT_NUM_PLAYERS 2

// Button bits, byte 2 of the report (low byte of 'buttons')
#define XINPUT_DPAD_UP     0x0001
#define XINPUT_DPAD_DOWN   0x0002
#define XINPUT_DPAD_LEFT   0x0004
#define XINPUT_DPAD_RIGHT  0x0008
#define XINPUT_START       0x0010
#define XINPUT_BACK        0x0020
#define XINPUT_L3          0x0040
#define XINPUT_R3          0x0080
// Byte 3 (high byte of 'buttons')
#define XINPUT_LB          0x0100
#define XINPUT_RB          0x0200
#define XINPUT_GUIDE       0x0400
#define XINPUT_A           0x1000
#define XINPUT_B           0x2000
#define XINPUT_X           0x4000
#define XINPUT_Y           0x8000

// Standard 20-byte Xbox 360 wired controller input report
typedef struct __attribute__((packed)) {
    uint8_t  report_id;     // 0x00
    uint8_t  report_size;   // 0x14
    uint16_t buttons;
    uint8_t  lt;
    uint8_t  rt;
    int16_t  lx;
    int16_t  ly;
    int16_t  rx;
    int16_t  ry;
    uint8_t  reserved[6];
} xinput_report_t;

// Fills in the constant header and zeroes everything else
void xinput_report_init(xinput_report_t *r);

// Queue a report on a player's IN endpoint. Returns true if it was accepted.
bool xinput_send(uint8_t player, const xinput_report_t *r);

#endif
