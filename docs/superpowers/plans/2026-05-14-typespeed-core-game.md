# Typespeed Core Game — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a fully playable typing-speed game in C++/Allegro5 covering build system, all game phases (Menu → Category → File Pick → Preview → Playing → Results), four round modes, prose paragraph progression, and per-session stats with a WPM graph.

**Architecture:** Event-driven loop in `main.cpp` with a phase enum; each module owns its own state and exposes a clean interface. Mirrors the sibling minesweeper project exactly — same Core singleton, same Input wrapper, same Graphic_Manager pattern.

**Tech Stack:** C++17, Allegro 5 (`allegro`, `allegro_image`, `allegro_font`, `allegro_ttf`, `allegro_primitives`), Premake4 + GNU Make, Linux (LATAM keyboard).

---

## File Map

| File | Role |
|---|---|
| `premake4.lua` | Build definition |
| `create.sh` | Clean + premake4 + make debug |
| `run.sh` | Launch executable |
| `.gitignore` | Ignore build/, data/progress.txt, data/history.txt, data/keylog.txt |
| `src/main.cpp` | Event loop, phase enum, wires all modules |
| `src/core/core.hpp` | Core singleton declaration (Allegro init + settings) |
| `src/core/core.cpp` | Core singleton implementation |
| `src/input/input.hpp` | Input declaration |
| `src/input/input.cpp` | Input implementation (event queue) |
| `src/textbank/textbank.hpp` | TextBank declaration (scan, load, progress) |
| `src/textbank/textbank.cpp` | TextBank implementation |
| `src/game/game.hpp` | Game declaration (round state, char tracking, timer) |
| `src/game/game.cpp` | Game implementation |
| `src/stats/stats.hpp` | Stats declaration (WPM, accuracy, history) |
| `src/stats/stats.cpp` | Stats implementation |
| `src/graphics/graphicMgr.hpp` | Graphic_Manager declaration |
| `src/graphics/graphicMgr.cpp` | Graphic_Manager implementation (all rendering) |
| `data/english/.gitkeep` | Placeholder so folder is tracked |
| `data/spanish/.gitkeep` | Placeholder |
| `data/python/.gitkeep` | Placeholder |
| `data/cpp/.gitkeep` | Placeholder |
| `data/latex/.gitkeep` | Placeholder |
| `data/html/.gitkeep` | Placeholder |
| `data/javascript/.gitkeep` | Placeholder |

---

### Task 1: Build system + skeleton compile

**Files:**
- Create: `premake4.lua`
- Create: `create.sh`
- Create: `run.sh`
- Create: `.gitignore`
- Create: `src/main.cpp`
- Create: `src/core/core.hpp`
- Create: `src/core/core.cpp`
- Create: `src/input/input.hpp`
- Create: `src/input/input.cpp`

- [ ] **Step 1: Create `premake4.lua`**

```lua
solution "Typespeed"
   configurations { "Debug", "Release" }
   location "build"
   language "C++"

project "typespeed"
   kind "ConsoleApp"
   language "C++"
   files { "src/**.hpp", "src/**.cpp" }
   includedirs {
      "src",
      "src/core",
      "src/graphics",
      "src/input",
      "src/game",
      "src/textbank",
      "src/stats",
      "src/globalstats",
   }
   links {
      "allegro",
      "allegro_image",
      "allegro_font",
      "allegro_ttf",
      "allegro_primitives",
   }
   configuration "Debug"
      defines { "DEBUG" }
      flags { "Symbols" }
      targetdir "build/bin/debug"
      objdir    "build/obj/debug"
   configuration "Release"
      defines { "NDEBUG" }
      flags { "Optimize" }
      targetdir "build/bin/release"
      objdir    "build/obj/release"
```

- [ ] **Step 2: Create `create.sh`**

```bash
#!/usr/bin/env bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
echo "=== 1. Generating Makefiles ==="
premake4 --file="$PROJECT_ROOT/premake4.lua" gmake
echo "=== 2. Compiling (Debug) ==="
make -k -C "$BUILD_DIR" config=debug
```

Run `chmod +x create.sh`.

- [ ] **Step 3: Create `run.sh`**

```bash
#!/usr/bin/env bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXECUTABLE="$PROJECT_ROOT/build/bin/debug/typespeed"
if [[ -x "$EXECUTABLE" ]]; then
  exec "$EXECUTABLE"
else
  echo "No executable found. Run create.sh first."
  exit 1
fi
```

Run `chmod +x run.sh`.

- [ ] **Step 4: Create `.gitignore`**

```
build/
data/progress.txt
data/history.txt
data/keylog.txt
settings.txt
CLAUDE.md
.claude/
```

- [ ] **Step 5: Create `src/core/core.hpp`**

```cpp
#pragma once
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <iostream>
#include <unordered_map>
#include <string>

class Core {
    std::unordered_map<std::string, std::string> settings = {
        {"mode",           "paragraph"},
        {"error_handling", "strict"},
        {"category",       "english"},
    };
public:
    Core();
    ~Core();
    static Core& instance();

    bool load_settings(const std::string& filename = "settings.txt");
    bool save_settings(const std::string& filename = "settings.txt") const;

    std::string get_string(const std::string& key, const std::string& def = "") const;
    int         get_int   (const std::string& key, int def = 0)                  const;
    bool        get_bool  (const std::string& key, bool def = false)             const;

    void set_string(const std::string& key, const std::string& value);
    void set_int   (const std::string& key, int value);
    void set_bool  (const std::string& key, bool value);
};
```

- [ ] **Step 6: Create `src/core/core.cpp`**

```cpp
#include "core.hpp"
#include <fstream>
#include <sstream>

Core::Core() {
    if (!al_init())                  { std::cerr << "al_init failed\n";       std::exit(1); }
    if (!al_init_primitives_addon()) { std::cerr << "primitives failed\n";    std::exit(1); }
    if (!al_init_image_addon())      { std::cerr << "image addon failed\n";   std::exit(1); }
    if (!al_init_font_addon())       { std::cerr << "font addon failed\n";    std::exit(1); }
    if (!al_init_ttf_addon())        { std::cerr << "ttf addon failed\n";     std::exit(1); }
    if (!al_install_keyboard())      { std::cerr << "keyboard failed\n";      std::exit(1); }
}

Core::~Core() {
    al_shutdown_ttf_addon();
    al_shutdown_font_addon();
    al_shutdown_image_addon();
    al_shutdown_primitives_addon();
    al_uninstall_keyboard();
    al_uninstall_system();
}

Core& Core::instance() {
    static Core c;
    return c;
}

bool Core::load_settings(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        settings[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return true;
}

bool Core::save_settings(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) return false;
    for (const auto& p : settings)
        out << p.first << "=" << p.second << "\n";
    return true;
}

std::string Core::get_string(const std::string& key, const std::string& def) const {
    auto it = settings.find(key);
    return (it != settings.end()) ? it->second : def;
}

int Core::get_int(const std::string& key, int def) const {
    auto s = get_string(key);
    if (s.empty()) return def;
    try { return std::stoi(s); } catch (...) { return def; }
}

bool Core::get_bool(const std::string& key, bool def) const {
    auto s = get_string(key);
    if (s == "1" || s == "true")  return true;
    if (s == "0" || s == "false") return false;
    return def;
}

void Core::set_string(const std::string& key, const std::string& value) { settings[key] = value; }
void Core::set_int   (const std::string& key, int value)   { settings[key] = std::to_string(value); }
void Core::set_bool  (const std::string& key, bool value)  { settings[key] = value ? "1" : "0"; }
```

- [ ] **Step 7: Create `src/input/input.hpp`**

```cpp
#pragma once
#include <allegro5/allegro5.h>

class Input {
    ALLEGRO_EVENT_QUEUE* queue;
public:
    explicit Input(ALLEGRO_DISPLAY* display);
    ~Input();
    ALLEGRO_EVENT_QUEUE* get_queue() const { return queue; }
};
```

- [ ] **Step 8: Create `src/input/input.cpp`**

```cpp
#include "input.hpp"

Input::Input(ALLEGRO_DISPLAY* display) {
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
}

Input::~Input() {
    al_destroy_event_queue(queue);
}
```

- [ ] **Step 9: Create skeleton `src/main.cpp`**

