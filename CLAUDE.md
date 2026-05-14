# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A typing-speed game written in C++ using the Allegro 5 graphics library — sibling project to `../minesweeper/` which shares the same stack and conventions.

## Tech Stack

- **Language:** C++17
- **Graphics:** Allegro 5 (`allegro`, `allegro_image`, `allegro_font`, `allegro_ttf`, `allegro_primitives`)
- **Build:** Premake4 → GNU Make. Output lands in `build/bin/debug/typespeed`.
- **Build command:** `bash create.sh` (generates Makefiles with premake4, then compiles debug)
- **Run command:** `bash run.sh`

## Dependencies

```bash
sudo apt install liballegro5-dev premake4
```

## Architecture

- **`src/main.cpp`** — event loop, phase enum (`MENU/CATEGORY/FILE_PICK/PREVIEW/PLAYING/RESULTS`), wires all modules
- **`src/core/`** — Allegro init/shutdown singleton + settings load/save (`settings.txt`)
- **`src/graphics/`** — display + all screen rendering (`Graphic_Manager`)
- **`src/input/`** — owns `ALLEGRO_EVENT_QUEUE`, keyboard + display sources
- **`src/game/`** — round state machine, char-by-char tracking, strict/lenient error modes
- **`src/textbank/`** — file scanning, paragraph loading, progress.txt tracking
- **`src/stats/`** — WPM calculation, accuracy, session history (history.txt)
- **`src/globalstats/`** — (Plan B) keyboard heatmap, bigram analysis, badges

Core is a singleton (`Core::instance()`). Do not construct it on the stack — Allegro would be double-initialized.

Settings are stored in `settings.txt` (auto-generated, gitignored).

## Data Files

- `data/english/` and `data/spanish/` — prose `.txt` files; paragraphs separated by blank lines. Progress tracked in `data/progress.txt`.
- `data/python/`, `data/cpp/`, `data/latex/`, `data/html/`, `data/javascript/` — code `.txt` files; full file loaded each session.
- `data/progress.txt`, `data/history.txt`, `data/keylog.txt` — auto-generated, gitignored.
- Add your own `.txt` files to any category folder — they appear automatically in the file picker.

## Build System

`premake4.lua` defines the solution. After adding new source directories, add them to `includedirs` in `premake4.lua` and re-run `bash create.sh`.

Typical `premake4.lua` structure mirrors `../minesweeper/premake4.lua`.
