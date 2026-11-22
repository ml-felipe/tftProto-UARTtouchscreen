#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "include/uart_touchscreen.h"

static int serial_fd = -1;
static TouchParser parser;
static unsigned char read_buf[256];
static size_t next_byte, buffered;
static int64_t last_touch;
int calibrating;
int pressing;

int init_uart(void) {
    close_uart();
    serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if (serial_fd < 0) return -1;
    struct termios options;
    if (tcgetattr(serial_fd, &options) < 0) goto fail;
    options.c_cflag = CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 0;
    if (cfsetispeed(&options, BAUDRATE) < 0 ||
        cfsetospeed(&options, BAUDRATE) < 0 ||
        tcsetattr(serial_fd, TCSANOW, &options) < 0 ||
        tcflush(serial_fd, TCIFLUSH) < 0) goto fail;
    return 0;
fail: {
    int saved_errno = errno;
    close_uart();
    errno = saved_errno;
    return -1;
}
}

void close_uart(void) {
    if (serial_fd >= 0) close(serial_fd);
    serial_fd = -1;
    parser.used = next_byte = buffered = 0;
    last_touch = 0;
    pressing = 0;
}

int getxy(Pair *point) {
    if (!point) { errno = EINVAL; return -1; }
    if (serial_fd < 0) { errno = EBADF; return -1; }
    if (calibrating) {
        if (tcflush(serial_fd, TCIFLUSH) < 0) return -1;
        parser.used = next_byte = buffered = 0;
    }
    for (;;) {
        if (next_byte == buffered) {
            ssize_t count = read(serial_fd, read_buf, sizeof read_buf);
            if (count < 0) { if (errno == EINTR) continue; return -1; }
            if (count == 0) { errno = EIO; return -1; }
            next_byte = 0;
            buffered = (size_t)count;
        }
        if (touch_parser_feed(&parser, read_buf[next_byte++], point)) {
            struct timespec ts;
            if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) return -1;
            int64_t now = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
            pressing = last_touch != 0 && now - last_touch <= TOUCH_TIMEOUT_MS;
            last_touch = now;
            return 0;
        }
    }
}
