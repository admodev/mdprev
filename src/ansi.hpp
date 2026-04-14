#pragma once
#include <string>

namespace ansi {

constexpr auto RESET     = "\033[0m";
constexpr auto BOLD      = "\033[1m";
constexpr auto DIM       = "\033[2m";
constexpr auto ITALIC    = "\033[3m";
constexpr auto UNDERLINE = "\033[4m";
constexpr auto STRIKE    = "\033[9m";

constexpr auto BLACK   = "\033[30m";
constexpr auto RED     = "\033[31m";
constexpr auto GREEN   = "\033[32m";
constexpr auto YELLOW  = "\033[33m";
constexpr auto BLUE    = "\033[34m";
constexpr auto MAGENTA = "\033[35m";
constexpr auto CYAN    = "\033[36m";
constexpr auto WHITE   = "\033[37m";

constexpr auto BRIGHT_BLACK   = "\033[90m";
constexpr auto BRIGHT_RED     = "\033[91m";
constexpr auto BRIGHT_GREEN   = "\033[92m";
constexpr auto BRIGHT_YELLOW  = "\033[93m";
constexpr auto BRIGHT_BLUE    = "\033[94m";
constexpr auto BRIGHT_MAGENTA = "\033[95m";
constexpr auto BRIGHT_CYAN    = "\033[96m";
constexpr auto BRIGHT_WHITE   = "\033[97m";

constexpr auto BG_BLACK   = "\033[40m";
constexpr auto BG_RED     = "\033[41m";
constexpr auto BG_GREEN   = "\033[42m";
constexpr auto BG_YELLOW  = "\033[43m";
constexpr auto BG_BLUE    = "\033[44m";
constexpr auto BG_MAGENTA = "\033[45m";
constexpr auto BG_CYAN    = "\033[46m";
constexpr auto BG_WHITE   = "\033[47m";
constexpr auto BG_DEFAULT = "\033[49m";

// Return visible (non-ANSI) length of a string.
inline int visible_length(const std::string& s) {
    int len = 0;
    bool in_esc = false;
    for (char c : s) {
        if (c == '\033') { in_esc = true; continue; }
        if (in_esc)      { if (c == 'm') in_esc = false; continue; }
        ++len;
    }
    return len;
}

// Build a horizontal rule using a repeated character, padded to terminal width.
inline std::string hline(char ch, int width, const char* color = "") {
    std::string line(color);
    for (int i = 0; i < width; ++i) line += ch;
    line += RESET;
    return line;
}

} // namespace ansi
