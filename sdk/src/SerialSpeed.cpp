// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#if defined(__linux__)
#include <asm/termbits.h>
#include <sys/ioctl.h>
#elif defined(__APPLE__)
#include <IOKit/serial/ioss.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

namespace hum::serial {

void applyCustomSpeed(int fd, int baud) {
#if defined(__linux__)
    struct termios2 t2{};
    if (::ioctl(fd, TCGETS2, &t2) != 0) return;
    t2.c_cflag &= ~(tcflag_t) CBAUD;
    t2.c_cflag |= BOTHER;
    t2.c_ispeed = (speed_t) baud;
    t2.c_ospeed = (speed_t) baud;
    ::ioctl(fd, TCSETS2, &t2);
#elif defined(__APPLE__)
    const speed_t speed = (speed_t) baud;
    ::ioctl(fd, IOSSIOSPEED, &speed);
#else
    (void) fd;
    (void) baud;
#endif
}

}
