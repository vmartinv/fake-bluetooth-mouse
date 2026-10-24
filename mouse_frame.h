// Turns evdev mouse events into the 7-byte serial frames the board expects (see README.md).
#pragma once
#include <linux/input.h>

#include <algorithm>
#include <array>
#include <cstdint>

constexpr uint8_t SYNC = 0xA5;
using Frame = std::array<uint8_t, 7>;

inline Frame encodeFrame(uint8_t buttons, int dx, int dy, int wheel) {
    auto i16 = [](int v) { return static_cast<int16_t>(std::clamp(v, -32768, 32767)); };
    int16_t x = i16(dx), y = i16(dy);
    return {SYNC,
            buttons,
            uint8_t(x),
            uint8_t(x >> 8),
            uint8_t(y),
            uint8_t(y >> 8),
            uint8_t(static_cast<int8_t>(std::clamp(wheel, -127, 127)))};
}

inline int buttonBit(int code) {
    switch (code) {
    case BTN_LEFT: return 0;
    case BTN_RIGHT: return 1;
    case BTN_MIDDLE: return 2;
    case BTN_SIDE: return 3;  // back
    case BTN_EXTRA: return 4;  // forward
    default: return -1;
    }
}

// Sums motion and tracks buttons until a SYN_REPORT closes the evdev packet.
class MouseState {
public:
    // Returns true when e ends a packet; frame() is then ready to send.
    bool feed(const input_event &e) {
        if (e.type == EV_REL) {
            if (e.code == REL_X) dx_ += e.value;
            else if (e.code == REL_Y) dy_ += e.value;
            else if (e.code == REL_WHEEL) wheel_ += e.value;
        } else if (e.type == EV_KEY) {
            int bit = buttonBit(e.code);
            if (bit >= 0) buttons_ = e.value ? buttons_ | 1 << bit : buttons_ & ~(1 << bit);
        } else if (e.type == EV_SYN && e.code == SYN_REPORT) {
            return true;
        }
        return false;
    }
    // The pending packet as a frame; motion is reset, buttons stay held.
    Frame takeFrame() {
        Frame f = encodeFrame(buttons_, dx_, dy_, wheel_);
        dx_ = dy_ = wheel_ = 0;
        return f;
    }

private:
    uint8_t buttons_ = 0;
    int dx_ = 0, dy_ = 0, wheel_ = 0;
};
