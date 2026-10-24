# fake-bluetooth-mouse

Use your computer's mouse on a second device (a laptop, tablet, phone…) as if it were a
Bluetooth mouse, with nothing to install on the second device.

You plug a cheap nRF52840 board into the computer by USB, and the second device pairs with it
like any Bluetooth mouse. A small Linux program, `forward`, takes over the computer's mouse and
sends its movements and clicks to the board. The board passes them on over Bluetooth. Stop
`forward` and the mouse controls the computer again.

```
mouse ──evdev──> forward ──USB serial──> nRF52840 ──BLE HID──> other device
```

- `fake-bluetooth-mouse/fake-bluetooth-mouse.ino`: board firmware (Bluefruit BLE HID mouse fed by USB serial).
- `forward.cpp`: computer side (Linux). Grabs the mouse with EVIOCGRAB and writes one frame
  per SYN_REPORT to the board.

## Hardware
Any nRF52840 board supported by the Adafruit nRF52 Arduino core (Bluefruit), e.g. a
SuperMini nRF52840 (nice!nano clone) or an Adafruit Feather nRF52840.

## Flash / build
Flash the firmware with the Arduino IDE or arduino-cli, picking your board's FQBN and port.
For a SuperMini with the nRFMicro-like core:

    arduino-cli compile --upload -b nRFMicro-like_Boards:nrf52:supermini -p /dev/ttyACM0 fake-bluetooth-mouse

Close any serial monitor first (it holds the port). Then build `forward`:

    make forward      # also: make test (doctest), make lint (clang-format + clang-tidy)

## Pairing
Once, on the other device: Bluetooth settings -> add device -> "Desk Mouse". The board's
LED blinks while advertising and stays on when connected; the device reconnects by itself.

## Running
    ./forward /dev/input/by-id/<your-mouse>-event-mouse /dev/serial/by-id/<your-board>

Use the stable `/dev/input/by-id` and `/dev/serial/by-id` names. You need read access to the
input device and write access to the serial port (e.g. the `input` and `uucp`/`dialout` groups,
or run it as root). It runs until stopped with Ctrl+C or SIGTERM, releasing any held button, so any
script or service can start and stop it.

## Protocol
Serial frame (7 bytes): `0xA5, buttons, dx int16 LE, dy int16 LE, wheel int8`.
Buttons: bit 0 left, 1 right, 2 middle, 3 back, 4 forward. The firmware sums motion
that arrives while the radio is busy and sends it in the next report.

Sending `?` on the serial port (outside a frame) prints the chip temperature, the BLE
link (peer, connection interval) and frames/reports per second since the last `?`.

## License
MIT, see `LICENSE`.
