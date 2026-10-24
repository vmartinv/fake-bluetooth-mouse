# fake-bluetooth-mouse

Share one computer's mouse with another device without installing anything on that device.
An nRF52840 board plugged into the computer by USB pairs with the other device as a plain
Bluetooth mouse ("Desk Mouse"). `forward` grabs the computer's mouse (so its own cursor stops)
and streams it to the board over USB serial; stop `forward` to get the mouse back.

```
mouse ──evdev──> forward ──USB serial──> nRF52840 ──BLE HID──> other device
```

- `fake-bluetooth-mouse/fake-bluetooth-mouse.ino`: board firmware (Bluefruit BLE HID mouse fed by USB serial).
- `forward.cpp`: computer side (Linux). Grabs the mouse with EVIOCGRAB and writes one frame
  per SYN_REPORT to the board.
- `mouse_frame.h`: evdev events -> serial frames, unit-tested in `test/` with doctest
  (vendored `test/doctest.h`, MIT).

## Hardware
Any nRF52840 board supported by the Adafruit nRF52 Arduino core (Bluefruit), e.g. a
SuperMini nRF52840 (nice!nano clone) or an Adafruit Feather nRF52840.

## Build / flash
    make forward      # also: make test (doctest), make lint (clang-format + clang-tidy)

Flash the firmware with the Arduino IDE or arduino-cli, picking your board's FQBN and port.
For a SuperMini with the nRFMicro-like core:

    arduino-cli compile --upload -b nRFMicro-like_Boards:nrf52:supermini -p /dev/ttyACM0 fake-bluetooth-mouse

Close any serial monitor first (it holds the port).

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
