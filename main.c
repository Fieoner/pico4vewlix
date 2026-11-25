#include <string.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "pico/bootrom.h"

#define NUM_PINS 22
#define P1_BASE_PIN 0
#define P2_BASE_PIN 11
#define START_PIN_MASK ((1u << P1_BASE_PIN) | (1u << P2_BASE_PIN)) // Start buttons
#define DEBOUNCE_US 2000 // 2ms eager-press / deferred-release debounce

static uint32_t debounced_state = 0;
static uint64_t last_change_us[NUM_PINS] = {0};

void init_gpio(void) {
    for (uint i = 0; i < NUM_PINS; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
        gpio_pull_up(i);
    }
}

// Fast sub-polling routine: reads all 22 GPIOs in 1 CPU cycle
void subpoll_inputs(void) {
    uint64_t now = time_us_64();
    // Invert raw reading so 1 = pressed (shorted to GND)
    uint32_t raw = ~gpio_get_all() & ((1u << NUM_PINS) - 1);

    for (uint i = 0; i < NUM_PINS; i++) {
        uint32_t mask = (1u << i);
        bool raw_bit = (raw & mask) != 0;
        bool cur_bit = (debounced_state & mask) != 0;

        if (raw_bit != cur_bit && (now - last_change_us[i]) >= DEBOUNCE_US) {
            if (raw_bit) debounced_state |= mask;
            else         debounced_state &= ~mask;
            last_change_us[i] = now;
        }
    }
}

typedef struct __attribute__((packed)) {
    uint16_t buttons;
    int8_t x;
    int8_t y;
} gamepad_report_t;

// HID button bits, laid out like an Xbox pad (SDL/Windows button order)
#define BTN_A     (1u << 0)
#define BTN_B     (1u << 1)
#define BTN_X     (1u << 2)
#define BTN_Y     (1u << 3)
#define BTN_RB    (1u << 5)
#define BTN_START (1u << 7)
#define BTN_RT    (1u << 9)  // digital button, not an analog trigger

// Per-player pin offsets: 0 Start, 1 Up, 2 Down, 3 Left, 4 Right, 5..10 buttons 1..6
// Top row (5,6,7) = X, Y, RB.  Bottom row (8,9,10) = A, B, RT.
static gamepad_report_t build_report(uint8_t base_pin) {
    gamepad_report_t rpt = {0};
    uint32_t s = debounced_state >> base_pin;

    if (s & (1u << 0)) rpt.buttons |= BTN_START;

    // Joystick -> X/Y axes (HID +Y is down)
    if (s & (1u << 1)) rpt.y = -127; // Up
    if (s & (1u << 2)) rpt.y =  127; // Down
    if (s & (1u << 3)) rpt.x = -127; // Left
    if (s & (1u << 4)) rpt.x =  127; // Right

    if (s & (1u << 5))  rpt.buttons |= BTN_X;
    if (s & (1u << 6))  rpt.buttons |= BTN_Y;
    if (s & (1u << 7))  rpt.buttons |= BTN_RB;
    if (s & (1u << 8))  rpt.buttons |= BTN_A;
    if (s & (1u << 9))  rpt.buttons |= BTN_B;
    if (s & (1u << 10)) rpt.buttons |= BTN_RT;
    return rpt;
}

int main(void) {
    board_init();
    init_gpio();

    // Hold either Start button while plugging in to enter the USB bootloader
    sleep_ms(20); // let the pull-ups settle
    if (~gpio_get_all() & START_PIN_MASK) {
        reset_usb_boot(0, 0);
    }

    tusb_init();

    gamepad_report_t last[2] = { {0xFFFF, 0, 0}, {0xFFFF, 0, 0} };
    const uint8_t base[2] = { P1_BASE_PIN, P2_BASE_PIN };

    while (1) {
        subpoll_inputs();
        tud_task();

        for (uint8_t i = 0; i < 2; i++) {
            if (!tud_hid_n_ready(i)) continue;
            gamepad_report_t cur = build_report(base[i]);
            if (memcmp(&cur, &last[i], sizeof(cur)) != 0) {
                tud_hid_n_report(i, 0, &cur, sizeof(cur));
                last[i] = cur;
            }
        }
    }
}

// Required TinyUSB HID callbacks
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) itf; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) itf; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}
