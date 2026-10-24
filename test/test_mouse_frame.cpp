// Unit tests for mouse_frame.h. Decodes frames the way fake-mouse.ino does.
#include "../mouse_frame.h"

#include <cstdio>

static int failures = 0;
#define CHECK(cond)                                                                                          \
    do {                                                                                                     \
        if (!(cond)) {                                                                                       \
            std::fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #cond);                           \
            failures++;                                                                                      \
        }                                                                                                    \
    } while (0)

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

static void testEncodeRoundTrip() {
    Frame f = encodeFrame(0x13, -300, 1234, -2);
    CHECK(f[0] == SYNC);
    Decoded d = decode(f);
    CHECK(d.buttons == 0x13);
    CHECK(d.dx == -300);
    CHECK(d.dy == 1234);
    CHECK(d.wheel == -2);
}

static void testEncodeClamps() {
    Decoded d = decode(encodeFrame(0, 100000, -100000, 500));
    CHECK(d.dx == 32767);
    CHECK(d.dy == -32768);
    CHECK(d.wheel == 127);
    CHECK(decode(encodeFrame(0, 0, 0, -500)).wheel == -127);
}

static void testButtonBits() {
    CHECK(buttonBit(BTN_LEFT) == 0);
    CHECK(buttonBit(BTN_RIGHT) == 1);
    CHECK(buttonBit(BTN_MIDDLE) == 2);
    CHECK(buttonBit(BTN_SIDE) == 3);
    CHECK(buttonBit(BTN_EXTRA) == 4);
    CHECK(buttonBit(BTN_TASK) == -1);
}

static void testStateSumsMotionUntilSyn() {
    MouseState s;
    CHECK(!s.feed(ev(EV_REL, REL_X, 3)));
    CHECK(!s.feed(ev(EV_REL, REL_X, 4)));
    CHECK(!s.feed(ev(EV_REL, REL_Y, -5)));
    CHECK(!s.feed(ev(EV_REL, REL_WHEEL, 1)));
    CHECK(!s.feed(ev(EV_REL, REL_HWHEEL, 9)));  // ignored
    CHECK(s.feed(ev(EV_SYN, SYN_REPORT, 0)));
    Decoded d = decode(s.takeFrame());
    CHECK(d.dx == 7);
    CHECK(d.dy == -5);
    CHECK(d.wheel == 1);

    d = decode(s.takeFrame());  // motion is reset after a frame
    CHECK(d.dx == 0 && d.dy == 0 && d.wheel == 0);
}

static void testStateButtonsPersist() {
    MouseState s;
    s.feed(ev(EV_KEY, BTN_LEFT, 1));
    s.feed(ev(EV_KEY, BTN_EXTRA, 1));
    s.feed(ev(EV_SYN, SYN_REPORT, 0));
    CHECK(decode(s.takeFrame()).buttons == 0x11);
    CHECK(decode(s.takeFrame()).buttons == 0x11);  // held across frames
    s.feed(ev(EV_KEY, BTN_LEFT, 0));
    CHECK(decode(s.takeFrame()).buttons == 0x10);
    s.feed(ev(EV_KEY, BTN_TASK, 1));  // unmapped button
    CHECK(decode(s.takeFrame()).buttons == 0x10);
}

int main() {
    testEncodeRoundTrip();
    testEncodeClamps();
    testButtonBits();
    testStateSumsMotionUntilSyn();
    testStateButtonsPersist();
    if (failures) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    std::puts("all tests passed");
    return 0;
}