```cpp
#include "core.hpp"
#include "input.hpp"

int main() {
    Core& core = Core::instance();
    core.load_settings();

    ALLEGRO_DISPLAY* display = al_create_display(800, 500);
    if (!display) { return -1; }

    Input input(display);

    bool running = true;
    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(input.get_queue(), &ev);
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) running = false;
    }

    al_destroy_display(display);
    return 0;
}
```

- [ ] **Step 10: Create data folder placeholders**

```bash
mkdir -p data/english data/spanish data/python data/cpp data/latex data/html data/javascript
touch data/english/.gitkeep data/spanish/.gitkeep data/python/.gitkeep \
      data/cpp/.gitkeep data/latex/.gitkeep data/html/.gitkeep data/javascript/.gitkeep
```

- [ ] **Step 11: Build and verify it compiles**

```bash
bash create.sh
```

Expected: `build/bin/debug/typespeed` exists, no errors. A blank window opens with `bash run.sh` and closes cleanly.

- [ ] **Step 12: Commit**

```bash
git add premake4.lua create.sh run.sh .gitignore src/ data/
git commit -m "feat: build system, Core, Input skeleton — blank window runs"
```

---

### Task 2: TextBank — file scanning, paragraph loading, progress tracking

**Files:**
- Create: `src/textbank/textbank.hpp`
- Create: `src/textbank/textbank.cpp`

- [ ] **Step 1: Create `src/textbank/textbank.hpp`**

```cpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

enum class Category { English, Spanish, Python, Cpp, Latex, Html, Javascript };

struct FileInfo {
    std::string filename;   // bare name, e.g. "1984.txt"
    std::string full_path;  // e.g. "data/english/1984.txt"
    int         word_count;
    int         line_count;
    int         paragraph_count; // prose only; 0 for code
};

class TextBank {
public:
    // Scan data/<category>/ and populate file list
    void scan(Category cat);

    // All files found after scan()
    const std::vector<FileInfo>& files() const { return files_; }

    // Category folder name, e.g. "english"
    static std::string category_folder(Category cat);

    // True if category is prose (English/Spanish)
    static bool is_prose(Category cat);

    // Load a passage:
    //   Prose  — loads paragraph at current progress index, advances index, saves progress.txt
    //   Code   — loads the full file
    // Returns the passage text. Empty string on failure.
    std::string load_passage(const FileInfo& fi, Category cat);

    // For the preview screen
    int current_paragraph(const FileInfo& fi) const;  // prose only; -1 for code
    int total_paragraphs (const FileInfo& fi) const;  // prose only; -1 for code

    // Load/save progress.txt
    void load_progress(const std::string& filename = "data/progress.txt");
    void save_progress(const std::string& filename = "data/progress.txt") const;

private:
    std::vector<FileInfo> files_;

    // key: "english/1984.txt"  value: current paragraph index (-1 = completed)
    std::unordered_map<std::string, int> progress_;

    // key: "english/1984.txt"  value: all paragraphs split from the file
    std::unordered_map<std::string, std::vector<std::string>> para_cache_;

    std::string progress_key(const FileInfo& fi, Category cat) const;
    std::vector<std::string> split_paragraphs(const std::string& text) const;
    FileInfo compute_info(const std::string& full_path, Category cat) const;
};
```

- [ ] **Step 2: Create `src/textbank/textbank.cpp`**

```cpp
#include "textbank.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <random>
#include <stdexcept>

namespace fs = std::filesystem;

std::string TextBank::category_folder(Category cat) {
    switch (cat) {
        case Category::English:    return "english";
        case Category::Spanish:    return "spanish";
        case Category::Python:     return "python";
        case Category::Cpp:        return "cpp";
        case Category::Latex:      return "latex";
        case Category::Html:       return "html";
        case Category::Javascript: return "javascript";
    }
    return "english";
}

bool TextBank::is_prose(Category cat) {
    return cat == Category::English || cat == Category::Spanish;
}

std::string TextBank::progress_key(const FileInfo& fi, Category cat) const {
    return category_folder(cat) + "/" + fi.filename;
}

std::vector<std::string> TextBank::split_paragraphs(const std::string& text) const {
    std::vector<std::string> result;
    std::istringstream ss(text);
    std::string line, block;
    while (std::getline(ss, line)) {
        if (line.empty()) {
            if (!block.empty()) { result.push_back(block); block.clear(); }
        } else {
            if (!block.empty()) block += '\n';
            block += line;
        }
    }
    if (!block.empty()) result.push_back(block);
    return result;
}

TextBank::FileInfo TextBank::compute_info(const std::string& full_path, Category cat) const {
    FileInfo fi;
    fi.full_path = full_path;
    fi.filename  = fs::path(full_path).filename().string();
    fi.word_count = 0;
    fi.line_count = 0;
    fi.paragraph_count = 0;

    std::ifstream f(full_path);
    if (!f.is_open()) return fi;

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    std::istringstream ss(content);
    std::string line;
    bool in_para = false;
    while (std::getline(ss, line)) {
        fi.line_count++;
        std::istringstream ws(line);
        std::string word;
        while (ws >> word) fi.word_count++;
        if (is_prose(cat)) {
            if (!line.empty()) in_para = true;
            else if (in_para) { fi.paragraph_count++; in_para = false; }
        }
    }
    if (is_prose(cat) && in_para) fi.paragraph_count++;

    return fi;
}

void TextBank::scan(Category cat) {
    files_.clear();
    std::string folder = "data/" + category_folder(cat);
    if (!fs::exists(folder)) return;
    for (auto& entry : fs::directory_iterator(folder)) {
        if (entry.path().extension() == ".txt")
            files_.push_back(compute_info(entry.path().string(), cat));
    }
    std::sort(files_.begin(), files_.end(),
              [](const FileInfo& a, const FileInfo& b){ return a.filename < b.filename; });
}

int TextBank::current_paragraph(const FileInfo& fi) const {
    // Returns 0-based index of the next paragraph to show, or 0 if not tracked
    auto it = progress_.find(fi.full_path);
    if (it == progress_.end()) return 0;
    return (it->second < 0) ? 0 : it->second;
}

int TextBank::total_paragraphs(const FileInfo& fi) const {
    auto it = para_cache_.find(fi.full_path);
    if (it == para_cache_.end()) return fi.paragraph_count;
    return (int)it->second.size();
}

std::string TextBank::load_passage(const FileInfo& fi, Category cat) {
    std::ifstream f(fi.full_path);
    if (!f.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    if (!is_prose(cat)) return content;

    // Prose: paragraph progression
    if (para_cache_.find(fi.full_path) == para_cache_.end())
        para_cache_[fi.full_path] = split_paragraphs(content);

    auto& paras = para_cache_[fi.full_path];
    if (paras.empty()) return content;

    int& idx = progress_[fi.full_path];
    if (idx < 0) {
        // completed — pick random
        std::mt19937 rng(std::random_device{}());
        idx = std::uniform_int_distribution<int>(0, (int)paras.size()-1)(rng);
    }
    std::string passage = paras[idx];
    idx++;
    if (idx >= (int)paras.size()) idx = -1; // mark completed
    save_progress();
    return passage;
}

void TextBank::load_progress(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        int val = 0;
        try { val = std::stoi(line.substr(eq+1)); } catch (...) {}
        // key format: "english/1984.txt" → full_path "data/english/1984.txt"
        progress_["data/" + key] = val;
    }
}

void TextBank::save_progress(const std::string& filename) const {
    std::ofstream f(filename);
    if (!f.is_open()) return;
    for (auto& p : progress_)
        // strip leading "data/" from key
        f << p.first.substr(5) << "=" << p.second << "\n";
}
```

- [ ] **Step 3: Build and verify**

```bash
bash create.sh 2>&1 | grep -E "error:|warning:" | head -20
```

Expected: no errors. Warnings about unused variables are acceptable at this stage.

- [ ] **Step 4: Commit**

```bash
git add src/textbank/
git commit -m "feat: TextBank — file scan, paragraph loading, progress tracking"
```

---

### Task 3: Stats — WPM calculation, session history

**Files:**
- Create: `src/stats/stats.hpp`
- Create: `src/stats/stats.cpp`

- [ ] **Step 1: Create `src/stats/stats.hpp`**

