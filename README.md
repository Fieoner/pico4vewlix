# pico4vewlix

RPi Pico firmware that exposes a single board as **two independent USB HID gamepads** — one per player — for a 2-player Vewlix-style arcade panel.

## Pin mapping

Pins are the same for both players, offset by 11. Active-low (internal pull-ups; short to GND to press).

| P1 GP | P2 GP | Input | XInput output |
|-------|-------|-------|---------------|
| 0  | 11 | Start    | Start |
| 1  | 12 | Up       | Left stick up |
| 2  | 13 | Down     | Left stick down |
| 3  | 14 | Left     | Left stick left |
| 4  | 15 | Right    | Left stick right |
| 5  | 16 | Top row, left   | X |
| 6  | 17 | Top row, middle | Y |
| 7  | 18 | Top row, right  | RB |
| 8  | 19 | Bottom row, left   | A |
| 9  | 20 | Bottom row, middle | B |
| 10 | 21 | Bottom row, right  | RT |

The board enumerates as two Xbox 360 (XInput) controllers.

## Bootloader

Hold **either Start button** while plugging in the USB cable and the board boots into the RP2040 USB bootloader (BOOTSEL drive), no need to touch the Pico.

## Build

```sh
mkdir build && cd build
cmake ..
make
```

Flash `dual_arcade_gamepad.uf2` to the Pico in BOOTSEL mode.

## Dependencies

- [pico-sdk](https://github.com/raspberrypi/pico-sdk)
- TinyUSB (bundled with the SDK)
