# fake-bluetooth-mouse

Uses the PC's mouse on the work laptop without installing anything on the laptop.
A SuperMini nRF52840 plugged into the PC is paired with the laptop as a plain
Bluetooth mouse ("Desk Mouse"). While the Magic Keyboard is unplugged from the PC
(you're on the laptop), `forward` grabs the PC's mouse and streams it to the board.

- `fake-bluetooth-mouse/fake-bluetooth-mouse.ino`: board firmware (Bluefruit BLE HID mouse fed by USB serial).
- `forward.cpp`: PC side. Grabs `/dev/input/by-id/usb-Logitech_USB_Receiver-if01-event-mouse`
  (EVIOCGRAB, so the PC cursor stops) and writes one frame per SYN_REPORT to the board.
- `mouse_frame.h`: evdev events -> serial frames, unit-tested in `test/` with doctest
  (vendored `test/doctest.h`, MIT).

Serial frame (7 bytes): `0xA5, buttons, dx int16 LE, dy int16 LE, wheel int8`.
Buttons: bit 0 left, 1 right, 2 middle, 3 back, 4 forward. The firmware sums motion
that arrives while the radio is busy and sends it in the next report.

Sending `?` on the serial port (outside a frame) prints the chip temperature, the BLE
link (peer, connection interval) and frames/reports per second since the last `?`.

## Trigger
`/etc/udev/rules.d/90-dell-follows-keyboard.rules`: keyboard unplugged ->
`systemd-run --unit=fake-bluetooth-mouse .../forward` (plus the Dell input switch); plugged ->
`systemctl stop fake-bluetooth-mouse`. Replugging the keyboard always gives the mouse back.
Log: `journalctl -t fake-bluetooth-mouse`.

## Build / flash
Close the Arduino IDE's Serial Monitor first (it holds the port).

    make forward      # also: make test (doctest), make lint (clang-format + clang-tidy)
    CLI=/opt/arduino-ide/resources/app/lib/backend/resources/arduino-cli
    $CLI --config-file ~/.arduinoIDE/arduino-cli.yaml compile --upload \
        -b nRFMicro-like_Boards:nrf52:supermini -p /dev/ttyACM0 fake-bluetooth-mouse

## Pairing
Once, on the laptop: Bluetooth settings -> add device -> "Desk Mouse". The board's
LED blinks while advertising and stays on when connected; the laptop reconnects by itself.

## License
MIT, see `LICENSE`.