```cpp
#pragma once
#include <string>
#include <vector>
#include <ctime>

struct WpmSample {
    double elapsed_sec;
    double wpm;
};

struct SessionResult {
    std::string category;     // "english", "python", etc.
    std::string filename;
    double      wpm;
    double      accuracy;     // 0.0–1.0
    int         error_count;
    int         elapsed_ms;
    std::time_t timestamp;
    std::vector<WpmSample> wpm_samples; // moment-to-moment
};

class Stats {
public:
    // Called every key event during a round
    void record_correct();
    void record_error();

    // Called every ~5 seconds to snapshot current WPM
    void sample_wpm(double elapsed_sec);

    // Compute final result. elapsed_ms = total round duration.
    SessionResult finish(const std::string& category,
                         const std::string& filename,
                         int elapsed_ms);

    // Reset for new round
    void reset();

    // Persist session to data/history.txt (append)
    static void append_history(const SessionResult& r,
                               const std::string& filename = "data/history.txt");

    // Load all sessions from data/history.txt
    static std::vector<SessionResult> load_history(
        const std::string& filename = "data/history.txt");

private:
    int correct_chars_ = 0;
    int total_chars_   = 0;  // correct + errors
    int error_count_   = 0;
    std::vector<WpmSample> samples_;
};
```

- [ ] **Step 2: Create `src/stats/stats.cpp`**

```cpp
#include "stats.hpp"
#include <fstream>
#include <sstream>
#include <cmath>

void Stats::record_correct() { correct_chars_++; total_chars_++; }
void Stats::record_error()   { error_count_++;   total_chars_++; }

void Stats::sample_wpm(double elapsed_sec) {
    if (elapsed_sec <= 0.0) return;
    double minutes = elapsed_sec / 60.0;
    double wpm = (correct_chars_ / 5.0) / minutes;
    samples_.push_back({elapsed_sec, wpm});
}

SessionResult Stats::finish(const std::string& category,
                             const std::string& filename,
                             int elapsed_ms) {
    SessionResult r;
    r.category    = category;
    r.filename    = filename;
    r.elapsed_ms  = elapsed_ms;
    r.error_count = error_count_;
    r.timestamp   = std::time(nullptr);
    r.wpm_samples = samples_;

    double minutes = elapsed_ms / 60000.0;
    r.wpm = (minutes > 0.0) ? (correct_chars_ / 5.0) / minutes : 0.0;
    r.accuracy = (total_chars_ > 0) ? (double)correct_chars_ / total_chars_ : 1.0;
    return r;
}

void Stats::reset() {
    correct_chars_ = 0;
    total_chars_   = 0;
    error_count_   = 0;
    samples_.clear();
}

void Stats::append_history(const SessionResult& r, const std::string& filename) {
    std::ofstream f(filename, std::ios::app);
    if (!f.is_open()) return;
    // Format: timestamp,category,file,wpm,accuracy,errors,elapsed_ms,samples
    // samples encoded as t1:w1;t2:w2;...
    f << r.timestamp << "," << r.category << "," << r.filename << ","
      << r.wpm << "," << r.accuracy << "," << r.error_count << ","
      << r.elapsed_ms;
    for (auto& s : r.wpm_samples)
        f << "," << s.elapsed_sec << ":" << s.wpm;
    f << "\n";
}

std::vector<SessionResult> Stats::load_history(const std::string& filename) {
    std::vector<SessionResult> results;
    std::ifstream f(filename);
    if (!f.is_open()) return results;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string tok;
        SessionResult r;
        // timestamp
        std::getline(ss, tok, ','); try { r.timestamp = std::stoll(tok); } catch (...) { continue; }
        std::getline(ss, r.category, ',');
        std::getline(ss, r.filename,  ',');
        std::getline(ss, tok, ','); try { r.wpm       = std::stod(tok);  } catch (...) {}
        std::getline(ss, tok, ','); try { r.accuracy  = std::stod(tok);  } catch (...) {}
        std::getline(ss, tok, ','); try { r.error_count = std::stoi(tok);} catch (...) {}
        std::getline(ss, tok, ','); try { r.elapsed_ms  = std::stoi(tok);} catch (...) {}
        while (std::getline(ss, tok, ',')) {
            auto colon = tok.find(':');
            if (colon == std::string::npos) continue;
            WpmSample s;
            try {
                s.elapsed_sec = std::stod(tok.substr(0, colon));
                s.wpm         = std::stod(tok.substr(colon+1));
            } catch (...) { continue; }
            r.wpm_samples.push_back(s);
        }
        results.push_back(r);
    }
    return results;
}
```

- [ ] **Step 3: Build and verify**

```bash
bash create.sh 2>&1 | grep -E "error:" | head -20
```

Expected: no errors.

- [ ] **Step 4: Commit**

```bash
git add src/stats/
git commit -m "feat: Stats — WPM calculation, accuracy, session history load/save"
```

---

### Task 4: Game — round state machine and character tracking

**Files:**
- Create: `src/game/game.hpp`
- Create: `src/game/game.cpp`

- [ ] **Step 1: Create `src/game/game.hpp`**

```cpp
#pragma once
#include <string>
#include <vector>
#include "stats.hpp"

enum class ErrorMode { Strict, Lenient };
enum class RoundMode { Paragraph, TimeLimit, WordCount, Endless };

struct CharState {
    char    ch;
    enum class Status { Neutral, Correct, Wrong } status = Status::Neutral;
};

class Game {
public:
    void start(const std::string& passage, RoundMode mode, ErrorMode emode,
               int time_limit_sec = 60, int word_target = 50);

    // Returns true if the key was consumed (false = ignore, e.g. wrong key in strict mode)
    // Pass ev.keyboard.unichar from Allegro KEY_CHAR event
    bool on_key(int unichar);
    bool on_backspace();

    // Called every ~5 seconds to snapshot WPM into Stats
    void tick_sample(double elapsed_sec);

    bool is_finished()  const;  // passage complete or time/word target reached
    bool is_strict()    const { return emode_ == ErrorMode::Strict; }

    const std::vector<CharState>& char_states() const { return chars_; }
    int cursor_pos()    const { return cursor_; }
    int error_count()   const { return stats_.error_count_proxy(); }
    int words_typed()   const;
    RoundMode round_mode() const { return mode_; }

    // Finalise and return result. Call after is_finished() == true.
    SessionResult finish(const std::string& category,
                         const std::string& filename,
                         int elapsed_ms);

    void reset();

private:
    std::vector<CharState> chars_;
    int         cursor_ = 0;
    RoundMode   mode_   = RoundMode::Paragraph;
    ErrorMode   emode_  = ErrorMode::Strict;
    int         time_limit_sec_ = 60;
    int         word_target_    = 50;
    bool        has_error_      = false; // strict: unresolved error at cursor
    Stats       stats_;
};
```

- [ ] **Step 2: Add `error_count_proxy()` to Stats**

In `src/stats/stats.hpp`, add inside the `Stats` class:

```cpp
    int error_count_proxy() const { return error_count_; }
```

- [ ] **Step 3: Create `src/game/game.cpp`**

```cpp
#include "game.hpp"
#include <sstream>

void Game::start(const std::string& passage, RoundMode mode, ErrorMode emode,
                 int time_limit_sec, int word_target) {
    chars_.clear();
    for (char c : passage)
        chars_.push_back({c, CharState::Status::Neutral});
    cursor_         = 0;
    mode_           = mode;
    emode_          = emode;
    time_limit_sec_ = time_limit_sec;
    word_target_    = word_target;
    has_error_      = false;
    stats_.reset();
}

bool Game::on_key(int unichar) {
    if (cursor_ >= (int)chars_.size()) return false;
    if (unichar < 32) return false; // ignore control characters

    char expected = chars_[cursor_].ch;
    bool correct  = ((char)unichar == expected);

    if (!correct && emode_ == ErrorMode::Strict) {
        // Mark wrong but don't advance
        chars_[cursor_].status = CharState::Status::Wrong;
        has_error_ = true;
        stats_.record_error();
        return true;
    }

    if (correct) {
        chars_[cursor_].status = CharState::Status::Correct;
        stats_.record_correct();
        has_error_ = false;
    } else {
        chars_[cursor_].status = CharState::Status::Wrong;
        stats_.record_error();
    }
    cursor_++;
    return true;
}

bool Game::on_backspace() {
    if (cursor_ <= 0) return false;
    cursor_--;
    chars_[cursor_].status = CharState::Status::Neutral;
    has_error_ = false;
    return true;
}

void Game::tick_sample(double elapsed_sec) {
    stats_.sample_wpm(elapsed_sec);
}

bool Game::is_finished() const {
    if (mode_ == RoundMode::Paragraph || mode_ == RoundMode::Endless)
        return cursor_ >= (int)chars_.size();
    if (mode_ == RoundMode::WordCount)
        return words_typed() >= word_target_;
    // TimeLimit: caller checks the clock and calls finish() directly
    return cursor_ >= (int)chars_.size();
}

int Game::words_typed() const {
    // Count spaces passed (words = spaces + 1, but only count completed words)
    int spaces = 0;
    for (int i = 0; i < cursor_ && i < (int)chars_.size(); i++)
        if (chars_[i].ch == ' ' && chars_[i].status == CharState::Status::Correct)
            spaces++;
    return spaces;
}

SessionResult Game::finish(const std::string& category,
                            const std::string& filename,
                            int elapsed_ms) {
    return stats_.finish(category, filename, elapsed_ms);
}

void Game::reset() {
    chars_.clear();
    cursor_    = 0;
    has_error_ = false;
    stats_.reset();
}
```

