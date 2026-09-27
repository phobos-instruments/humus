/* Not part of xwax. MSVC has no <unistd.h>, and timecoder.c includes it
 * without using anything from it, so this empty header lets the vendored
 * source compile on Windows unmodified. Only the MSVC build sees it:
 * engine/cmake/Vendored.cmake puts this directory on the include path there
 * and nowhere else. */
#pragma once
