# mdprev

A fast, dependency-free terminal Markdown previewer written in C++17.

Renders Markdown to ANSI-coloured terminal output directly — no browser,
no GUI, no external runtime.

## Features

- ATX headings (`#` … `######`) with decorative underlines for H1/H2
- Setext headings (`===` / `---`)
- **Bold**, *italic*, ***bold+italic***, ~~strikethrough~~, `inline code`
- Fenced code blocks with language label and full-width background
- Blockquotes (recursive, supports nested formatting)
- Unordered and ordered lists
- Links `[text](url)` and images `![alt](url)`
- Horizontal rules
- Backslash escapes
- `--no-color` flag for plain-text output
- Reads from a file or stdin

## Installation

### Linux / macOS

```bash
git clone https://github.com/youruser/mdprev
cd mdprev
make install          # installs to /usr/local/bin (sudo if needed)
```

Override the install directory:

```bash
make install INSTALL_DIR=$HOME/.local/bin
```

### Windows (MSYS2 / MinGW / WSL)

Under WSL the Linux path applies.  For native Windows with MSYS2 or MinGW:

```bash
make install INSTALL_DIR="C:/Windows/System32"
```

The binary is placed in `INSTALL_DIR` as `mdprev.exe`.

### Uninstall

```bash
make uninstall
```

## Usage

```
mdprev [options] [file]

  file           Markdown file to render (reads stdin if omitted)

Options:
  -h, --help     Show help
  -v, --version  Show version
      --no-color Disable ANSI colour output
```

**Render a file:**

```bash
mdprev README.md
```

**Pipe stdin:**

```bash
cat NOTES.md | mdprev
echo "# Hello **world**" | mdprev
```

**Pipe through a pager:**

```bash
mdprev docs/guide.md | less -R
```

**Strip colours (plain text):**

```bash
mdprev --no-color README.md
```

## Building from source

Requirements: a C++17-capable compiler (`g++` ≥ 7 or `clang++` ≥ 5).

```bash
make          # builds ./mdprev
make clean    # removes the binary
```

Override the compiler:

```bash
make CXX=clang++
```

## Project structure

```
mdprev/
├── src/
│   ├── main.cpp        Entry point — argument parsing, I/O
│   ├── parser.hpp/cpp  Block-level Markdown parser → AST
│   ├── renderer.hpp/cpp ANSI renderer + inline span parser
│   └── ansi.hpp        ANSI escape-code constants and helpers
├── Makefile
├── README.md
└── CHANGELOG.md
```

## Architecture

| Component | Role |
|-----------|------|
| `parser`  | Converts raw Markdown text into a `vector<Block>` AST via a single left-to-right line scan with a small state machine. |
| `renderer`| Walks the AST and emits ANSI escape sequences. Inline formatting (bold, italic, links, …) is handled by a recursive scan-forward function inside the renderer. |
| `ansi`    | Thin header of `constexpr` escape-code strings and two utility functions (`visible_length`, `hline`). |
| `main`    | Reads source, orchestrates parse → render, handles flags. |

## Licence

MIT — see source headers.