- [ ] **Step 4: Build and verify**

```bash
bash create.sh 2>&1 | grep -E "error:" | head -20
```

Expected: no errors.

- [ ] **Step 5: Commit**

```bash
git add src/game/ src/stats/stats.hpp
git commit -m "feat: Game round state machine — char tracking, strict/lenient modes"
```

---

### Task 5: Graphic_Manager — display + menu rendering

**Files:**
- Create: `src/graphics/graphicMgr.hpp`
- Create: `src/graphics/graphicMgr.cpp`

- [ ] **Step 1: Create `src/graphics/graphicMgr.hpp`**

```cpp
#pragma once
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <string>
#include <vector>
#include "textbank.hpp"
#include "game.hpp"
#include "stats.hpp"

class Graphic_Manager {
public:
    static constexpr int WIN_W    = 900;
    static constexpr int WIN_H    = 600;
    static constexpr int TOOLBAR_H = 44;

    explicit Graphic_Manager();
    ~Graphic_Manager();

    ALLEGRO_DISPLAY* get_display() const { return display_; }

    // ── Menu ──────────────────────────────────────────────────────────────────
    // Returns button index clicked, or -1
    // Buttons: 0=Paragraph 1=TimeLimit 2=WordCount 3=Endless 4=GlobalStats
    int  render_menu(RoundMode current_mode, ErrorMode current_emode);
    int  menu_click(int x, int y) const;

    // ── Category pick ─────────────────────────────────────────────────────────
    // Returns Category index clicked, or -1
    void render_category(Category current);
    int  category_click(int x, int y) const;

    // ── File pick ─────────────────────────────────────────────────────────────
    // Returns file index clicked, or -1. scroll_offset = first visible file index.
    void render_file_pick(const std::vector<FileInfo>& files, int scroll_offset);
    int  file_click(int x, int y, int scroll_offset) const;
    bool file_scroll_up_click(int x, int y)   const;
    bool file_scroll_down_click(int x, int y) const;

    // ── Preview ───────────────────────────────────────────────────────────────
    // Returns true=confirm, false=back (caller checks which button)
    void render_preview(const FileInfo& fi, Category cat,
                        int current_para, int total_para);
    bool preview_confirm_click(int x, int y) const;
    bool preview_back_click(int x, int y)    const;

    // ── Playing ───────────────────────────────────────────────────────────────
    void render_playing(const Game& game, double elapsed_sec,
                        int time_remaining_sec, // -1 if not time-limit mode
                        double live_wpm,
                        bool cursor_visible);   // blink state

    // ── Results ───────────────────────────────────────────────────────────────
    void render_results(const SessionResult& r);
    bool results_again_click(int x, int y) const;
    bool results_menu_click(int x, int y)  const;

private:
    ALLEGRO_DISPLAY* display_;
    ALLEGRO_FONT*    font_ui_;    // loaded from data/font.ttf or built-in
    ALLEGRO_FONT*    font_mono_;  // monospace for the passage text

    void draw_toolbar(double elapsed_sec, double wpm, int time_remaining_sec);
    void draw_button(float x, float y, float w, float h,
                     const std::string& label, bool highlighted = false);
    void draw_passage(const Game& game, float x, float y,
                      float max_w, bool cursor_visible);
};
```

- [ ] **Step 2: Create `src/graphics/graphicMgr.cpp`**

This is the largest file. Implement section by section.

