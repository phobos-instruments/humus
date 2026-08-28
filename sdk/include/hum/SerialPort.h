#pragma once
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

#include "hum/Number.h"

#if !JUCE_WINDOWS
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace hum::serial {

inline const std::vector<int>& standardBauds() {
    static const std::vector<int> rates = {300,   1200,  2400,  4800,   9600,
                                           19200, 38400, 57600, 115200, 230400};
    return rates;
}

inline int baudFromIndex(int index) {
    const auto& r = standardBauds();
    return r[(size_t) juce::jlimit(0, (int) r.size() - 1, index)];
}

inline std::vector<std::string> listDevices() {
    std::vector<std::string> out;
    for (const char* pattern : {"tty.usbmodem*", "tty.usbserial*", "ttyACM*", "ttyUSB*"}) {
        auto found = juce::File("/dev").findChildFiles(juce::File::findFiles, false, pattern);
        for (const auto& f : found) out.push_back(f.getFullPathName().toStdString());
    }
    std::sort(out.begin(), out.end());
    return out;
}

inline int openPort(const std::string& path, int baud, bool forWriting) {
#if !JUCE_WINDOWS
    const int fd = ::open(path.c_str(), (forWriting ? O_WRONLY : O_RDONLY)
                                            | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return -1;
    termios tio{};
    if (::tcgetattr(fd, &tio) == 0) {
        ::cfmakeraw(&tio);
        struct BaudMap { int b; speed_t s; };
        static const BaudMap map[] = {{300, B300},     {1200, B1200},   {2400, B2400},
                                      {4800, B4800},   {9600, B9600},   {19200, B19200},
                                      {38400, B38400}, {57600, B57600}, {115200, B115200},
                                      {230400, B230400}};
        speed_t sp = B9600;
        for (const auto& m : map)
            if (baud >= m.b) sp = m.s;
        ::cfsetispeed(&tio, sp);
        ::cfsetospeed(&tio, sp);
        tio.c_cflag |= CLOCAL;
        ::tcsetattr(fd, TCSANOW, &tio);
    }
    return fd;
#else
    juce::ignoreUnused(path, baud, forWriting);
    return -1;
#endif
}

inline int parseFloats(const char* line, float* out, int maxVals) {
    int n = 0;
    const char* p = line;
    while (n < maxVals && *p != '\0') {
        const char* end = nullptr;
        const auto v = (float) scanDouble(p, &end);
        if (end == p) { ++p; continue; }
        p = end;
        if (!std::isfinite(v)) continue;
        out[n++] = v;
    }
    return n;
}

}
