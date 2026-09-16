// SPDX-FileCopyrightText: 2026 Gabriele Arcangelo Scalici (Phobos Instruments)
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cctype>
#include <string>
#include <vector>

namespace hum {
namespace morse {

struct Seg {
    bool on = false;
    int units = 0;
};

inline const char* code(char c) {
    switch (std::toupper((unsigned char) c)) {
        case 'A': return ".-";     case 'B': return "-...";   case 'C': return "-.-.";
        case 'D': return "-..";    case 'E': return ".";      case 'F': return "..-.";
        case 'G': return "--.";    case 'H': return "....";   case 'I': return "..";
        case 'J': return ".---";   case 'K': return "-.-";    case 'L': return ".-..";
        case 'M': return "--";     case 'N': return "-.";     case 'O': return "---";
        case 'P': return ".--.";   case 'Q': return "--.-";   case 'R': return ".-.";
        case 'S': return "...";    case 'T': return "-";      case 'U': return "..-";
        case 'V': return "...-";   case 'W': return ".--";    case 'X': return "-..-";
        case 'Y': return "-.--";   case 'Z': return "--..";
        case '0': return "-----";  case '1': return ".----";  case '2': return "..---";
        case '3': return "...--";  case '4': return "....-";  case '5': return ".....";
        case '6': return "-....";  case '7': return "--...";  case '8': return "---..";
        case '9': return "----.";
        case '.': return ".-.-.-"; case ',': return "--..--"; case '?': return "..--..";
        case '\'': return ".----."; case '!': return "-.-.--"; case '/': return "-..-.";
        case '(': return "-.--.";  case ')': return "-.--.-"; case '&': return ".-...";
        case ':': return "---..."; case ';': return "-.-.-."; case '=': return "-...-";
        case '+': return ".-.-.";  case '-': return "-....-"; case '_': return "..--.-";
        case '"': return ".-..-."; case '$': return "...-..-"; case '@': return ".--.-.";
        default:  return "";
    }
}

inline std::vector<Seg> encode(const std::string& text) {
    std::vector<Seg> out;
    auto gap = [&](int units) {
        if (out.empty()) return;
        if (!out.back().on) out.back().units = units > out.back().units ? units : out.back().units;
        else out.push_back({false, units});
    };
    for (const char c : text) {
        if (std::isspace((unsigned char) c)) { gap(7); continue; }
        const char* symbols = code(c);
        if (*symbols == '\0') continue;
        gap(3);
        for (const char* s = symbols; *s; ++s) {
            if (s != symbols) gap(1);
            out.push_back({true, *s == '-' ? 3 : 1});
        }
    }
    if (!out.empty() && !out.back().on) out.pop_back();
    return out;
}

}
}
