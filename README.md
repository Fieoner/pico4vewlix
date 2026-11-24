# pico4vewlix

RPi Pico firmware that exposes a single board as **two independent USB HID gamepads** — one per player — for a 2-player Vewlix-style arcade panel.

## Pin mapping

| GP | P1 function | GP | P2 function |
|----|-------------|----|-------------|
| 0  | Start       | 11 | Start       |
| 1  | Right       | 12 | Right       |
| 2  | Left        | 13 | Left        |
| 3  | Up          | 14 | Up          |
| 4  | Down        | 15 | Down        |
| 5  | Button 1    | 16 | Button 1    |
| 6  | Button 2    | 17 | Button 2    |
| 7  | Button 3    | 18 | Button 3    |
| 8  | Button 4    | 19 | Button 4    |
| 9  | Button 5    | 20 | Button 5    |
| 10 | Button 6    | 21 | Button 6    |

All inputs are **active-low** (pulled up internally; short to GND to press).  
The joystick is rotated 90° CCW to match a Vewlix-style layout.

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
