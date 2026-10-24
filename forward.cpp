// Grab a local mouse and stream it to the fake-bluetooth-mouse board (BLE mouse) over USB serial.
// Usage: forward <mouse event device> <board serial device>; see README.md.
#include "mouse_frame.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

volatile sig_atomic_t stop = 0;

class Fd {
public:
    Fd(const char *path, int flags) : fd_(open(path, flags | O_CLOEXEC)) {
        if (fd_ < 0) std::fprintf(stderr, "open %s: %s\n", path, std::strerror(errno));
    }
    ~Fd() {
        if (fd_ >= 0) close(fd_);
    }
    Fd(const Fd &) = delete;
    Fd &operator=(const Fd &) = delete;
    explicit operator bool() const { return fd_ >= 0; }
    int get() const { return fd_; }

private:
    int fd_;
};

static bool sendFrame(int fd, const Frame &frame) {
    return write(fd, frame.data(), frame.size()) == ssize_t(frame.size());
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <mouse event device> <board serial device>\n", argv[0]);
        return 2;
    }
    struct sigaction sa {};
    sa.sa_handler = [](int) { stop = 1; };  // no SA_RESTART: interrupts the blocking read
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    Fd board(argv[2], O_WRONLY | O_NOCTTY);
    Fd mouse(argv[1], O_RDONLY);
    if (!board || !mouse) return 1;

    termios tio {};
    tcgetattr(board.get(), &tio);
    cfmakeraw(&tio);
    tcsetattr(board.get(), TCSANOW, &tio);

    if (ioctl(mouse.get(), EVIOCGRAB, 1) < 0) {
        std::perror("EVIOCGRAB");
        return 1;
    }
    std::fprintf(stderr, "forwarding mouse to the board\n");

    MouseState state;
    input_event ev[64];
    while (!stop) {
        ssize_t n = read(mouse.get(), ev, sizeof ev);
        if (n < 0) {
            if (errno == EINTR) continue;
            std::perror("read mouse");
            break;
        }
        for (ssize_t i = 0; i < n / ssize_t(sizeof *ev); i++) {
            if (state.feed(ev[i]) && !sendFrame(board.get(), state.takeFrame())) {
                std::perror("write board");
                return 1;
            }
        }
    }
    sendFrame(board.get(), encodeFrame(0, 0, 0, 0));  // release any held button on the other device
    std::fprintf(stderr, "stopped\n");
    return 0;
}
