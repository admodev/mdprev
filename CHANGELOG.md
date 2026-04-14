# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-04-14

### Added

- ATX headings (`#`–`######`) with ANSI colour levels; H1 and H2 include
  decorative underlines scaled to heading text width.
- Setext headings (`===` and `---` underlines).
- Fenced code blocks (`` ``` `` and `~~~`) with language label, full-width
  background shading, and green-on-black code lines.
- Inline formatting: **bold** (`**`/`__`), *italic* (`*`/`_`),
  ***bold+italic*** (`***`), ~~strikethrough~~ (`~~`), `` `inline code` ``,
  backslash escapes.
- Links `[text](url)` rendered with underline + dim URL suffix.
- Images `![alt](url)` rendered as `[img: alt] <url>`.
- Blockquotes (`>`) with vertical-bar prefix; content is recursively parsed so
  nested formatting works correctly.
- Unordered lists (`-`, `*`, `+`) with bullet `•`.
- Ordered lists (`1.`, `2.`, …) with number prefix.
- Horizontal rules (`---`, `***`, `___`).
- `--no-color` flag strips ANSI escape sequences for plain-text output.
- `-h`/`--help` usage message and `-v`/`--version` flag.
- Reads from a file path argument or from stdin.
- Makefile with `build`, `install`, `uninstall`, and `clean` targets; platform
  detection for Linux, macOS, and Windows (MSYS2/MinGW/WSL).

[Unreleased]: https://github.com/youruser/mdprev/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/youruser/mdprev/releases/tag/v1.0.0
