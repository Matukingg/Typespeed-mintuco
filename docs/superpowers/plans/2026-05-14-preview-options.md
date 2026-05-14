# Preview Screen Options Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Redo Last, Skip, Restart, and Jump To paragraph navigation buttons to the Preview screen for prose files, with a digit-input overlay for Jump To.

**Architecture:** `TextBank` gains four progress-manipulation methods; `Graphic_Manager::render_preview` gains a prose-only button row and a `render_jump_overlay` method; `main.cpp` handles new button codes (40=redo, 41=skip, 42=restart, 43=jump) and a new `JUMP` sub-phase for digit input. Code categories show only the Start/Back buttons unchanged.

**Tech Stack:** C++17, Allegro 5.

---

## File Map

| File | Change |
|---|---|
| `src/textbank/textbank.hpp` | Add `reset_progress`, `redo_last`, `skip_paragraph`, `jump_to_paragraph` |
| `src/textbank/textbank.cpp` | Implement the four new methods |
| `src/graphics/graphicMgr.hpp` | Add `preview_redo_click`, `preview_skip_click`, `preview_restart_click`, `preview_jump_click`, `render_jump_overlay`, `jump_confirm_click`, `jump_cancel_click` |
| `src/graphics/graphicMgr.cpp` | Update `render_preview` for prose button row; add `render_jump_overlay` and hit-test methods |
| `src/main.cpp` | Handle new preview button codes + JUMP sub-phase keyboard input |

---

### Task 1: TextBank — progress navigation methods

**Files:**
- Modify: `src/textbank/textbank.hpp`
- Modify: `src/textbank/textbank.cpp`

- [ ] **Step 1: Add four new public methods to `textbank.hpp`**

Inside the `public:` section of `TextBank`, after `save_progress`:

```cpp
    // Progress navigation (prose only — no-op for code)
    void reset_progress   (const FileInfo& fi);          // go back to paragraph 0
    void redo_last        (const FileInfo& fi);          // go back one paragraph
    void skip_paragraph   (const FileInfo& fi, int total); // advance one paragraph
    void jump_to_paragraph(const FileInfo& fi, int idx); // jump to specific 0-based index
```

- [ ] **Step 2: Implement the four methods in `textbank.cpp`**

Add at the end of the file, before the closing of the translation unit:

```cpp
void TextBank::reset_progress(const FileInfo& fi) {
    progress_[fi.full_path] = 0;
    save_progress();
}

void TextBank::redo_last(const FileInfo& fi) {
    int& idx = progress_[fi.full_path];
    // idx points to the NEXT paragraph to show.
    // To redo, go back two (one to undo the advance, one more to re-show previous).
    // But if idx == 0 or idx == -1 (completed), clamp to 0.
    if (idx < 0) {
        // completed — redo the last paragraph
        auto it = para_cache_.find(fi.full_path);
        int total = (it != para_cache_.end()) ? (int)it->second.size() : fi.paragraph_count;
        idx = std::max(0, total - 1);
    } else {
        idx = std::max(0, idx - 1);
    }
    save_progress();
}

void TextBank::skip_paragraph(const FileInfo& fi, int total) {
    int& idx = progress_[fi.full_path];
    if (idx < 0) return; // already completed, nothing to skip
    idx++;
    if (idx >= total) idx = -1; // mark completed if we skip the last one
    save_progress();
}

void TextBank::jump_to_paragraph(const FileInfo& fi, int target_idx) {
    auto it = para_cache_.find(fi.full_path);
    int total = (it != para_cache_.end()) ? (int)it->second.size() : fi.paragraph_count;
    // Clamp to valid range
    if (target_idx < 0) target_idx = 0;
    if (target_idx >= total) target_idx = total - 1;
    progress_[fi.full_path] = target_idx;
    save_progress();
}
```

Also add `#include <algorithm>` at the top of `textbank.cpp` if not already present (it is — no change needed).

