#pragma once
#include "parser.hpp"
#include <string>

// Render a list of blocks to an ANSI-coloured terminal string.
// depth controls blockquote / list nesting indentation.
std::string render_blocks(const std::vector<Block>& blocks, int depth = 0);
