# Fullscreen Toggle Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add F11 fullscreen toggle (windowed ↔ borderless fullscreen) that persists across sessions via `settings.txt`.

**Architecture:** `Graphic_Manager` gets a `toggle_fullscreen()` method and reads the startup fullscreen setting from `Core` during construction. `main.cpp` handles `ALLEGRO_KEY_F11` on `KEY_DOWN` in all phases, calls `toggle_fullscreen()`, and saves the new state. No layout changes — rendering already uses `WIN_W`/`WIN_H` constants and Allegro scales the drawing surface automatically for `ALLEGRO_FULLSCREEN_WINDOW`.

**Tech Stack:** C++17, Allegro 5 (`al_set_new_display_flags`, `al_toggle_display_flag`, `ALLEGRO_FULLSCREEN_WINDOW`).

---

## File Map

| File | Change |
|---|---|
| `src/graphics/graphicMgr.hpp` | Add `toggle_fullscreen()`, `is_fullscreen()`, `fullscreen_` member |
| `src/graphics/graphicMgr.cpp` | Read startup flag from `Core`, implement toggle |
| `src/main.cpp` | Handle `ALLEGRO_KEY_F11` on `KEY_DOWN` in all phases |

---

### Task 1: Add fullscreen support to Graphic_Manager

**Files:**
- Modify: `src/graphics/graphicMgr.hpp`
- Modify: `src/graphics/graphicMgr.cpp`

- [ ] **Step 1: Add `fullscreen_` member and two public methods to `graphicMgr.hpp`**

Replace the `private:` block:

```cpp
    // ── Fullscreen ────────────────────────────────────────────────────────────
    void toggle_fullscreen();
    bool is_fullscreen() const { return fullscreen_; }

private:
    ALLEGRO_DISPLAY* display_;
    ALLEGRO_FONT*    font_ui_;
    ALLEGRO_FONT*    font_mono_;
    bool             fullscreen_ = false;

    void draw_toolbar(double elapsed_sec, double wpm, int time_remaining_sec);
    void draw_button(float x, float y, float w, float h,
                     const std::string& label, bool highlighted = false);
    void draw_passage(const Game& game, float x, float y,
                      float max_w, bool cursor_visible);
```

- [ ] **Step 2: Read startup fullscreen flag and set display flag in `Graphic_Manager` constructor**

The constructor is in `src/graphics/graphicMgr.cpp`. Replace:

```cpp
Graphic_Manager::Graphic_Manager() {
    display_ = al_create_display(WIN_W, WIN_H);
    if (!display_) std::exit(1);
    al_set_window_title(display_, "Typespeed");
```

With:

```cpp
Graphic_Manager::Graphic_Manager() {
    // Read fullscreen preference before creating the display
    Core& core = Core::instance();
    fullscreen_ = core.get_bool("fullscreen", false);
    if (fullscreen_)
        al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);

    display_ = al_create_display(WIN_W, WIN_H);
    if (!display_) std::exit(1);
    al_set_window_title(display_, "Typespeed");
```

Also add `#include "core.hpp"` at the top of `graphicMgr.cpp` (after the existing includes):

```cpp
#include "graphicMgr.hpp"
#include "core.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
```

- [ ] **Step 3: Implement `toggle_fullscreen()` in `graphicMgr.cpp`**

Add this method just before the destructor:

```cpp
void Graphic_Manager::toggle_fullscreen() {
    fullscreen_ = !fullscreen_;
    al_toggle_display_flag(display_, ALLEGRO_FULLSCREEN_WINDOW, fullscreen_);
}
```

- [ ] **Step 4: Build and verify**

```bash
bash create.sh 2>&1 | grep -E "error:|warning:"
```

Expected: no output (zero errors, zero warnings).

- [ ] **Step 5: Commit**

```bash
git add src/graphics/graphicMgr.hpp src/graphics/graphicMgr.cpp
git commit -m "feat: Graphic_Manager — fullscreen toggle and startup flag"
```

---

### Task 2: Wire F11 in main.cpp and persist setting

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Handle `ALLEGRO_KEY_F11` on `KEY_DOWN` in the event loop**

In `src/main.cpp`, the keyboard handler currently only handles `ALLEGRO_EVENT_KEY_CHAR`. F11 should use `ALLEGRO_EVENT_KEY_DOWN` so it fires once per press (not per repeat). Add this block just before the existing `if (ev.type == ALLEGRO_EVENT_KEY_CHAR)` check:

```cpp
        // ── F11 fullscreen toggle (all phases) ────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN &&
            ev.keyboard.keycode == ALLEGRO_KEY_F11) {
            gfx.toggle_fullscreen();
            core.set_bool("fullscreen", gfx.is_fullscreen());
            core.save_settings();
        }
```

- [ ] **Step 2: Build and verify**

```bash
bash create.sh 2>&1 | grep -E "error:|warning:"
```

Expected: no output.

- [ ] **Step 3: Smoke test**

```bash
bash run.sh
```

- Launch the game — window opens at `900×600`.
- Press F11 — window goes borderless fullscreen.
- Press F11 again — returns to `900×600` windowed.
- Close the game. Reopen — if you closed while fullscreen, it should reopen fullscreen.
- Verify F11 works from every phase: MENU, CATEGORY, FILE_PICK, PREVIEW, PLAYING, RESULTS.

- [ ] **Step 4: Commit and push**

```bash
git add src/main.cpp
git commit -m "feat: F11 fullscreen toggle — persists to settings.txt"
git push
```

---

## Self-Review

**Spec coverage:**
- F11 toggles windowed ↔ borderless fullscreen ✓ (Task 2)
- Works from all phases ✓ (KEY_DOWN fires globally before phase checks)
- Persists to `settings.txt` ✓ (Task 2, `core.set_bool` + `save_settings`)
- Startup respects saved setting ✓ (Task 1, reads `Core` before `al_create_display`)
- Maximized window left to OS ✓ (no code touches that state)

**Placeholder scan:** None.

**Type consistency:** `toggle_fullscreen()` and `is_fullscreen()` match across `.hpp` and `.cpp`. `core.get_bool("fullscreen", false)` / `core.set_bool("fullscreen", ...)` use the same key string.
