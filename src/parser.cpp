#include "parser.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

// ── helpers ──────────────────────────────────────────────────────────────────

static std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        // Strip trailing \r for Windows-style line endings
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

static bool is_blank(const std::string& line) {
    return line.find_first_not_of(" \t") == std::string::npos;
}

static bool is_hr(const std::string& line) {
    // Must be 3+ identical chars (-, *, _) optionally separated by spaces.
    std::string s;
    for (char c : line) if (c != ' ' && c != '\t') s += c;
    if (s.size() < 3) return false;
    char ch = s[0];
    if (ch != '-' && ch != '*' && ch != '_') return false;
    for (char c : s) if (c != ch) return false;
    return true;
}

// Returns heading level (1-6) or 0 if not a heading.
static int heading_level(const std::string& line) {
    int lv = 0;
    while (lv < (int)line.size() && line[lv] == '#') ++lv;
    if (lv < 1 || lv > 6) return 0;
    if (lv >= (int)line.size() || (line[lv] != ' ' && line[lv] != '\t')) return 0;
    return lv;
}

static bool is_unordered_item(const std::string& line) {
    if (line.size() < 2) return false;
    char c = line[0];
    return (c == '-' || c == '*' || c == '+') && (line[1] == ' ' || line[1] == '\t');
}

static bool is_ordered_item(const std::string& line) {
    size_t i = 0;
    while (i < line.size() && std::isdigit((unsigned char)line[i])) ++i;
    if (i == 0 || i + 1 >= line.size()) return false;
    return line[i] == '.' && (line[i+1] == ' ' || line[i+1] == '\t');
}

static bool is_blockquote(const std::string& line) {
    return !line.empty() && line[0] == '>';
}

// Detects opening/closing ``` fence; sets lang on opening.
static bool is_code_fence(const std::string& line, std::string& lang) {
    size_t i = 0;
    while (i < line.size() && line[i] == ' ') ++i;   // up to 3 leading spaces
    if (i > 3) return false;
    if (line.size() < i + 3) return false;
    if (line[i] != '`' && line[i] != '~') return false;
    char fence_ch = line[i];
    size_t j = i;
    while (j < line.size() && line[j] == fence_ch) ++j;
    if (j - i < 3) return false;
    std::string tag = line.substr(j);
    // trim whitespace
    while (!tag.empty() && std::isspace((unsigned char)tag.back())) tag.pop_back();
    while (!tag.empty() && std::isspace((unsigned char)tag.front())) tag.erase(tag.begin());
    lang = tag;
    return true;
}

// Strip leading '> ' prefix from a blockquote line.
static std::string strip_bq_prefix(const std::string& line) {
    if (line.empty() || line[0] != '>') return line;
    if (line.size() > 1 && line[1] == ' ') return line.substr(2);
    return line.substr(1);
}

// Extract list item content (strip marker prefix).
static std::string list_item_content(const std::string& line) {
    size_t i = 0;
    while (i < line.size() && std::isdigit((unsigned char)line[i])) ++i;
    if (i > 0 && i < line.size() && line[i] == '.') {
        // ordered: skip "N. "
        i += 2;
    } else {
        // unordered: skip "- "
        i = 2;
    }
    return i < line.size() ? line.substr(i) : std::string{};
}

// ── flushing helpers ──────────────────────────────────────────────────────────

static void flush_para(std::vector<Block>& out, std::string& para) {
    if (!para.empty()) {
        out.push_back({BlockType::Paragraph, 0, para});
        para.clear();
    }
}

static void flush_list(std::vector<Block>& out, Block& list, bool& in_ul, bool& in_ol) {
    if (in_ul || in_ol) {
        out.push_back(list);
        list = {};
        in_ul = in_ol = false;
    }
}

// ── main parser ───────────────────────────────────────────────────────────────

