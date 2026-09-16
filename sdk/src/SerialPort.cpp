// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: AGPL-3.0-only
#include "hum/SerialPort.h"

#include <algorithm>
#include <cstdint>

#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace hum::serial {

void applyCustomSpeed(int fd, int baud);

#if JUCE_WINDOWS

std::vector<std::string> listDevices() {
    std::vector<std::string> out;
    char target[512];
    for (int i = 1; i <= 256; ++i) {
        const std::string name = "COM" + std::to_string(i);
        if (QueryDosDeviceA(name.c_str(), target, sizeof(target)) != 0) out.push_back(name);
    }
    return out;
}

bool Port::open(const std::string& path, const Settings& settings) {
    close();
    const std::string device = path.rfind("\\\\.\\", 0) == 0 ? path : "\\\\.\\" + path;
    HANDLE h = CreateFileA(device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (GetCommState(h, &dcb)) {
        dcb.BaudRate = (DWORD) settings.baud;
        dcb.ByteSize = 8;
        dcb.Parity = settings.frame == kFrame8E1 ? EVENPARITY : settings.frame == kFrame8O1 ? ODDPARITY : NOPARITY;
        dcb.StopBits = settings.frame == kFrame8N2 ? TWOSTOPBITS : ONESTOPBIT;
        dcb.fBinary = TRUE;
        dcb.fParity = settings.frame == kFrame8E1 || settings.frame == kFrame8O1;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        dcb.fNull = FALSE;
        dcb.fAbortOnError = FALSE;
        SetCommState(h, &dcb);
    }
    if (settings.reset) {
        EscapeCommFunction(h, CLRDTR);
        Sleep(100);
        EscapeCommFunction(h, SETDTR);
    }
    handle_ = (std::intptr_t) h;
    path_ = path;
    settings_ = settings;
    return true;
}

void Port::close() {
    if (handle_ != -1) CloseHandle((HANDLE) handle_);
    handle_ = -1;
    path_.clear();
    settings_ = Settings{};
}

int Port::read(void* buf, int max, int timeoutMs) {
    if (handle_ == -1 || max <= 0) return -1;
    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout = MAXDWORD;
    to.ReadTotalTimeoutMultiplier = MAXDWORD;
    to.ReadTotalTimeoutConstant = (DWORD) std::max(1, timeoutMs);
    to.WriteTotalTimeoutConstant = 200;
    SetCommTimeouts((HANDLE) handle_, &to);
    DWORD got = 0;
    if (!ReadFile((HANDLE) handle_, buf, (DWORD) max, &got, nullptr)) return -1;
    return (int) got;
}

bool Port::write(const void* buf, int n) {
    if (handle_ == -1) return false;
    const char* p = (const char*) buf;
    while (n > 0) {
        DWORD put = 0;
        if (!WriteFile((HANDLE) handle_, p, (DWORD) n, &put, nullptr)) return false;
        if (put == 0) return false;
        p += put;
        n -= (int) put;
    }
    return true;
}

#else

std::vector<std::string> listDevices() {
    std::vector<std::string> out;
    for (const char* pattern : {"tty.usbmodem*", "tty.usbserial*", "ttyACM*", "ttyUSB*"}) {
        auto found = juce::File("/dev").findChildFiles(juce::File::findFiles, false, pattern);
        for (const auto& f : found) out.push_back(f.getFullPathName().toStdString());
    }
    std::sort(out.begin(), out.end());
    return out;
}

namespace {

bool standardSpeed(int baud, speed_t& sp) {
    struct BaudMap { int b; speed_t s; };
    static const BaudMap map[] = {{300, B300},     {1200, B1200},   {2400, B2400},
                                  {4800, B4800},   {9600, B9600},   {19200, B19200},
                                  {38400, B38400}, {57600, B57600}, {115200, B115200},
                                  {230400, B230400}};
    for (const auto& m : map)
        if (baud == m.b) { sp = m.s; return true; }
    return false;
}

void pulseDtr(int fd) {
    int dtr = TIOCM_DTR;
    ::ioctl(fd, TIOCMBIC, &dtr);
    ::usleep(100 * 1000);
    ::ioctl(fd, TIOCMBIS, &dtr);
}

}

bool Port::open(const std::string& path, const Settings& settings) {
    close();
    const int fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return false;
    termios tio{};
    bool custom = false;
    if (::tcgetattr(fd, &tio) == 0) {
        ::cfmakeraw(&tio);
        speed_t sp = B9600;
        custom = !standardSpeed(settings.baud, sp);
        ::cfsetispeed(&tio, sp);
        ::cfsetospeed(&tio, sp);
        tio.c_cflag |= CLOCAL | CREAD;
        if (settings.frame == kFrame8E1 || settings.frame == kFrame8O1) tio.c_cflag |= PARENB;
        if (settings.frame == kFrame8O1) tio.c_cflag |= PARODD;
        if (settings.frame == kFrame8N2) tio.c_cflag |= CSTOPB;
        if (!settings.reset) tio.c_cflag &= ~(tcflag_t) HUPCL;
        ::tcsetattr(fd, TCSANOW, &tio);
    }
    if (custom) applyCustomSpeed(fd, settings.baud);
    if (settings.reset) pulseDtr(fd);
    handle_ = fd;
    path_ = path;
    settings_ = settings;
    return true;
}

void Port::close() {
    if (handle_ != -1) ::close((int) handle_);
    handle_ = -1;
    path_.clear();
    settings_ = Settings{};
}

int Port::read(void* buf, int max, int timeoutMs) {
    if (handle_ == -1 || max <= 0) return -1;
    const int fd = (int) handle_;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    timeval tv{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
    const int ready = ::select(fd + 1, &set, nullptr, nullptr, &tv);
    if (ready < 0) return errno == EINTR ? 0 : -1;
    if (ready == 0) return 0;
    const ssize_t n = ::read(fd, buf, (size_t) max);
    if (n > 0) return (int) n;
    if (n == 0) return -1;
    return errno == EAGAIN || errno == EINTR ? 0 : -1;
}

bool Port::write(const void* buf, int n) {
    if (handle_ == -1) return false;
    const char* p = (const char*) buf;
    int stalls = 0;
    while (n > 0) {
        const ssize_t put = ::write((int) handle_, p, (size_t) n);
        if (put > 0) { p += put; n -= (int) put; continue; }
        if (put < 0 && errno != EAGAIN && errno != EINTR) return false;
        if (++stalls > 50) return false;
        ::usleep(2000);
    }
    return true;
}

#endif

}
