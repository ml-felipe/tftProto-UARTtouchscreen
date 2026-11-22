#define _XOPEN_SOURCE 600
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "uart_touchscreen.h"
int main(int argc, char **argv) {
    (void)argv;
    if (argc > 1) { assert(init_uart() == -1); close_uart(); return 0; }
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    assert(master >= 0 && grantpt(master) == 0 && unlockpt(master) == 0);
    assert(symlink(ptsname(master), SERIAL_PORT) == 0);
    assert(init_uart() == 0);
    int slave = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    struct termios settings;
    assert(slave >= 0 && tcgetattr(slave, &settings) == 0);
    assert(cfgetispeed(&settings) == B9600 && cfgetospeed(&settings) == B9600);
    assert((settings.c_cflag & CSIZE) == CS8);
    assert(!(settings.c_cflag & (PARENB | CSTOPB)));
    assert(settings.c_cc[VMIN] == 1 && settings.c_cc[VTIME] == 0);
    assert(!(settings.c_lflag & (ICANON | ECHO)));
    unsigned char frames[] = {'X','Y',1,128,2,255,3,'\n','X','Y',0,1,0,2,0,'\n'};
    assert(write(master, frames, sizeof frames) == sizeof frames);
    Pair point;
    assert(getxy(&point) == 0 && point.x == 384 && point.y == 767 && !pressing);
    assert(getxy(&point) == 0 && point.x == 1 && point.y == 2);
    close(master); close(slave);
    assert(getxy(&point) == -1);
    close_uart(); close_uart();
    assert(unlink(SERIAL_PORT) == 0);
    return 0;
}
