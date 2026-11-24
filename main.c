#include <string.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"

#define NUM_PINS 22
#define DEBOUNCE_US 2000 // 2ms eager-press / deferred-release debounce

typedef struct __attribute__((packed)) {
    uint16_t buttons;
    int8_t x;
    int8_t y;
    int8_t z;
    int8_t rx;
    int8_t ry;
    int8_t rz;
} gamepad_report_t;

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

gamepad_report_t build_report(uint8_t base_pin) {
    gamepad_report_t rpt = {0};
    uint32_t s = debounced_state >> base_pin;

    // Start (bit 0) -> bit 7
    if (s & (1u << 0)) rpt.buttons |= (1u << 7);

    // Rotated 90 degrees left (CCW)
    if (s & (1u << 3)) rpt.x = -127; // Physical Up    -> Left (-X)
    if (s & (1u << 4)) rpt.x =  127; // Physical Down  -> Right (+X)
    if (s & (1u << 2)) rpt.y =  127; // Physical Left  -> Down (+Y)
    if (s & (1u << 1)) rpt.y = -127; // Physical Right -> Up (-Y)

    // Arcade buttons 0..4 (s bits 5..9) -> bit positions 2, 3, 5, 0, 1
    if (s & (1u << 5)) rpt.buttons |= (1u << 2);
    if (s & (1u << 6)) rpt.buttons |= (1u << 3);
    if (s & (1u << 7)) rpt.buttons |= (1u << 5);
    if (s & (1u << 8)) rpt.buttons |= (1u << 0);
    if (s & (1u << 9)) rpt.buttons |= (1u << 1);

    // 6th arcade button (s bit 10) -> keep only the output that matched
    if (s & (1u << 10)) {
        rpt.rz = 127;
        rpt.buttons |= (1u << 11);
    }

    return rpt;
}

int main(void) {
    board_init();
    init_gpio();
    tusb_init();

    gamepad_report_t last_p1 = {0xFFFF, 0, 0};
    gamepad_report_t last_p2 = {0xFFFF, 0, 0};

    while (1) {
        // Sub-poll at tens of kHz continuously
        subpoll_inputs();
        tud_task();

        if (tud_hid_n_ready(0)) {
            gamepad_report_t p1 = build_report(0); // GP0..GP10
            if (memcmp(&p1, &last_p1, sizeof(p1)) != 0) {
                tud_hid_n_report(0, 0, &p1, sizeof(p1));
                last_p1 = p1;
            }
        }

        if (tud_hid_n_ready(1)) {
            gamepad_report_t p2 = build_report(11); // GP11..GP21
            if (memcmp(&p2, &last_p2, sizeof(p2)) != 0) {
                tud_hid_n_report(1, 0, &p2, sizeof(p2));
                last_p2 = p2;
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
