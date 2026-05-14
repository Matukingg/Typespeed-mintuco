# Typespeed — Handoff Document for a Fresh Claude Session

## What this project is

A typing-speed practice game written in C++17 using the Allegro 5 graphics library.
It lives at: `https://github.com/Matukingg/Typespeed-mintuco`
Local path: `/home/matuco/Documents/typespeed/`

---

## How to build and run

```bash
# Install dependencies (once)
sudo apt install liballegro5-dev premake4

# Build
bash create.sh

# Run
bash run.sh
```

Output binary: `build/bin/debug/typespeed`

---

## Current state (as of 2026-05-14)

The core game is **fully implemented and compiling cleanly**. All 9 plan tasks are complete:

| What | Status |
|---|---|
| Build system (premake4.lua, create.sh, run.sh) | Done |
| Core singleton + settings.txt | Done |
| Input module (event queue) | Done |
| TextBank (file scanning, paragraph loading, progress tracking) | Done |
| Stats (WPM, accuracy, session history) | Done |
| Game state machine (char tracking, strict/lenient modes) | Done |
| Graphic_Manager (all 6 screens) | Done |
| Full event loop (all phases wired) | Done |
| Endless mode + live WPM | Done |
| Sample data files for all 7 categories | Done |
| Time limit / word count adjustable with arrow keys | Done |

---

## Architecture

```
src/
  main.cpp          — event loop, phase enum, wires all modules
  core/             — Allegro init/shutdown singleton + settings.txt
  graphics/         — Graphic_Manager: all screen rendering
  input/            — ALLEGRO_EVENT_QUEUE wrapper
  game/             — round state machine, char tracking
  textbank/         — file scanning, paragraph loading, progress.txt
  stats/            — WPM/accuracy calculation, history.txt
  globalstats/      — (empty, Plan B)
data/
  english/          — prose .txt files (paragraph-by-paragraph)
  spanish/          — prose .txt files
  python/           — code .txt files (full file each session)
  cpp/
  latex/
  html/
  javascript/
```

**Key rules:**
- `Core` is a singleton — always use `Core::instance()`, never construct on stack
- Event-driven loop — redraws only on keyboard/timer events, not every frame
- Font paths are relative to the executable (not hardcoded absolute paths)
- `data/progress.txt`, `data/history.txt`, `data/keylog.txt` are gitignored

---

## Phase flow

```
MENU → CATEGORY → FILE_PICK → PREVIEW → PLAYING → RESULTS → (back to MENU)
MENU → GLOBAL_STATS  (not yet implemented — Plan B)
```

### MENU
- Select round mode: Paragraph, Time Limit, Word Count, Endless
- Toggle error mode: Strict (must fix errors) / Lenient (errors counted, typing continues)
- Adjust time limit or word target with Left/Right arrow keys
- Click "Start" to proceed to CATEGORY
- Click "Global Stats" (placeholder, Plan B)

### CATEGORY
- Pick: English, Spanish, Python, C++, LaTeX, HTML, JavaScript

### FILE_PICK
- Lists `.txt` files from `data/<category>/`
- Scroll with Up/Down buttons if more than 10 files

### PREVIEW
- Shows filename, paragraph N of M (prose) or "Full file" (code)
- Shows word count and line count
- Confirm or go Back

### PLAYING
- Full passage rendered with colour-coded characters (neutral / green / correct / red / wrong)
- Blinking vertical bar cursor
- Toolbar: elapsed time, live WPM, time remaining (time limit mode only)
- Escape → Results

### RESULTS
- WPM, accuracy %, error count
- Moment-to-moment WPM graph (sampled every ~5 seconds)
- Play Again → back to PREVIEW | Menu → back to MENU

---

## Key types and interfaces

### `TextBank` (`src/textbank/textbank.hpp`)
```cpp
void scan(Category cat);                          // scan data/<cat>/
std::string load_passage(const FileInfo& fi, Category cat); // loads + advances progress
int current_paragraph(const FileInfo& fi) const;
int total_paragraphs(const FileInfo& fi)  const;
void load_progress(); void save_progress();
static std::string category_folder(Category cat);
static bool        is_prose(Category cat);
```

### `Game` (`src/game/game.hpp`)
```cpp
void start(const std::string& passage, RoundMode, ErrorMode, int time_limit_sec, int word_target);
bool on_key(int unichar);       // returns false if key ignored (strict mode blocker)
bool on_backspace();
bool is_finished() const;
double live_wpm(double elapsed_sec) const;
SessionResult finish(const std::string& category, const std::string& filename, int elapsed_ms);
```

### `Stats` (`src/stats/stats.hpp`)
```cpp
void record_correct(); void record_error();
void sample_wpm(double elapsed_sec);             // call every ~5s
SessionResult finish(category, filename, elapsed_ms);
static void append_history(const SessionResult& r);
static std::vector<SessionResult> load_history();
```

### `Graphic_Manager` (`src/graphics/graphicMgr.hpp`)
```cpp
void render_menu(RoundMode, ErrorMode, int time_limit_sec, int word_target);
int  menu_click(int x, int y) const;             // 0-3=mode, 10=strict, 11=lenient, 20=stats, 30=start
void render_category(Category current);
int  category_click(int x, int y) const;         // 0-6 or -1
void render_file_pick(const vector<FileInfo>&, int scroll_offset);
int  file_click(int x, int y, int scroll_offset) const;
void render_preview(const FileInfo&, Category, int current_para, int total_para);
bool preview_confirm_click(int x, int y) const;
bool preview_back_click(int x, int y)    const;
void render_playing(const Game&, double elapsed_sec, int time_remaining_sec, double live_wpm, bool cursor_visible);
void render_results(const SessionResult&);
bool results_again_click(int x, int y) const;
bool results_menu_click(int x, int y)  const;
```

---

## What's next — Plan B (Global Stats)

The `src/globalstats/` directory is empty and waiting. Plan B covers:

- **`data/keylog.txt`** — per-keypress timing log (appended during PLAYING phase)
- **Keyboard heatmap** — full LATAM keyboard (layout: `latam`, variant: `deadtilde`, model: `pc105`) rendered on screen; keys coloured by average press time or error rate
- **Bigram analysis** — slowest two-key sequences
- **WPM trend graph** — all sessions over time
- **Sessions per day heatmap** (GitHub-style)
- **Personal bests per category**
- **Consistency stats** — WPM std deviation, flow state streaks
- **Lifetime totals** — words typed, time practiced, most practiced file
- **Motivational** — nemesis key, WPM goal tracker, mastery badges (Bronze/Silver/Gold per category)

To start Plan B: create `src/globalstats/globalstats.hpp/.cpp`, add a `GLOBAL_STATS` phase to the enum in `main.cpp`, wire the "Global Stats" button (currently code 20 in `menu_click`), and implement keypress logging in the `PLAYING` keyboard handler.

---

## Known issues / rough edges

- Fonts: if `data/font.ttf` and `data/mono.ttf` are missing, Allegro's built-in 8px bitmap font is used. The game works but looks basic. Add TTF fonts to `data/` for a better appearance.
- The passage text wraps by character width using a fixed `M`-width estimate — works for monospace fonts; may look off with proportional fonts.
- Strict mode marks the character red but doesn't stop the cursor from visually blinking at the same position — it stays put until backspace is pressed, which is correct behaviour.