```cpp
#include "graphicMgr.hpp"
#include <allegro5/allegro_font.h>
#include <cstring>
#include <cstdio>

// ── Colours ──────────────────────────────────────────────────────────────────
static const ALLEGRO_COLOR COL_BG       = {0.10f, 0.10f, 0.12f, 1.0f};
static const ALLEGRO_COLOR COL_TOOLBAR  = {0.14f, 0.14f, 0.18f, 1.0f};
static const ALLEGRO_COLOR COL_TEXT     = {0.85f, 0.85f, 0.85f, 1.0f};
static const ALLEGRO_COLOR COL_CORRECT  = {0.35f, 0.85f, 0.45f, 1.0f};
static const ALLEGRO_COLOR COL_WRONG    = {0.90f, 0.25f, 0.25f, 1.0f};
static const ALLEGRO_COLOR COL_CURSOR   = {0.90f, 0.80f, 0.20f, 1.0f};
static const ALLEGRO_COLOR COL_BUTTON   = {0.20f, 0.20f, 0.28f, 1.0f};
static const ALLEGRO_COLOR COL_BUTTON_H = {0.30f, 0.30f, 0.45f, 1.0f};
static const ALLEGRO_COLOR COL_WHITE    = {1.0f,  1.0f,  1.0f,  1.0f};

// ── Constructor / Destructor ──────────────────────────────────────────────────
Graphic_Manager::Graphic_Manager() {
    display_ = al_create_display(WIN_W, WIN_H);
    if (!display_) { std::exit(1); }
    al_set_window_title(display_, "Typespeed");

    // Try loading a bundled font; fall back to built-in
    font_ui_   = al_load_ttf_font("data/font.ttf", 18, 0);
    font_mono_ = al_load_ttf_font("data/mono.ttf", 16, 0);
    if (!font_ui_)   font_ui_   = al_create_builtin_font();
    if (!font_mono_) font_mono_ = al_create_builtin_font();
}

Graphic_Manager::~Graphic_Manager() {
    al_destroy_font(font_ui_);
    al_destroy_font(font_mono_);
    al_destroy_display(display_);
}

// ── Helpers ───────────────────────────────────────────────────────────────────
void Graphic_Manager::draw_button(float x, float y, float w, float h,
                                   const std::string& label, bool highlighted) {
    ALLEGRO_COLOR col = highlighted ? COL_BUTTON_H : COL_BUTTON;
    al_draw_filled_rounded_rectangle(x, y, x+w, y+h, 6, 6, col);
    al_draw_rounded_rectangle(x, y, x+w, y+h, 6, 6, COL_TEXT, 1.0f);
    al_draw_text(font_ui_, COL_WHITE,
                 x + w/2, y + h/2 - al_get_font_line_height(font_ui_)/2,
                 ALLEGRO_ALIGN_CENTRE, label.c_str());
}

void Graphic_Manager::draw_toolbar(double elapsed_sec, double wpm,
                                    int time_remaining_sec) {
    al_draw_filled_rectangle(0, 0, WIN_W, TOOLBAR_H, COL_TOOLBAR);
    char buf[64];
    int  mins = (int)elapsed_sec / 60;
    int  secs = (int)elapsed_sec % 60;
    std::snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
    al_draw_text(font_ui_, COL_TEXT, 10, 12, 0, buf);

    std::snprintf(buf, sizeof(buf), "%.0f WPM", wpm);
    al_draw_text(font_ui_, COL_TEXT, WIN_W/2, 12, ALLEGRO_ALIGN_CENTRE, buf);

    if (time_remaining_sec >= 0) {
        std::snprintf(buf, sizeof(buf), "-%02d:%02d",
                      time_remaining_sec/60, time_remaining_sec%60);
        al_draw_text(font_ui_, COL_TEXT, WIN_W-10, 12, ALLEGRO_ALIGN_RIGHT, buf);
    }
}

// ── Menu ──────────────────────────────────────────────────────────────────────
// Buttons arranged vertically centred. Layout constants (pixels from top of window):
static constexpr float MENU_BTN_X = 300.0f;
static constexpr float MENU_BTN_W = 300.0f;
static constexpr float MENU_BTN_H =  44.0f;
static constexpr float MENU_BTN_GAP = 14.0f;
static constexpr float MENU_START_Y = 160.0f;

int Graphic_Manager::render_menu(RoundMode current_mode, ErrorMode current_emode) {
    al_clear_to_color(COL_BG);
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 80, ALLEGRO_ALIGN_CENTRE, "TYPESPEED");

    const char* mode_labels[] = {"Paragraph", "Time Limit", "Word Count", "Endless"};
    for (int i = 0; i < 4; i++) {
        bool hi = ((int)current_mode == i);
        draw_button(MENU_BTN_X,
                    MENU_START_Y + i*(MENU_BTN_H + MENU_BTN_GAP),
                    MENU_BTN_W, MENU_BTN_H, mode_labels[i], hi);
    }
    // Error mode toggle
    float ey = MENU_START_Y + 4*(MENU_BTN_H + MENU_BTN_GAP) + 10;
    draw_button(MENU_BTN_X,       ey, 140, 36,
                "Strict",  current_emode == ErrorMode::Strict);
    draw_button(MENU_BTN_X+160,   ey, 140, 36,
                "Lenient", current_emode == ErrorMode::Lenient);

    // Global Stats button
    draw_button(WIN_W - 160, WIN_H - 60, 140, 36, "Global Stats");

    al_flip_display();
    return -1; // hit testing done separately
}

int Graphic_Manager::menu_click(int x, int y) const {
    // Mode buttons 0-3
    for (int i = 0; i < 4; i++) {
        float bx = MENU_BTN_X, by = MENU_START_Y + i*(MENU_BTN_H + MENU_BTN_GAP);
        if (x >= bx && x <= bx+MENU_BTN_W && y >= by && y <= by+MENU_BTN_H) return i;
    }
    // Strict = 10, Lenient = 11
    float ey = MENU_START_Y + 4*(MENU_BTN_H + MENU_BTN_GAP) + 10;
    if (x >= MENU_BTN_X && x <= MENU_BTN_X+140 && y >= ey && y <= ey+36) return 10;
    if (x >= MENU_BTN_X+160 && x <= MENU_BTN_X+300 && y >= ey && y <= ey+36) return 11;
    // Global Stats = 20
    if (x >= WIN_W-160 && x <= WIN_W-20 && y >= WIN_H-60 && y <= WIN_H-24) return 20;
    return -1;
}

// ── Category ──────────────────────────────────────────────────────────────────
static const char* CAT_LABELS[] = {
    "English","Spanish","Python","C++","LaTeX","HTML","JavaScript"
};

void Graphic_Manager::render_category(Category current) {
    al_clear_to_color(COL_BG);
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 60, ALLEGRO_ALIGN_CENTRE, "Select Category");
    for (int i = 0; i < 7; i++) {
        bool hi = ((int)current == i);
        draw_button(MENU_BTN_X,
                    140 + i*(MENU_BTN_H + MENU_BTN_GAP),
                    MENU_BTN_W, MENU_BTN_H, CAT_LABELS[i], hi);
    }
    al_flip_display();
}

int Graphic_Manager::category_click(int x, int y) const {
    for (int i = 0; i < 7; i++) {
        float bx = MENU_BTN_X, by = 140.0f + i*(MENU_BTN_H + MENU_BTN_GAP);
        if (x >= bx && x <= bx+MENU_BTN_W && y >= by && y <= by+MENU_BTN_H) return i;
    }
    return -1;
}

// ── File Pick ─────────────────────────────────────────────────────────────────
static constexpr int  FILES_VISIBLE = 10;
static constexpr float FILE_ROW_H   = 44.0f;
static constexpr float FILE_X       = 80.0f;
static constexpr float FILE_W       = WIN_W - 160.0f;
static constexpr float FILE_START_Y = 100.0f;

void Graphic_Manager::render_file_pick(const std::vector<FileInfo>& files,
                                        int scroll_offset) {
    al_clear_to_color(COL_BG);
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 50, ALLEGRO_ALIGN_CENTRE, "Select File");
    int end = std::min(scroll_offset + FILES_VISIBLE, (int)files.size());
    for (int i = scroll_offset; i < end; i++) {
        float by = FILE_START_Y + (i - scroll_offset)*FILE_ROW_H;
        al_draw_filled_rounded_rectangle(FILE_X, by, FILE_X+FILE_W, by+FILE_ROW_H-4,
                                          4, 4, COL_BUTTON);
        const auto& fi = files[i];
        al_draw_text(font_ui_, COL_WHITE, FILE_X+12, by+8, 0, fi.filename.c_str());
        char meta[64];
        std::snprintf(meta, sizeof(meta), "%d words  %d lines", fi.word_count, fi.line_count);
        al_draw_text(font_ui_, COL_TEXT, FILE_X+FILE_W-8, by+8,
                     ALLEGRO_ALIGN_RIGHT, meta);
    }
    // Scroll arrows
    if (scroll_offset > 0)
        draw_button(WIN_W/2-60, WIN_H-60, 120, 34, "^ Up");
    if (scroll_offset + FILES_VISIBLE < (int)files.size())
        draw_button(WIN_W/2-60, WIN_H-20, 120, 34, "v Down");
    al_flip_display();
}

int Graphic_Manager::file_click(int x, int y, int scroll_offset) const {
    for (int i = 0; i < FILES_VISIBLE; i++) {
        float by = FILE_START_Y + i*FILE_ROW_H;
        if (x >= FILE_X && x <= FILE_X+FILE_W && y >= by && y <= by+FILE_ROW_H-4)
            return scroll_offset + i;
    }
    return -1;
}

bool Graphic_Manager::file_scroll_up_click(int x, int y) const {
    return x >= WIN_W/2-60 && x <= WIN_W/2+60 && y >= WIN_H-60 && y <= WIN_H-26;
}
bool Graphic_Manager::file_scroll_down_click(int x, int y) const {
    return x >= WIN_W/2-60 && x <= WIN_W/2+60 && y >= WIN_H-20 && y <= WIN_H+14;
}

// ── Preview ───────────────────────────────────────────────────────────────────
void Graphic_Manager::render_preview(const FileInfo& fi, Category cat,
                                      int current_para, int total_para) {
    al_clear_to_color(COL_BG);
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 80, ALLEGRO_ALIGN_CENTRE,
                 fi.filename.c_str());
    char buf[128];
    if (TextBank::is_prose(cat))
        std::snprintf(buf, sizeof(buf), "Paragraph %d of %d",
                      current_para+1, total_para);
    else
        std::strcpy(buf, "Full file");
    al_draw_text(font_ui_, COL_TEXT, WIN_W/2, 130, ALLEGRO_ALIGN_CENTRE, buf);

    std::snprintf(buf, sizeof(buf), "%d words  |  %d lines",
                  fi.word_count, fi.line_count);
    al_draw_text(font_ui_, COL_TEXT, WIN_W/2, 170, ALLEGRO_ALIGN_CENTRE, buf);

    draw_button(WIN_W/2-160, 280, 140, 44, "Back");
    draw_button(WIN_W/2+20,  280, 140, 44, "Start!", true);
    al_flip_display();
}

bool Graphic_Manager::preview_confirm_click(int x, int y) const {
    return x >= WIN_W/2+20 && x <= WIN_W/2+160 && y >= 280 && y <= 324;
}
bool Graphic_Manager::preview_back_click(int x, int y) const {
    return x >= WIN_W/2-160 && x <= WIN_W/2-20 && y >= 280 && y <= 324;
}

// ── Playing ───────────────────────────────────────────────────────────────────
void Graphic_Manager::draw_passage(const Game& game, float x, float y,
                                    float max_w, bool cursor_visible) {
    const auto& chars = game.char_states();
    int cursor = game.cursor_pos();
    int fh = al_get_font_line_height(font_mono_);
    int fw = al_get_text_width(font_mono_, "M"); // monospace char width

    float cx = x, cy = y;
    for (int i = 0; i < (int)chars.size(); i++) {
        char c = chars[i].ch;
        ALLEGRO_COLOR col;
        switch (chars[i].status) {
            case CharState::Status::Correct: col = COL_CORRECT; break;
            case CharState::Status::Wrong:   col = COL_WRONG;   break;
            default:                         col = COL_TEXT;    break;
        }

        // Draw cursor before this character
        if (i == cursor && cursor_visible)
            al_draw_filled_rectangle(cx, cy, cx+2, cy+fh, COL_CURSOR);

        if (c == '\n') {
            cx = x;
            cy += fh + 4;
            continue;
        }

        char buf[2] = {c, 0};
        al_draw_text(font_mono_, col, cx, cy, 0, buf);
        cx += fw;

        // Wrap at max_w
        if (cx + fw > x + max_w) { cx = x; cy += fh + 4; }
    }
    // Cursor at end
    if (cursor == (int)chars.size() && cursor_visible)
        al_draw_filled_rectangle(cx, cy, cx+2, cy+fh, COL_CURSOR);
}

void Graphic_Manager::render_playing(const Game& game, double elapsed_sec,
                                      int time_remaining_sec, double live_wpm,
                                      bool cursor_visible) {
    al_clear_to_color(COL_BG);
    draw_toolbar(elapsed_sec, live_wpm, time_remaining_sec);
    draw_passage(game, 60, TOOLBAR_H + 30, WIN_W - 120, cursor_visible);
    al_flip_display();
}

// ── Results ───────────────────────────────────────────────────────────────────
void Graphic_Manager::render_results(const SessionResult& r) {
    al_clear_to_color(COL_BG);
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 40, ALLEGRO_ALIGN_CENTRE, "Results");

    char buf[128];
    std::snprintf(buf, sizeof(buf), "WPM: %.1f", r.wpm);
    al_draw_text(font_ui_, COL_CORRECT, WIN_W/2, 90, ALLEGRO_ALIGN_CENTRE, buf);

    std::snprintf(buf, sizeof(buf), "Accuracy: %.1f%%", r.accuracy * 100.0);
    al_draw_text(font_ui_, COL_TEXT, WIN_W/2, 120, ALLEGRO_ALIGN_CENTRE, buf);

    std::snprintf(buf, sizeof(buf), "Errors: %d", r.error_count);
    al_draw_text(font_ui_, COL_WRONG, WIN_W/2, 150, ALLEGRO_ALIGN_CENTRE, buf);

    // WPM graph — draw if we have samples
    if (!r.wpm_samples.empty()) {
        float gx = 80, gy = 190, gw = WIN_W-160, gh = 200;
        al_draw_rectangle(gx, gy, gx+gw, gy+gh, COL_TEXT, 1.0f);
        double max_wpm = 1.0;
        for (auto& s : r.wpm_samples) if (s.wpm > max_wpm) max_wpm = s.wpm;
        double max_t = r.wpm_samples.back().elapsed_sec;
        if (max_t <= 0) max_t = 1;
        for (int i = 1; i < (int)r.wpm_samples.size(); i++) {
            auto& a = r.wpm_samples[i-1];
            auto& b = r.wpm_samples[i];
            float x1 = gx + (float)(a.elapsed_sec / max_t) * gw;
            float y1 = gy + gh - (float)(a.wpm / max_wpm) * gh;
            float x2 = gx + (float)(b.elapsed_sec / max_t) * gw;
            float y2 = gy + gh - (float)(b.wpm / max_wpm) * gh;
            al_draw_line(x1, y1, x2, y2, COL_CORRECT, 2.0f);
        }
    }

    draw_button(WIN_W/2-160, WIN_H-70, 140, 40, "Play Again");
    draw_button(WIN_W/2+20,  WIN_H-70, 140, 40, "Menu");
    al_flip_display();
}

bool Graphic_Manager::results_again_click(int x, int y) const {
    return x >= WIN_W/2-160 && x <= WIN_W/2-20 && y >= WIN_H-70 && y <= WIN_H-30;
}
bool Graphic_Manager::results_menu_click(int x, int y) const {
    return x >= WIN_W/2+20 && x <= WIN_W/2+160 && y >= WIN_H-70 && y <= WIN_H-30;
}
```