- [ ] **Step 3: Write headless unit tests**

```bash
cat > /tmp/test_textbank_nav.cpp << 'EOF'
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "/home/matuco/Documents/typespeed/src/textbank/textbank.hpp"
#include "/home/matuco/Documents/typespeed/src/textbank/textbank.cpp"

FileInfo make_fi(const std::string& path) {
    FileInfo fi;
    fi.full_path = path;
    fi.filename  = std::filesystem::path(path).filename().string();
    fi.word_count = 10; fi.line_count = 5; fi.paragraph_count = 4;
    return fi;
}

int main() {
    namespace fs = std::filesystem;
    // Set up a temp prose file with 4 paragraphs
    fs::create_directories("/tmp/tbtest/data/english");
    {
        std::ofstream f("/tmp/tbtest/data/english/test.txt");
        f << "para one\n\npara two\n\npara three\n\npara four\n";
    }
    // Change cwd to /tmp/tbtest so data/ paths resolve
    fs::current_path("/tmp/tbtest");

    TextBank tb;
    tb.scan(Category::English);
    assert(!tb.files().empty());
    FileInfo fi = tb.files()[0];

    // Prime the cache by loading 2 paragraphs
    tb.load_passage(fi, Category::English); // idx 0 → 1
    tb.load_passage(fi, Category::English); // idx 1 → 2
    assert(tb.current_paragraph(fi) == 2);

    // redo_last: goes back to 1
    tb.redo_last(fi);
    assert(tb.current_paragraph(fi) == 1);
    std::cout << "PASS: redo_last\n";

    // skip: goes to 2
    tb.skip_paragraph(fi, tb.total_paragraphs(fi));
    assert(tb.current_paragraph(fi) == 2);
    std::cout << "PASS: skip_paragraph\n";

    // jump_to: go to paragraph 3 (0-based)
    tb.jump_to_paragraph(fi, 3);
    assert(tb.current_paragraph(fi) == 3);
    std::cout << "PASS: jump_to_paragraph\n";

    // reset: back to 0
    tb.reset_progress(fi);
    assert(tb.current_paragraph(fi) == 0);
    std::cout << "PASS: reset_progress\n";

    // jump clamps to valid range
    tb.jump_to_paragraph(fi, 99);
    assert(tb.current_paragraph(fi) == 3); // total=4, max index=3
    std::cout << "PASS: jump_to clamps high\n";

    tb.jump_to_paragraph(fi, -5);
    assert(tb.current_paragraph(fi) == 0);
    std::cout << "PASS: jump_to clamps low\n";

    // skip past last paragraph marks completed (-1 internally), current_paragraph returns 0
    tb.jump_to_paragraph(fi, 3);
    tb.skip_paragraph(fi, 4); // skip last → completed
    assert(tb.current_paragraph(fi) == 0); // completed returns 0
    std::cout << "PASS: skip_paragraph marks completed\n";

    // redo from completed state goes to last paragraph
    tb.redo_last(fi);
    assert(tb.current_paragraph(fi) == 3);
    std::cout << "PASS: redo_last from completed\n";

    std::cout << "\nAll TextBank nav tests passed.\n";
    return 0;
}
EOF
g++ -std=c++17 -g -fsanitize=address,undefined \
  -I /home/matuco/Documents/typespeed/src \
  -I /home/matuco/Documents/typespeed/src/textbank \
  /tmp/test_textbank_nav.cpp -o /tmp/test_textbank_nav 2>&1
```

Expected: compiles cleanly.

- [ ] **Step 4: Run tests**

```bash
/tmp/test_textbank_nav 2>&1
```

Expected output:
```
PASS: redo_last
PASS: skip_paragraph
PASS: jump_to_paragraph
PASS: reset_progress
PASS: jump_to clamps high
PASS: jump_to clamps low
PASS: skip_paragraph marks completed
PASS: redo_last from completed

All TextBank nav tests passed.
```

