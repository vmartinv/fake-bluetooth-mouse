// Unit tests for mouse_frame.h. Frames are decoded the way fake-bluetooth-mouse.ino does.
#include "../mouse_frame.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

struct Decoded {
    uint8_t buttons;
    int dx, dy, wheel;
};

static Decoded decode(const Frame &f) {
    return {f[1], int16_t(f[2] | f[3] << 8), int16_t(f[4] | f[5] << 8), int8_t(f[6])};
}

static input_event ev(uint16_t type, uint16_t code, int32_t value) {
    input_event e {};
    e.type = type;
    e.code = code;
    e.value = value;
    return e;
}

TEST_CASE("encodeFrame round-trips") {
    Frame f = encodeFrame(0x13, -300, 1234, -2);
    CHECK(f[0] == SYNC);
    Decoded d = decode(f);
    CHECK(d.buttons == 0x13);
    CHECK(d.dx == -300);
    CHECK(d.dy == 1234);
    CHECK(d.wheel == -2);
}

TEST_CASE("encodeFrame clamps to the field range") {
    Decoded d = decode(encodeFrame(0, 100000, -100000, 500));
    CHECK(d.dx == 32767);
    CHECK(d.dy == -32768);
    CHECK(d.wheel == 127);
    CHECK(decode(encodeFrame(0, 0, 0, -500)).wheel == -127);
}

TEST_CASE("buttonBit maps the known buttons") {
    CHECK(buttonBit(BTN_LEFT) == 0);
    CHECK(buttonBit(BTN_RIGHT) == 1);
    CHECK(buttonBit(BTN_MIDDLE) == 2);
    CHECK(buttonBit(BTN_SIDE) == 3);
    CHECK(buttonBit(BTN_EXTRA) == 4);
    CHECK(buttonBit(BTN_TASK) == -1);
}

TEST_CASE("MouseState sums motion until SYN_REPORT") {
    MouseState s;
    CHECK_FALSE(s.feed(ev(EV_REL, REL_X, 3)));
    CHECK_FALSE(s.feed(ev(EV_REL, REL_X, 4)));
    CHECK_FALSE(s.feed(ev(EV_REL, REL_Y, -5)));
    CHECK_FALSE(s.feed(ev(EV_REL, REL_WHEEL, 1)));
    CHECK_FALSE(s.feed(ev(EV_REL, REL_HWHEEL, 9)));  // not forwarded
    CHECK(s.feed(ev(EV_SYN, SYN_REPORT, 0)));
    Decoded d = decode(s.takeFrame());
    CHECK(d.dx == 7);
    CHECK(d.dy == -5);
    CHECK(d.wheel == 1);
}

TEST_CASE("MouseState resets motion after a frame") {
    MouseState s;
    s.feed(ev(EV_REL, REL_X, 3));
    s.takeFrame();
    Decoded d = decode(s.takeFrame());
    CHECK(d.dx == 0);
    CHECK(d.dy == 0);
    CHECK(d.wheel == 0);
}

TEST_CASE("MouseState keeps buttons held across frames") {
    MouseState s;
    s.feed(ev(EV_KEY, BTN_LEFT, 1));
    s.feed(ev(EV_KEY, BTN_EXTRA, 1));
    CHECK(decode(s.takeFrame()).buttons == 0x11);
    CHECK(decode(s.takeFrame()).buttons == 0x11);
    s.feed(ev(EV_KEY, BTN_LEFT, 0));
    CHECK(decode(s.takeFrame()).buttons == 0x10);
    s.feed(ev(EV_KEY, BTN_TASK, 1));  // unmapped
    CHECK(decode(s.takeFrame()).buttons == 0x10);
}