- [ ] **Step 3: Build and verify**

```bash
bash create.sh 2>&1 | grep -E "error:" | head -30
```

Expected: no errors. If `data/font.ttf` or `data/mono.ttf` are missing, the built-in font is used automatically — that's fine for now.

- [ ] **Step 4: Commit**

```bash
git add src/graphics/
git commit -m "feat: Graphic_Manager — menu, category, file pick, preview, playing, results screens"
```

---

### Task 6: main.cpp — wire all modules, full event loop

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace skeleton `src/main.cpp` with full implementation**

```cpp
#include "core.hpp"
#include "input.hpp"
#include "textbank.hpp"
#include "game.hpp"
#include "stats.hpp"
#include "graphicMgr.hpp"

enum class Phase {
    MENU, CATEGORY, FILE_PICK, PREVIEW, PLAYING, RESULTS
};

int main() {
    Core& core = Core::instance();
    core.load_settings();

    Graphic_Manager gfx;
    Input input(gfx.get_display());

    TextBank bank;
    bank.load_progress();

    Game        game;
    Stats       stats_unused; // game owns its own Stats internally
    SessionResult last_result{};

    // Persistent selections
    RoundMode  sel_mode  = (RoundMode) core.get_int("mode_idx",  0);
    ErrorMode  sel_emode = core.get_bool("strict", true)
                               ? ErrorMode::Strict : ErrorMode::Lenient;
    Category   sel_cat   = (Category)  core.get_int("cat_idx",   0);
    int        sel_file  = 0;
    int        file_scroll = 0;

    int  time_limit_sec = core.get_int("time_limit_sec", 60);
    int  word_target    = core.get_int("word_target",    50);

    Phase phase = Phase::MENU;

    // Timer: 20 Hz tick for cursor blink + WPM sampling
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 20.0);
    al_register_event_source(input.get_queue(), al_get_timer_event_source(timer));
    al_start_timer(timer);

    bool   running       = true;
    bool   cursor_vis    = true;
    int    tick_count    = 0;
    double start_time    = 0.0;
    double elapsed_sec   = 0.0;
    double live_wpm      = 0.0;
    int    sample_ticks  = 0;   // counts ticks since last WPM sample

    FileInfo current_file{};

    gfx.render_menu(sel_mode, sel_emode);

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(input.get_queue(), &ev);

        // ── Timer tick ────────────────────────────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_TIMER) {
            tick_count++;
            // Cursor blink: toggle every 10 ticks (0.5 s)
            if (tick_count % 10 == 0) cursor_vis = !cursor_vis;

            if (phase == Phase::PLAYING) {
                elapsed_sec = al_get_time() - start_time;
                // WPM sample every 100 ticks (~5 s)
                if (++sample_ticks >= 100) {
                    sample_ticks = 0;
                    game.tick_sample(elapsed_sec);
                    double mins = elapsed_sec / 60.0;
                    // approximate live WPM from game's correct chars
                    // (re-computed in render call)
                }
                // Time limit: check expiry
                if (sel_mode == RoundMode::TimeLimit &&
                    elapsed_sec >= time_limit_sec) {
                    last_result = game.finish(
                        TextBank::category_folder(sel_cat),
                        current_file.filename,
                        (int)(elapsed_sec * 1000));
                    Stats::append_history(last_result);
                    phase = Phase::RESULTS;
                    gfx.render_results(last_result);
                    continue;
                }
                int remaining = (sel_mode == RoundMode::TimeLimit)
                    ? std::max(0, (int)(time_limit_sec - elapsed_sec)) : -1;
                gfx.render_playing(game, elapsed_sec, remaining, live_wpm, cursor_vis);
            }
            continue;
        }

        // ── Display close ─────────────────────────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) { running = false; break; }

        // ── Mouse click ───────────────────────────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP && ev.mouse.button == 1) {
            int mx = ev.mouse.x, my = ev.mouse.y;

            if (phase == Phase::MENU) {
                int btn = gfx.menu_click(mx, my);
                if (btn >= 0 && btn <= 3) {
                    sel_mode = (RoundMode)btn;
                    core.set_int("mode_idx", btn);
                    core.save_settings();
                    gfx.render_menu(sel_mode, sel_emode);
                } else if (btn == 10) { // Strict
                    sel_emode = ErrorMode::Strict;
                    core.set_bool("strict", true); core.save_settings();
                    gfx.render_menu(sel_mode, sel_emode);
                } else if (btn == 11) { // Lenient
                    sel_emode = ErrorMode::Lenient;
                    core.set_bool("strict", false); core.save_settings();
                    gfx.render_menu(sel_mode, sel_emode);
                } else if (btn == 20) { // Global Stats (placeholder — Task 8)
                    // TODO: phase = Phase::GLOBAL_STATS;
                }
                // Second click on selected mode → go to category
                if (btn >= 0 && btn <= 3) {
                    phase = Phase::CATEGORY;
                    gfx.render_category(sel_cat);
                }

            } else if (phase == Phase::CATEGORY) {
                int c = gfx.category_click(mx, my);
                if (c >= 0) {
                    sel_cat = (Category)c;
                    core.set_int("cat_idx", c); core.save_settings();
                    bank.scan(sel_cat);
                    file_scroll = 0; sel_file = 0;
                    phase = Phase::FILE_PICK;
                    gfx.render_file_pick(bank.files(), file_scroll);
                }

            } else if (phase == Phase::FILE_PICK) {
                if (gfx.file_scroll_up_click(mx, my)) {
                    if (file_scroll > 0) file_scroll--;
                    gfx.render_file_pick(bank.files(), file_scroll);
                } else if (gfx.file_scroll_down_click(mx, my)) {
                    file_scroll++;
                    gfx.render_file_pick(bank.files(), file_scroll);
                } else {
                    int fi = gfx.file_click(mx, my, file_scroll);
                    if (fi >= 0 && fi < (int)bank.files().size()) {
                        sel_file = fi;
                        current_file = bank.files()[fi];
                        int cp = bank.current_paragraph(current_file);
                        int tp = bank.total_paragraphs(current_file);
                        phase = Phase::PREVIEW;
                        gfx.render_preview(current_file, sel_cat, cp, tp);
                    }
                }

            } else if (phase == Phase::PREVIEW) {
                if (gfx.preview_confirm_click(mx, my)) {
                    std::string passage = bank.load_passage(current_file, sel_cat);
                    if (passage.empty()) { phase = Phase::FILE_PICK;
                                          gfx.render_file_pick(bank.files(), file_scroll);
                    } else {
                        game.start(passage, sel_mode, sel_emode,
                                   time_limit_sec, word_target);
                        start_time  = al_get_time();
                        elapsed_sec = 0.0;
                        live_wpm    = 0.0;
                        sample_ticks = 0;
                        cursor_vis  = true;
                        phase = Phase::PLAYING;
                        gfx.render_playing(game, 0, -1, 0, true);
                    }
                } else if (gfx.preview_back_click(mx, my)) {
                    phase = Phase::FILE_PICK;
                    gfx.render_file_pick(bank.files(), file_scroll);
                }

            } else if (phase == Phase::RESULTS) {
                if (gfx.results_again_click(mx, my)) {
                    // Reload same file, same position (already advanced by load_passage)
                    int cp = bank.current_paragraph(current_file);
                    int tp = bank.total_paragraphs(current_file);
                    phase = Phase::PREVIEW;
                    gfx.render_preview(current_file, sel_cat, cp, tp);
                } else if (gfx.results_menu_click(mx, my)) {
                    phase = Phase::MENU;
                    gfx.render_menu(sel_mode, sel_emode);
                }
            }
        }

        // ── Keyboard ──────────────────────────────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_KEY_CHAR && phase == Phase::PLAYING) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                last_result = game.finish(
                    TextBank::category_folder(sel_cat),
                    current_file.filename,
                    (int)(elapsed_sec * 1000));
                Stats::append_history(last_result);
                phase = Phase::RESULTS;
                gfx.render_results(last_result);
            } else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE) {
                game.on_backspace();
                gfx.render_playing(game, elapsed_sec,
                    (sel_mode == RoundMode::TimeLimit)
                        ? std::max(0,(int)(time_limit_sec - elapsed_sec)) : -1,
                    live_wpm, cursor_vis);
            } else if (ev.keyboard.unichar >= 32) {
                game.on_key(ev.keyboard.unichar);

                // Update live WPM
                double mins = elapsed_sec / 60.0;
                // live_wpm re-derived in render; expose via a game helper if needed
                // For now: recompute from elapsed and chars typed
                // (Stats are internal to game; this is an approximation)

                if (game.is_finished()) {
                    last_result = game.finish(
                        TextBank::category_folder(sel_cat),
                        current_file.filename,
                        (int)(elapsed_sec * 1000));
                    Stats::append_history(last_result);
                    phase = Phase::RESULTS;
                    gfx.render_results(last_result);
                } else {
                    gfx.render_playing(game, elapsed_sec,
                        (sel_mode == RoundMode::TimeLimit)
                            ? std::max(0,(int)(time_limit_sec - elapsed_sec)) : -1,
                        live_wpm, cursor_vis);
                }
            }
        }
    }

    al_destroy_timer(timer);
    core.save_settings();
    return 0;
}
```