- [ ] **Step 5: Build the game and verify**

```bash
cd /home/matuco/Documents/typespeed && bash create.sh 2>&1 | grep -E "error:|warning:"
```

Expected: no output.

- [ ] **Step 6: Commit**

```bash
git add src/textbank/
git commit -m "feat: TextBank — redo, skip, restart, jump paragraph navigation"
```

---

### Task 2: Graphic_Manager — prose preview button row + jump overlay

**Files:**
- Modify: `src/graphics/graphicMgr.hpp`
- Modify: `src/graphics/graphicMgr.cpp`

- [ ] **Step 1: Add new declarations to `graphicMgr.hpp`**

Replace the `// ── Preview` section:

```cpp
    // ── Preview ───────────────────────────────────────────────────────────────
    // current_para and total_para are 0-based index and total count.
    // is_prose controls whether nav buttons are shown.
    void render_preview(const FileInfo& fi, Category cat,
                        int current_para, int total_para);
    bool preview_confirm_click (int x, int y) const;
    bool preview_back_click    (int x, int y) const;
    // Prose navigation buttons (return false for code categories)
    bool preview_redo_click    (int x, int y) const;
    bool preview_skip_click    (int x, int y) const;
    bool preview_restart_click (int x, int y) const;
    bool preview_jump_click    (int x, int y) const;

    // Jump-to overlay — shown over the preview screen
    void render_jump_overlay(int current_para, int total_para,
                             const std::string& input);
    bool jump_confirm_click(int x, int y) const;
    bool jump_cancel_click (int x, int y) const;
```

- [ ] **Step 2: Update `render_preview` in `graphicMgr.cpp`**

Replace the existing `render_preview` implementation:

```cpp
void Graphic_Manager::render_preview(const FileInfo& fi, Category cat,
                                      int current_para, int total_para) {
    al_clear_to_color(COL_BG);
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 80, ALLEGRO_ALIGN_CENTRE,
                 fi.filename.c_str());
    char buf[128];
    if (TextBank::is_prose(cat) && total_para > 0)
        std::snprintf(buf, sizeof(buf), "Paragraph %d of %d",
                      current_para + 1, total_para);
    else
        std::strcpy(buf, "Full file");
    al_draw_text(font_ui_, COL_DIM, WIN_W/2, 120, ALLEGRO_ALIGN_CENTRE, buf);

    std::snprintf(buf, sizeof(buf), "%d words  |  %d lines",
                  fi.word_count, fi.line_count);
    al_draw_text(font_ui_, COL_TEXT, WIN_W/2, 160, ALLEGRO_ALIGN_CENTRE, buf);

    // Back / Start always present
    draw_button(WIN_W/2-160, 220, 140, 44, "Back");
    draw_button(WIN_W/2+20,  220, 140, 44, "Start!", true);

    // Prose-only navigation row
    if (TextBank::is_prose(cat) && total_para > 0) {
        float bw = 130.0f, bh = 36.0f, gap = 10.0f;
        float total_w = 4*bw + 3*gap;
        float bx = (WIN_W - total_w) / 2.0f;
        float by = 290.0f;
        draw_button(bx,             by, bw, bh, "Redo Last");
        draw_button(bx+bw+gap,      by, bw, bh, "Skip");
        draw_button(bx+2*(bw+gap),  by, bw, bh, "Restart");
        draw_button(bx+3*(bw+gap),  by, bw, bh, "Jump To...");
    }

    al_flip_display();
}
```

- [ ] **Step 3: Add hit-test methods for prose nav buttons**

After the existing `preview_back_click` implementation, add:

