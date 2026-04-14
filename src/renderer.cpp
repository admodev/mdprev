#include "renderer.hpp"
#include "ansi.hpp"
#include <sstream>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/ioctl.h>
#  include <unistd.h>
#endif

// ── terminal helpers ──────────────────────────────────────────────────────────

static int terminal_width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
#endif
    return 80;
}

// ── inline renderer ───────────────────────────────────────────────────────────

// Forward declaration (recursive for nested spans).
static std::string render_inline(const std::string& text);

// Try to match a delimited span starting at position i in text.
// marker:   the opening/closing delimiter string
// style:    ANSI style to wrap the inner content
// Returns the rendered span and advances i past it, or returns "" if no match.
static std::string try_span(const std::string& text, size_t& i,
                             const std::string& open_marker,
                             const std::string& close_marker,
                             const char* style1, const char* style2 = nullptr)
{
    const size_t mlen = open_marker.size();
    if (text.compare(i, mlen, open_marker) != 0) return "";
    size_t end = text.find(close_marker, i + mlen);
    if (end == std::string::npos) return "";
    // Reject if inner content starts or ends with a space (CommonMark rule).
    std::string inner = text.substr(i + mlen, end - i - mlen);
    if (inner.empty() || inner.front() == ' ' || inner.back() == ' ') return "";

    std::string out;
    out += style1;
    if (style2) out += style2;
    out += render_inline(inner);
    out += ansi::RESET;
    i = end + close_marker.size();
    return out;
}

static std::string render_inline(const std::string& text) {
    std::string out;
    const size_t n = text.size();
    size_t i = 0;

    while (i < n) {
        char c = text[i];

        // ── backslash escape ──────────────────────────────────────────────
        if (c == '\\' && i + 1 < n) {
            out += text[i + 1];
            i += 2;
            continue;
        }

        // ── bold+italic *** / ___ ─────────────────────────────────────────
        if (c == '*' && i + 2 < n && text[i+1] == '*' && text[i+2] == '*') {
            std::string s = try_span(text, i, "***", "***", ansi::BOLD, ansi::ITALIC);
            if (!s.empty()) { out += s; continue; }
        }

        // ── bold ** / __ ──────────────────────────────────────────────────
        if (c == '*' && i + 1 < n && text[i+1] == '*') {
            std::string s = try_span(text, i, "**", "**", ansi::BOLD);
            if (!s.empty()) { out += s; continue; }
        }
        if (c == '_' && i + 1 < n && text[i+1] == '_') {
            std::string s = try_span(text, i, "__", "__", ansi::BOLD);
            if (!s.empty()) { out += s; continue; }
        }

        // ── italic * / _ ──────────────────────────────────────────────────
        if (c == '*') {
            std::string s = try_span(text, i, "*", "*", ansi::ITALIC);
            if (!s.empty()) { out += s; continue; }
        }
        if (c == '_') {
            std::string s = try_span(text, i, "_", "_", ansi::ITALIC);
            if (!s.empty()) { out += s; continue; }
        }

        // ── strikethrough ~~ ──────────────────────────────────────────────
        if (c == '~' && i + 1 < n && text[i+1] == '~') {
            std::string s = try_span(text, i, "~~", "~~", ansi::STRIKE);
            if (!s.empty()) { out += s; continue; }
        }

        // ── inline code `` ────────────────────────────────────────────────
        if (c == '`') {
            // Support both ` and `` delimiters.
            std::string delim = "`";
            if (i + 1 < n && text[i+1] == '`') delim = "``";
            size_t end = text.find(delim, i + delim.size());
            if (end != std::string::npos) {
                std::string code = text.substr(i + delim.size(), end - i - delim.size());
                out += ansi::BRIGHT_CYAN;
                out += ansi::BG_BLACK;
                out += code;
                out += ansi::RESET;
                i = end + delim.size();
                continue;
            }
        }

        // ── image ![alt](url) ─────────────────────────────────────────────
        if (c == '!' && i + 1 < n && text[i+1] == '[') {
            size_t alt_end = text.find(']', i + 2);
            if (alt_end != std::string::npos && alt_end + 1 < n && text[alt_end+1] == '(') {
                size_t url_end = text.find(')', alt_end + 2);
                if (url_end != std::string::npos) {
                    std::string alt = text.substr(i + 2, alt_end - i - 2);
                    std::string url = text.substr(alt_end + 2, url_end - alt_end - 2);
                    out += ansi::BRIGHT_MAGENTA;
                    out += "[img: " + alt + "]";
                    out += ansi::RESET;
                    out += ansi::DIM;
                    out += " <" + url + ">";
                    out += ansi::RESET;
                    i = url_end + 1;
                    continue;
                }
            }
        }

        // ── link [text](url) ──────────────────────────────────────────────
        if (c == '[') {
            size_t txt_end = text.find(']', i + 1);
            if (txt_end != std::string::npos && txt_end + 1 < n && text[txt_end+1] == '(') {
                size_t url_end = text.find(')', txt_end + 2);
                if (url_end != std::string::npos) {
                    std::string link_text = text.substr(i + 1, txt_end - i - 1);
                    std::string url = text.substr(txt_end + 2, url_end - txt_end - 2);
                    out += ansi::UNDERLINE;
                    out += ansi::BRIGHT_BLUE;
                    out += render_inline(link_text);
                    out += ansi::RESET;
                    out += ansi::DIM;
                    out += " <" + url + ">";
                    out += ansi::RESET;
                    i = url_end + 1;
                    continue;
                }
            }
        }

        out += c;
        ++i;
    }

    return out;
}

// ── block renderers ───────────────────────────────────────────────────────────

