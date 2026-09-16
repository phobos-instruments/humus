// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_LINUX
#include <asm/termbits.h>
#include <sys/ioctl.h>
#elif JUCE_MAC
#include <IOKit/serial/ioss.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

namespace hum::serial {

void applyCustomSpeed(int fd, int baud) {
#if JUCE_LINUX
    struct termios2 t2{};
    if (::ioctl(fd, TCGETS2, &t2) != 0) return;
    t2.c_cflag &= ~(tcflag_t) CBAUD;
    t2.c_cflag |= BOTHER;
    t2.c_ispeed = (speed_t) baud;
    t2.c_ospeed = (speed_t) baud;
    ::ioctl(fd, TCSETS2, &t2);
#elif JUCE_MAC
    const speed_t speed = (speed_t) baud;
    ::ioctl(fd, IOSSIOSPEED, &speed);
#else
    (void) fd;
    (void) baud;
#endif
}

}