```cpp
// Prose nav buttons — same layout as render_preview
static constexpr float PREV_BTN_W   = 130.0f;
static constexpr float PREV_BTN_H   =  36.0f;
static constexpr float PREV_BTN_GAP =  10.0f;
static constexpr float PREV_BTN_Y   = 290.0f;

static float prev_nav_x(int idx) {
    float total_w = 4*PREV_BTN_W + 3*PREV_BTN_GAP;
    float bx = ((float)Graphic_Manager::WIN_W - total_w) / 2.0f;
    return bx + (float)idx * (PREV_BTN_W + PREV_BTN_GAP);
}

static bool in_rect(int x, int y, float bx, float by, float bw, float bh) {
    return (float)x >= bx && (float)x <= bx+bw
        && (float)y >= by && (float)y <= by+bh;
}

bool Graphic_Manager::preview_redo_click(int x, int y) const {
    return in_rect(x, y, prev_nav_x(0), PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H);
}
bool Graphic_Manager::preview_skip_click(int x, int y) const {
    return in_rect(x, y, prev_nav_x(1), PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H);
}
bool Graphic_Manager::preview_restart_click(int x, int y) const {
    return in_rect(x, y, prev_nav_x(2), PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H);
}
bool Graphic_Manager::preview_jump_click(int x, int y) const {
    return in_rect(x, y, prev_nav_x(3), PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H);
}
```

- [ ] **Step 4: Add `render_jump_overlay` and its hit-tests**

```cpp
// Jump overlay — centred modal panel
static constexpr float JUMP_W  = 340.0f;
static constexpr float JUMP_H  = 160.0f;
static constexpr float JUMP_X  = (Graphic_Manager::WIN_W  - JUMP_W) / 2.0f;
static constexpr float JUMP_Y  = (Graphic_Manager::WIN_H  - JUMP_H) / 2.0f;

void Graphic_Manager::render_jump_overlay(int current_para, int total_para,
                                           const std::string& input) {
    // Dim background
    al_draw_filled_rectangle(0, 0, WIN_W, WIN_H, {0,0,0,0.55f});
    // Panel
    al_draw_filled_rounded_rectangle(JUMP_X, JUMP_Y, JUMP_X+JUMP_W, JUMP_Y+JUMP_H,
                                      8, 8, {0.15f,0.15f,0.20f,1.0f});
    al_draw_rounded_rectangle(JUMP_X, JUMP_Y, JUMP_X+JUMP_W, JUMP_Y+JUMP_H,
                               8, 8, COL_DIM, 1.5f);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Jump to paragraph (1 – %d):", total_para);
    al_draw_text(font_ui_, COL_TEXT, JUMP_X+JUMP_W/2, JUMP_Y+18,
                 ALLEGRO_ALIGN_CENTRE, buf);

    // Input box
    float ix = JUMP_X+20, iy = JUMP_Y+52, iw = JUMP_W-40, ih = 36;
    al_draw_filled_rounded_rectangle(ix, iy, ix+iw, iy+ih, 4, 4, {0.08f,0.08f,0.10f,1.0f});
    al_draw_rounded_rectangle(ix, iy, ix+iw, iy+ih, 4, 4, COL_DIM, 1.0f);
    std::string display = input.empty() ? std::to_string(current_para+1) : input;
    al_draw_text(font_ui_, input.empty() ? COL_DIM : COL_WHITE,
                 ix+iw/2, iy+8, ALLEGRO_ALIGN_CENTRE, display.c_str());

    // Buttons
    draw_button(JUMP_X+20,        JUMP_Y+106, 130, 36, "Cancel");
    draw_button(JUMP_X+JUMP_W-150, JUMP_Y+106, 130, 36, "Go!", true);
    al_flip_display();
}

bool Graphic_Manager::jump_confirm_click(int x, int y) const {
    return in_rect(x, y, JUMP_X+JUMP_W-150, JUMP_Y+106, 130, 36);
}
bool Graphic_Manager::jump_cancel_click(int x, int y) const {
    return in_rect(x, y, JUMP_X+20, JUMP_Y+106, 130, 36);
}
```

- [ ] **Step 5: Build and verify**

```bash
bash create.sh 2>&1 | grep -E "error:|warning:"
```

Expected: no output.