static std::string make_indent(int depth) {
    return std::string(static_cast<size_t>(depth) * 2, ' ');
}

static std::string render_heading(const Block& b, int /*depth*/) {
    const int tw = terminal_width();
    std::string text = render_inline(b.content);
    std::string out = "\n";

    switch (b.level) {
        case 1:
            out += ansi::BOLD;
            out += ansi::BRIGHT_CYAN;
            out += text;
            out += ansi::RESET;
            out += '\n';
            out += ansi::hline('=', std::min(tw, ansi::visible_length(text) + 2), ansi::BRIGHT_CYAN);
            out += '\n';
            break;
        case 2:
            out += ansi::BOLD;
            out += ansi::BRIGHT_BLUE;
            out += text;
            out += ansi::RESET;
            out += '\n';
            out += ansi::hline('-', std::min(tw, ansi::visible_length(text) + 2), ansi::BLUE);
            out += '\n';
            break;
        case 3:
            out += ansi::BOLD;
            out += ansi::BRIGHT_YELLOW;
            out += text;
            out += ansi::RESET;
            out += '\n';
            break;
        case 4:
            out += ansi::BOLD;
            out += ansi::YELLOW;
            out += text;
            out += ansi::RESET;
            out += '\n';
            break;
        case 5:
            out += ansi::BOLD;
            out += ansi::WHITE;
            out += text;
            out += ansi::RESET;
            out += '\n';
            break;
        default:  // 6
            out += ansi::DIM;
            out += ansi::WHITE;
            out += text;
            out += ansi::RESET;
            out += '\n';
            break;
    }

    return out;
}

static std::string render_paragraph(const Block& b, int depth) {
    std::string indent = make_indent(depth);
    std::string text   = render_inline(b.content);
    return indent + text + "\n\n";
}

static std::string render_code_block(const Block& b, int depth) {
    const int   tw     = terminal_width();
    std::string indent = make_indent(depth);
    std::string out;

    // Header bar with language tag
    std::string header = (b.lang.empty() ? " code " : " " + b.lang + " ");
    out += indent;
    out += ansi::BG_BLACK;
    out += ansi::BRIGHT_BLACK;
    out += header;
    // Pad to a fixed width
    int pad = std::max(0, tw - (int)indent.size() - (int)header.size());
    out += std::string(static_cast<size_t>(pad), ' ');
    out += ansi::RESET;
    out += '\n';

    // Code lines
    std::istringstream ss(b.content);
    std::string line;
    while (std::getline(ss, line)) {
        out += indent;
        out += ansi::BG_BLACK;
        out += ansi::BRIGHT_GREEN;
        out += ' ';
        out += line;
        // Pad line to terminal width so background fills the row.
        int vis = (int)indent.size() + 1 + ansi::visible_length(line);
        int fill = std::max(0, tw - vis);
        out += std::string(static_cast<size_t>(fill), ' ');
        out += ansi::RESET;
        out += '\n';
    }

    // Footer bar
    out += indent;
    out += ansi::BG_BLACK;
    out += ansi::BRIGHT_BLACK;
    int footer_width = std::max(0, tw - (int)indent.size());
    out += std::string(static_cast<size_t>(footer_width), ' ');
    out += ansi::RESET;
    out += "\n\n";

    return out;
}

// Forward declaration needed for blockquote recursion.
std::string render_blocks(const std::vector<Block>& blocks, int depth);

static std::string render_blockquote(const Block& b, int depth) {
    // Render children at depth+1, then prefix each line with a vertical bar.
    std::string inner   = render_blocks(b.children, depth + 1);
    std::string prefix  = std::string(ansi::BRIGHT_BLACK) + "│ " + ansi::RESET;
    std::string out;

    std::istringstream ss(inner);
    std::string line;
    while (std::getline(ss, line)) {
        out += prefix + line + '\n';
    }
    out += '\n';
    return out;
}

static std::string render_list(const Block& b, int depth) {
    std::string indent = make_indent(depth);
    bool ordered = (b.type == BlockType::OrderedList);
    std::string out;
    int idx = 1;

    for (const auto& item : b.children) {
        std::string bullet;
        if (ordered) {
            bullet = std::to_string(idx++) + ". ";
        } else {
            bullet = "• ";
        }

        std::string text = render_inline(item.content);

        out += indent;
        out += ansi::BRIGHT_WHITE;
        out += bullet;
        out += ansi::RESET;
        out += text;
        out += '\n';
    }
    out += '\n';
    return out;
}

static std::string render_hr(int depth) {
    const int   tw     = terminal_width();
    std::string indent = make_indent(depth);
    int         w      = std::max(0, tw - (int)indent.size());
    std::string out    = "\n" + indent;
    out += ansi::DIM;
    out += ansi::BRIGHT_BLACK;
    for (int i = 0; i < w; ++i) out += (i % 2 == 0 ? '-' : ' ');
    out += ansi::RESET;
    out += "\n\n";
    return out;
}

// ── public interface ──────────────────────────────────────────────────────────

std::string render_blocks(const std::vector<Block>& blocks, int depth) {
    std::string out;
    for (const auto& b : blocks) {
        switch (b.type) {
            case BlockType::Heading:        out += render_heading(b, depth);    break;
            case BlockType::Paragraph:      out += render_paragraph(b, depth);  break;
            case BlockType::CodeBlock:      out += render_code_block(b, depth); break;
            case BlockType::BlockQuote:     out += render_blockquote(b, depth); break;
            case BlockType::UnorderedList:
            case BlockType::OrderedList:    out += render_list(b, depth);       break;
            case BlockType::HorizontalRule: out += render_hr(depth);            break;
            default: break;
        }
    }
    return out;
}
