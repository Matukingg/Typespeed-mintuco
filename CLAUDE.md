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

## Architecture Conventions (from sibling project)

- **`src/main.cpp`** — game loop and event dispatch
- **`src/core/`** — Allegro init/shutdown singleton + settings load/save (`settings.txt`)
- **`src/graphics/`** — display creation and rendering manager
- **`src/input/`** — owns `ALLEGRO_EVENT_QUEUE`, registers sources
- **`src/utils/`** — game logic (word lists, scoring, timing)
- **`data/`** — image/font assets
- **`assets/`** — source art files

Core is a singleton (`Core::instance()`). Do not construct it on the stack — Allegro would be double-initialized.

Settings are stored in `settings.txt` (auto-generated, gitignored).

## Build System

`premake4.lua` defines the solution. After adding new source directories, add them to `includedirs` in `premake4.lua` and re-run `bash create.sh`.

Typical `premake4.lua` structure mirrors `../minesweeper/premake4.lua`.
