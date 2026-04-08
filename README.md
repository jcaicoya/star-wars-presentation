# StarWarsText

A Qt6/C++ cinematic presentation tool inspired by the Star Wars opening crawl.

Features a complete animated pipeline — intro text, shrinking outlined logo, perspective-scrolling crawl with justified body text, 3D spaceflight between goal stars, hyperspace jump, and a finale recap with sphere-rendered stars and Saturn-ringed planets. Includes a free-flight video game mode and an integrated editor for all content.

## Build

Requirements: Qt 6.7.3+, CMake, C++20 compiler.

```bash
# Linux (clang++)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build -j$(nproc)
./build/StarWarsText
```

On Windows: open in CLion with MSVC, no extra flags needed. See `CLAUDE.md` for cross-platform details.

## Usage

See [OPERATOR.md](OPERATOR.md) for the full user guide: launcher, editors, show controls, and data file format.

## Architecture

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point |
| `src/MainWindow.cpp/.h` | Launcher, editor pages, mode switching, save/load |
| `src/CrawlWindow.cpp/.h` | All rendering and animation (6 phases + hyperspace) |
| `src/CrawlContent.h` | Data structs (`CrawlContent`, `Star`, `StarDefinition`) |
| `src/StarMapWidget.cpp/.h` | 3D cube star map viewer |
| `src/StarsEditorWidget.cpp/.h` | Star list, property form, StarMapWidget integration |
| `src/LineNumberTextEdit.cpp/.h` | Text editor with line numbers and column guide |
| `src/TextIO.cpp/.h` | File I/O, text section parser, JSON star serializer |
| `resources/text.txt` | Crawl content (section-based) |
| `resources/stars.json` | Star/planet definitions (JSON) |