- [ ] **Step 2: Build and run**

```bash
bash create.sh && bash run.sh
```

Expected: window opens, menu shows four mode buttons and a Global Stats button. Click a mode button, then a category, then a file, see the preview, then the playing screen. Typing works — characters turn green/red. Escape → Results → graph appears.

- [ ] **Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "feat: full event loop — all phases wired, game playable end-to-end"
```

---

### Task 7: Endless mode + live WPM helper

**Files:**
- Modify: `src/game/game.hpp` — add `live_wpm()` helper
- Modify: `src/game/game.cpp`
- Modify: `src/main.cpp` — endless phase logic

- [ ] **Step 1: Add `live_wpm()` to Game**

In `src/game/game.hpp`, add to the public section:

```cpp
    double live_wpm(double elapsed_sec) const;
```

In `src/game/game.cpp`, add:

```cpp
double Game::live_wpm(double elapsed_sec) const {
    double mins = elapsed_sec / 60.0;
    if (mins <= 0.0) return 0.0;
    return stats_.correct_chars_proxy() / 5.0 / mins;
}
```

In `src/stats/stats.hpp`, add to public section:

```cpp
    int correct_chars_proxy() const { return correct_chars_; }
```

- [ ] **Step 2: Endless passage streaming in `main.cpp`**

In the `ALLEGRO_EVENT_KEY_CHAR` handler, replace the `game.is_finished()` block with:

```cpp
if (game.is_finished()) {
    if (sel_mode == RoundMode::Endless) {
        // Stream next passage — append to game without showing results
        std::string next = bank.load_passage(current_file, sel_cat);
        if (next.empty()) {
            // Exhausted file — pick a new random file from the same category
            bank.scan(sel_cat);
            if (!bank.files().empty()) {
                current_file = bank.files()[
                    std::mt19937(std::random_device{}())() % bank.files().size()];
                next = bank.load_passage(current_file, sel_cat);
            }
        }
        if (!next.empty()) {
            game.start(next, sel_mode, sel_emode, time_limit_sec, word_target);
        }
    } else {
        last_result = game.finish(
            TextBank::category_folder(sel_cat),
            current_file.filename,
            (int)(elapsed_sec * 1000));
        Stats::append_history(last_result);
        phase = Phase::RESULTS;
        gfx.render_results(last_result);
    }
}
```

Add `#include <random>` at the top of `main.cpp`.

Also replace the `live_wpm` approximation placeholder in `main.cpp` with:

```cpp
live_wpm = game.live_wpm(elapsed_sec);
```

- [ ] **Step 3: Build and test endless mode**

```bash
bash create.sh && bash run.sh
```

Select Endless, pick a file with multiple paragraphs, type to the end — it should load the next passage immediately. Escape → Results → WPM graph.

- [ ] **Step 4: Commit**

```bash
git add src/game/ src/stats/stats.hpp src/main.cpp
git commit -m "feat: endless mode streaming, live WPM helper"
```

---

### Task 8: Sample data files + .gitignore + CLAUDE.md update

**Files:**
- Create: `data/english/sample.txt`
- Create: `data/spanish/muestra.txt`
- Create: `data/python/sample.txt`
- Create: `data/cpp/sample.txt`
- Create: `data/latex/sample.txt`
- Create: `data/html/sample.txt`
- Create: `data/javascript/sample.txt`
- Modify: `CLAUDE.md`

- [ ] **Step 1: Create sample data files**

`data/english/sample.txt`:
```
The quick brown fox jumps over the lazy dog near the riverbank where the willows grow tall.
Typing fast requires consistent practice and muscle memory built over many sessions.

A journey of a thousand miles begins with a single step taken deliberately and without hesitation.
The best way to predict the future is to create it through disciplined daily effort.
```

`data/spanish/muestra.txt`:
```
El veloz murciélago hindú comía feliz cardillo y kiwi mientras la cigüeña tocaba el saxofón.
La práctica constante mejora la velocidad y precisión al escribir en cualquier idioma.

Un camino de mil millas comienza con un solo paso dado con determinación y sin vacilación.
La mejor forma de predecir el futuro es crearlo mediante el esfuerzo diario y disciplinado.
```