- [ ] **Step 6: Commit**

```bash
git add src/graphics/
git commit -m "feat: preview screen — redo, skip, restart, jump buttons + jump overlay"
```

---

### Task 3: main.cpp — wire preview navigation and jump sub-phase

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Add `JUMP` to the `Phase` enum**

```cpp
enum class Phase {
    MENU, CATEGORY, FILE_PICK, PREVIEW, JUMP, PLAYING, RESULTS
};
```

- [ ] **Step 2: Add `jump_input` string to main's local variables**

After the `FileInfo current_file{};` declaration add:

```cpp
    std::string jump_input;
```

- [ ] **Step 3: Add prose nav button handling inside the PREVIEW mouse block**

The existing PREVIEW mouse handler is:

```cpp
            } else if (phase == Phase::PREVIEW) {
                if (gfx.preview_confirm_click(mx, my)) {
                    ...
                } else if (gfx.preview_back_click(mx, my)) {
                    phase = Phase::FILE_PICK;
                    gfx.render_file_pick(bank.files(), file_scroll);
                }
```

Replace it with:

```cpp
            } else if (phase == Phase::PREVIEW) {
                if (gfx.preview_confirm_click(mx, my)) {
                    std::string passage = bank.load_passage(current_file, sel_cat);
                    if (passage.empty()) {
                        phase = Phase::FILE_PICK;
                        gfx.render_file_pick(bank.files(), file_scroll);
                    } else {
                        game.start(passage, sel_mode, sel_emode,
                                   time_limit_sec, word_target);
                        start_time   = al_get_time();
                        elapsed_sec  = 0.0;
                        live_wpm     = 0.0;
                        sample_ticks = 0;
                        cursor_vis   = true;
                        al_start_timer(timer);
                        phase = Phase::PLAYING;
                        gfx.render_playing(game, 0,
                            sel_mode == RoundMode::TimeLimit ? time_limit_sec : -1,
                            0, true);
                    }
                } else if (gfx.preview_back_click(mx, my)) {
                    phase = Phase::FILE_PICK;
                    gfx.render_file_pick(bank.files(), file_scroll);
                } else if (TextBank::is_prose(sel_cat)) {
                    int cp = bank.current_paragraph(current_file);
                    int tp = bank.total_paragraphs(current_file);
                    if (gfx.preview_redo_click(mx, my)) {
                        bank.redo_last(current_file);
                        cp = bank.current_paragraph(current_file);
                        gfx.render_preview(current_file, sel_cat, cp, tp);
                    } else if (gfx.preview_skip_click(mx, my)) {
                        bank.skip_paragraph(current_file, tp);
                        cp = bank.current_paragraph(current_file);
                        gfx.render_preview(current_file, sel_cat, cp, tp);
                    } else if (gfx.preview_restart_click(mx, my)) {
                        bank.reset_progress(current_file);
                        cp = bank.current_paragraph(current_file);
                        gfx.render_preview(current_file, sel_cat, cp, tp);
                    } else if (gfx.preview_jump_click(mx, my)) {
                        jump_input.clear();
                        phase = Phase::JUMP;
                        gfx.render_jump_overlay(cp, tp, jump_input);
                    }
                }
```

- [ ] **Step 4: Add JUMP mouse handler**

After the PREVIEW mouse block, add:

```cpp
            } else if (phase == Phase::JUMP) {
                int cp = bank.current_paragraph(current_file);
                int tp = bank.total_paragraphs(current_file);
                if (gfx.jump_cancel_click(mx, my)) {
                    phase = Phase::PREVIEW;
                    gfx.render_preview(current_file, sel_cat, cp, tp);
                } else if (gfx.jump_confirm_click(mx, my)) {
                    if (!jump_input.empty()) {
                        int target = 0;
                        try { target = std::stoi(jump_input) - 1; } catch (...) {}
                        bank.jump_to_paragraph(current_file, target);
                        cp = bank.current_paragraph(current_file);
                    }
                    phase = Phase::PREVIEW;
                    gfx.render_preview(current_file, sel_cat, cp, tp);
                }
```