std::vector<Block> parse_markdown(const std::string& input) {
    std::vector<Block> blocks;
    auto lines = split_lines(input);
    const size_t n = lines.size();

    bool        in_code = false;
    std::string code_content;
    std::string code_lang;
    std::string code_fence_char;   // remembers whether ``` or ~~~

    std::string para;

    bool  in_ul = false;
    bool  in_ol = false;
    Block cur_list{};

    auto flush_all = [&]() {
        flush_para(blocks, para);
        flush_list(blocks, cur_list, in_ul, in_ol);
    };

    for (size_t i = 0; i < n; ++i) {
        const std::string& line = lines[i];

        // ── inside fenced code block ──────────────────────────────────────
        if (in_code) {
            std::string dummy;
            if (is_code_fence(line, dummy)) {
                in_code = false;
                if (!code_content.empty() && code_content.back() == '\n')
                    code_content.pop_back();
                blocks.push_back({BlockType::CodeBlock, 0, code_content, code_lang});
                code_content.clear();
                code_lang.clear();
            } else {
                code_content += line + "\n";
            }
            continue;
        }

        // ── blank line ────────────────────────────────────────────────────
        if (is_blank(line)) {
            flush_all();
            continue;
        }

        // ── fenced code block start ───────────────────────────────────────
        {
            std::string lang;
            if (is_code_fence(line, lang)) {
                flush_all();
                in_code = true;
                code_lang = lang;
                continue;
            }
        }

        // ── ATX heading (#…) ──────────────────────────────────────────────
        {
            int lv = heading_level(line);
            if (lv > 0) {
                flush_all();
                std::string text = line.substr(lv + 1);
                // strip optional trailing #s
                while (!text.empty() && (text.back() == '#' || text.back() == ' '))
                    text.pop_back();
                blocks.push_back({BlockType::Heading, lv, text});
                continue;
            }
        }

        // ── setext heading (=== or ---) ───────────────────────────────────
        if (!para.empty()) {
            std::string stripped;
            for (char c : line) if (c != ' ' && c != '\t') stripped += c;
            if (!stripped.empty()) {
                bool all_eq   = std::all_of(stripped.begin(), stripped.end(), [](char c){ return c == '='; });
                bool all_dash = std::all_of(stripped.begin(), stripped.end(), [](char c){ return c == '-'; });
                if (all_eq || all_dash) {
                    flush_list(blocks, cur_list, in_ul, in_ol);
                    int lv = all_eq ? 1 : 2;
                    blocks.push_back({BlockType::Heading, lv, para});
                    para.clear();
                    continue;
                }
            }
        }

        // ── horizontal rule (before list check; --- could be setext above) ─
        if (para.empty() && is_hr(line)) {
            flush_all();
            blocks.push_back({BlockType::HorizontalRule});
            continue;
        }

        // ── blockquote ────────────────────────────────────────────────────
        if (is_blockquote(line)) {
            flush_all();
            // Collect all consecutive blockquote lines.
            std::string bq_raw;
            while (i < n && (is_blockquote(lines[i]) ||
                              (!is_blank(lines[i]) && i > 0 && is_blockquote(lines[i-1])))) {
                if (is_blockquote(lines[i]))
                    bq_raw += strip_bq_prefix(lines[i]) + "\n";
                else if (!is_blank(lines[i]))
                    bq_raw += lines[i] + "\n";   // lazy continuation
                else
                    break;
                ++i;
            }
            --i;  // compensate outer loop increment
            if (!bq_raw.empty() && bq_raw.back() == '\n') bq_raw.pop_back();
            Block bq{BlockType::BlockQuote};
            bq.children = parse_markdown(bq_raw);
            blocks.push_back(bq);
            continue;
        }

        // ── unordered list ────────────────────────────────────────────────
        if (is_unordered_item(line)) {
            flush_para(blocks, para);
            if (!in_ul) {
                flush_list(blocks, cur_list, in_ul, in_ol);
                in_ul = true;
                cur_list = {BlockType::UnorderedList};
            }
            cur_list.children.push_back({BlockType::ListItem, 0, list_item_content(line)});
            continue;
        }

        // ── ordered list ──────────────────────────────────────────────────
        if (is_ordered_item(line)) {
            flush_para(blocks, para);
            if (!in_ol) {
                flush_list(blocks, cur_list, in_ul, in_ol);
                in_ol = true;
                cur_list = {BlockType::OrderedList};
            }
            cur_list.children.push_back({BlockType::ListItem, 0, list_item_content(line)});
            continue;
        }

        // ── list continuation (indented) ──────────────────────────────────
        if ((in_ul || in_ol) && !cur_list.children.empty()) {
            // A line indented ≥2 spaces/1 tab continues the last list item.
            bool is_continuation = (line.size() >= 2 && (line[0] == ' ' || line[0] == '\t'));
            if (is_continuation) {
                std::string cont = (line[0] == '\t') ? line.substr(1) : line.substr(2);
                cur_list.children.back().content += " " + cont;
                continue;
            }
            // Non-indented, non-list line ends the list.
            flush_list(blocks, cur_list, in_ul, in_ol);
        }

        // ── paragraph ─────────────────────────────────────────────────────
        if (para.empty())
            para = line;
        else
            para += ' ' + line;
    }

    flush_all();

    // Unclosed fenced code block: emit what we have.
    if (in_code) {
        if (!code_content.empty() && code_content.back() == '\n')
            code_content.pop_back();
        blocks.push_back({BlockType::CodeBlock, 0, code_content, code_lang});
    }

    return blocks;
}