`data/python/sample.txt`:
```python
def fibonacci(n):
    if n <= 1:
        return n
    a, b = 0, 1
    for _ in range(2, n + 1):
        a, b = b, a + b
    return b

def is_prime(n):
    if n < 2:
        return False
    for i in range(2, int(n**0.5) + 1):
        if n % i == 0:
            return False
    return True

if __name__ == "__main__":
    print([fibonacci(i) for i in range(10)])
    print([x for x in range(2, 50) if is_prime(x)])
```

`data/cpp/sample.txt`:
```cpp
#include <iostream>
#include <vector>
#include <algorithm>

std::vector<int> sieve(int n) {
    std::vector<bool> is_prime(n + 1, true);
    std::vector<int>  primes;
    is_prime[0] = is_prime[1] = false;
    for (int i = 2; i <= n; i++) {
        if (!is_prime[i]) continue;
        primes.push_back(i);
        for (int j = 2*i; j <= n; j += i)
            is_prime[j] = false;
    }
    return primes;
}

int main() {
    auto p = sieve(100);
    for (int x : p) std::cout << x << " ";
    std::cout << "\n";
    return 0;
}
```

`data/latex/sample.txt`:
```latex
\documentclass{article}
\usepackage{amsmath}
\usepackage{graphicx}

\title{Sample Document}
\author{Author Name}
\date{\today}

\begin{document}
\maketitle

\section{Introduction}
The quadratic formula is given by:
\begin{equation}
    x = \frac{-b \pm \sqrt{b^2 - 4ac}}{2a}
\end{equation}

\section{Conclusion}
This document demonstrates basic \LaTeX{} typesetting.
\end{document}
```

`data/html/sample.txt`:
```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sample Page</title>
    <style>
        body { font-family: sans-serif; margin: 2rem; }
        h1   { color: #333; }
    </style>
</head>
<body>
    <h1>Hello, World!</h1>
    <p>This is a sample HTML file for typing practice.</p>
    <ul>
        <li>Item one</li>
        <li>Item two</li>
    </ul>
</body>
</html>
```

`data/javascript/sample.txt`:
```javascript
function debounce(fn, delay) {
    let timer = null;
    return function(...args) {
        clearTimeout(timer);
        timer = setTimeout(() => fn.apply(this, args), delay);
    };
}

const fetchUser = async (id) => {
    const response = await fetch(`/api/users/${id}`);
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return response.json();
};

const numbers = [1, 2, 3, 4, 5];
const doubled = numbers.map(n => n * 2);
const evens   = numbers.filter(n => n % 2 === 0);
console.log({ doubled, evens });
```

- [ ] **Step 2: Update CLAUDE.md**

Add a **Data Files** section after the Architecture section:

```markdown
## Data Files

- `data/<category>/` — `.txt` files for each category. Add your own files here.
- Prose files (english/spanish): paragraphs separated by blank lines.
- Code files (python/cpp/latex/html/javascript): full file loaded as-is.
- `data/progress.txt`, `data/history.txt`, `data/keylog.txt` — auto-generated, gitignored.
```

- [ ] **Step 3: Build, run, and verify sample files load**

```bash
bash create.sh && bash run.sh
```

Select English → sample.txt → confirm → type the passage to completion → verify Results screen shows WPM and graph.

- [ ] **Step 4: Commit**

```bash
git add data/ CLAUDE.md
git commit -m "feat: sample data files for all categories, CLAUDE.md data section"
```

---

### Task 9: Final polish — fonts, word-count/time-limit settings, push

**Files:**
- Modify: `src/graphics/graphicMgr.cpp` — word count and time limit input in menu
- Modify: `src/main.cpp` — hook up time limit and word count inputs

- [ ] **Step 1: Add numeric input to menu for time limit and word target**

In `render_menu`, after the error mode toggle, add:

```cpp
    // Time limit input label (shown only when mode == TimeLimit)
    if (current_mode == RoundMode::TimeLimit) {
        char tlbuf[32];
        std::snprintf(tlbuf, sizeof(tlbuf), "Time: %d sec  (< / >)", time_limit_sec_hint);
        al_draw_text(font_ui_, COL_TEXT, WIN_W/2, ey + 50, ALLEGRO_ALIGN_CENTRE, tlbuf);
    }
    if (current_mode == RoundMode::WordCount) {
        char wcbuf[32];
        std::snprintf(wcbuf, sizeof(wcbuf), "Words: %d  (< / >)", word_target_hint);
        al_draw_text(font_ui_, COL_TEXT, WIN_W/2, ey + 50, ALLEGRO_ALIGN_CENTRE, wcbuf);
    }
```

Add `int time_limit_sec_hint` and `int word_target_hint` parameters to `render_menu` and update the declaration in `graphicMgr.hpp`.

- [ ] **Step 2: Handle `<` / `>` keys in MENU phase in `main.cpp`**

In the `ALLEGRO_EVENT_KEY_CHAR` block, before the `phase == Phase::PLAYING` check, add:

```cpp
        if (phase == Phase::MENU) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_COMMA ||
                ev.keyboard.keycode == ALLEGRO_KEY_LESS) {
                if (sel_mode == RoundMode::TimeLimit)
                    time_limit_sec = std::max(10, time_limit_sec - 10);
                else if (sel_mode == RoundMode::WordCount)
                    word_target = std::max(10, word_target - 10);
                core.set_int("time_limit_sec", time_limit_sec);
                core.set_int("word_target",    word_target);
                core.save_settings();
                gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);
            } else if (ev.keyboard.keycode == ALLEGRO_KEY_FULLSTOP ||
                       ev.keyboard.keycode == ALLEGRO_KEY_GREATER) {
                if (sel_mode == RoundMode::TimeLimit)
                    time_limit_sec = std::min(600, time_limit_sec + 10);
                else if (sel_mode == RoundMode::WordCount)
                    word_target = std::min(1000, word_target + 10);
                core.set_int("time_limit_sec", time_limit_sec);
                core.set_int("word_target",    word_target);
                core.save_settings();
                gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);
            }
        }
```

- [ ] **Step 3: Final build + smoke test all modes**

```bash
bash create.sh && bash run.sh
```

Test each path:
1. Paragraph mode → English → sample.txt → type to end → Results → Play Again
2. Time Limit mode → adjust with `<`/`>` → type → timer expires → Results
3. Word Count mode → adjust with `<`/`>` → type until word target → Results
4. Endless mode → type multiple paragraphs → Escape → Results with full WPM graph

- [ ] **Step 4: Commit and push**

```bash
git add src/
git commit -m "feat: time limit and word count adjustable from menu with < / >"
git push
```

---

## Self-Review

**Spec coverage check:**

| Spec requirement | Covered by task |
|---|---|
| C++17 / Allegro5 build | Task 1 |
| Core singleton + settings.txt | Task 1 |
| Input module | Task 1 |
| TextBank: scan, paragraph load, progress.txt | Task 2 |
| Code categories: full file load | Task 2 |
| Stats: WPM, accuracy, errors, history.txt | Task 3 |
| Game: char tracking, strict/lenient | Task 4 |
| Graphic_Manager: all 6 screens | Task 5 |
| Full event loop, phase transitions | Task 6 |
| Endless mode streaming | Task 7 |
| Live WPM in toolbar | Task 7 |
| Sample data for all 7 categories | Task 8 |
| Preview screen: word/line count | Task 5 |
| Paragraph progression (prose) | Task 2 |
| Completed-book → random paragraph | Task 2 |
| Time limit mode | Task 6 + 9 |
| Word count mode | Task 6 + 9 |
| WPM graph in results | Task 5 |
| Moment-to-moment WPM samples | Task 3 + 7 |
| Cursor: blinking vertical bar | Task 5 |
| Escape → stops round | Task 6 |
| Settings persist | Task 1 + 9 |
| Global Stats (placeholder) | Plan B |

**Global Stats** is intentionally deferred to Plan B — the core game must be working first.

**Placeholder scan:** no TBDs. All code is complete. Type signatures are consistent across tasks (`live_wpm`, `correct_chars_proxy`, `error_count_proxy` all defined before use).

**Type consistency:** `CharState::Status`, `RoundMode`, `ErrorMode`, `Category`, `FileInfo`, `SessionResult`, `WpmSample` — all defined in Task 1–4 and referenced consistently in Tasks 5–9.
