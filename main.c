#include <string.h>
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "tusb.h"
#include "pico/bootrom.h"
#include "xinput_driver.h"

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

// Per-player pin offsets: 0 Start, 1 Up, 2 Down, 3 Left, 4 Right, 5..10 buttons 1..6
// Top row (5,6,7) = X, Y, RB.  Bottom row (8,9,10) = A, B, RT.
static void build_report(uint8_t base_pin, xinput_report_t *rpt) {
    xinput_report_init(rpt);
    uint32_t s = debounced_state >> base_pin;

    if (s & (1u << 0)) rpt->buttons |= XINPUT_START;

    // Joystick -> left analog stick (XInput +Y is up)
    if (s & (1u << 1)) rpt->ly =  32767; // Up
    if (s & (1u << 2)) rpt->ly = -32768; // Down
    if (s & (1u << 3)) rpt->lx = -32768; // Left
    if (s & (1u << 4)) rpt->lx =  32767; // Right

    if (s & (1u << 5))  rpt->buttons |= XINPUT_X;
    if (s & (1u << 6))  rpt->buttons |= XINPUT_Y;
    if (s & (1u << 7))  rpt->buttons |= XINPUT_RB;
    if (s & (1u << 8))  rpt->buttons |= XINPUT_A;
    if (s & (1u << 9))  rpt->buttons |= XINPUT_B;
    if (s & (1u << 10)) rpt->rt = 255;
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

    xinput_report_t last[XINPUT_NUM_PLAYERS];
    xinput_report_t cur[XINPUT_NUM_PLAYERS];
    const uint8_t base[XINPUT_NUM_PLAYERS] = { P1_BASE_PIN, P2_BASE_PIN };
    for (int i = 0; i < XINPUT_NUM_PLAYERS; i++) {
        xinput_report_init(&last[i]);
        last[i].report_id = 0xFF; // force an initial send
    }

    while (1) {
        // Sub-poll continuously
        subpoll_inputs();
        tud_task();

        for (uint8_t i = 0; i < XINPUT_NUM_PLAYERS; i++) {
            build_report(base[i], &cur[i]);
            if (memcmp(&cur[i], &last[i], sizeof(cur[i])) != 0 && xinput_send(i, &cur[i])) {
                last[i] = cur[i];
            }
        }
    }
}