- [ ] **Step 5: Add JUMP keyboard handler inside `ALLEGRO_EVENT_KEY_CHAR`**

In the `KEY_CHAR` block, before the `if (phase == Phase::PLAYING)` check, add:

```cpp
            if (phase == Phase::JUMP) {
                int cp = bank.current_paragraph(current_file);
                int tp = bank.total_paragraphs(current_file);
                if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    phase = Phase::PREVIEW;
                    gfx.render_preview(current_file, sel_cat, cp, tp);
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE) {
                    if (!jump_input.empty()) jump_input.pop_back();
                    gfx.render_jump_overlay(cp, tp, jump_input);
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                           ev.keyboard.keycode == ALLEGRO_KEY_PAD_ENTER) {
                    if (!jump_input.empty()) {
                        int target = 0;
                        try { target = std::stoi(jump_input) - 1; } catch (...) {}
                        bank.jump_to_paragraph(current_file, target);
                        cp = bank.current_paragraph(current_file);
                    }
                    phase = Phase::PREVIEW;
                    gfx.render_preview(current_file, sel_cat, cp, tp);
                } else if (ev.keyboard.unichar >= '0' && ev.keyboard.unichar <= '9'
                           && (int)jump_input.size() < 4) {
                    jump_input += (char)ev.keyboard.unichar;
                    gfx.render_jump_overlay(cp, tp, jump_input);
                }
            }
```

- [ ] **Step 6: Build and verify**

```bash
bash create.sh 2>&1 | grep -E "error:|warning:"
```

Expected: no output.

- [ ] **Step 7: Smoke test**

```bash
bash run.sh
```

- Select English → any prose file → Preview screen shows four nav buttons below Start/Back.
- Click **Redo Last** — paragraph counter decrements, screen refreshes.
- Click **Skip** — paragraph counter increments.
- Click **Restart** — counter resets to 1.
- Click **Jump To…** — overlay appears. Type `3`, press Enter — returns to preview at paragraph 3.
- Click **Jump To…**, type a number > total, press Enter — clamps to last paragraph.
- Select a code category (Python) — Preview shows only Start/Back, no nav buttons.
- F11 still works from PREVIEW and JUMP phases.

- [ ] **Step 8: Commit and push**

```bash
git add src/main.cpp
git commit -m "feat: preview navigation — redo, skip, restart, jump with overlay input"
git push
```

---

## Self-Review

**Spec coverage:**

| Requirement | Task |
|---|---|
| Restart from paragraph 0 | Task 1 (`reset_progress`) + Task 3 (restart button) |
| Redo last paragraph | Task 1 (`redo_last`) + Task 3 (redo button) |
| Skip to next paragraph | Task 1 (`skip_paragraph`) + Task 3 (skip button) |
| Jump to specific paragraph | Task 1 (`jump_to_paragraph`) + Task 2 (overlay) + Task 3 (jump phase) |
| Code files unaffected (no nav buttons) | Task 2 (`is_prose` guard) + Task 3 (mouse block guard) |
| Jump input clamped to valid range | Task 1 (`jump_to_paragraph` clamps) |
| Jump overlay Escape cancels | Task 3 (KEY_CHAR JUMP handler) |
| Jump overlay Enter confirms | Task 3 (KEY_CHAR JUMP handler) |

**Placeholder scan:** None — all code is complete.

**Type consistency:** `reset_progress`, `redo_last`, `skip_paragraph`, `jump_to_paragraph` defined in Task 1 and called in Task 3. `render_jump_overlay`, `jump_confirm_click`, `jump_cancel_click` defined in Task 2 and called in Task 3. `Phase::JUMP` added in Task 3 Step 1 before use in Steps 3–5. `jump_input` added in Task 3 Step 2 before use in Steps 4–5.
