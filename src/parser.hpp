#pragma once
#include <string>
#include <vector>

enum class BlockType {
    Heading,
    Paragraph,
    CodeBlock,
    BlockQuote,
    UnorderedList,
    OrderedList,
    ListItem,
    HorizontalRule,
};

struct Block {
    BlockType          type    = BlockType::Paragraph;
    int                level   = 0;       // heading level 1-6
    std::string        content;           // raw inline text (or code text)
    std::string        lang;             // code block language tag
    std::vector<Block> children;         // lists → ListItems; blockquote → sub-blocks
};

// Parse markdown text into a flat sequence of Block nodes.
// Blockquote and list children are nested inside the parent block.
std::vector<Block> parse_markdown(const std::string& input);
