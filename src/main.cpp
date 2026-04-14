#include "parser.hpp"
#include "renderer.hpp"
#include "ansi.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#define MDPREV_VERSION "1.0.0"

static void print_usage(const char* prog) {
    std::fprintf(stderr,
        "Usage: %s [options] [file]\n"
        "\n"
        "  file           Markdown file to render (reads stdin if omitted)\n"
        "\n"
        "Options:\n"
        "  -h, --help     Show this help message\n"
        "  -v, --version  Show version\n"
        "      --no-color Disable ANSI colour output\n"
        "\n",
        prog);
}

static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "error: cannot open '%s'\n", path.c_str());
        std::exit(1);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string read_stdin() {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return ss.str();
}

// Strip all ANSI escape sequences from a string (for --no-color).
static std::string strip_ansi(const std::string& s) {
    std::string out;
    bool in_esc = false;
    for (char c : s) {
        if (c == '\033') { in_esc = true; continue; }
        if (in_esc)      { if (c == 'm') in_esc = false; continue; }
        out += c;
    }
    return out;
}

int main(int argc, char* argv[]) {
    bool        no_color = false;
    std::string input_file;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strcmp(arg, "-h") == 0 || std::strcmp(arg, "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (std::strcmp(arg, "-v") == 0 || std::strcmp(arg, "--version") == 0) {
            std::printf("mdprev %s\n", MDPREV_VERSION);
            return 0;
        }
        if (std::strcmp(arg, "--no-color") == 0) {
            no_color = true;
            continue;
        }
        if (arg[0] == '-') {
            std::fprintf(stderr, "error: unknown option '%s'\n", arg);
            print_usage(argv[0]);
            return 1;
        }
        if (!input_file.empty()) {
            std::fprintf(stderr, "error: too many arguments\n");
            print_usage(argv[0]);
            return 1;
        }
        input_file = arg;
    }

    std::string source = input_file.empty() ? read_stdin() : read_file(input_file);

    auto blocks   = parse_markdown(source);
    auto rendered = render_blocks(blocks);

    if (no_color)
        rendered = strip_ansi(rendered);

    std::cout << rendered;
    return 0;
}
