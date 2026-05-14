#include "graphicMgr.hpp"
#include "core.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

// ── Colours ───────────────────────────────────────────────────────────────────
static const ALLEGRO_COLOR COL_BG       = {0.10f, 0.10f, 0.12f, 1.0f};
static const ALLEGRO_COLOR COL_TOOLBAR  = {0.14f, 0.14f, 0.18f, 1.0f};
static const ALLEGRO_COLOR COL_TEXT     = {0.85f, 0.85f, 0.85f, 1.0f};
static const ALLEGRO_COLOR COL_CORRECT  = {0.35f, 0.85f, 0.45f, 1.0f};
static const ALLEGRO_COLOR COL_WRONG    = {0.90f, 0.25f, 0.25f, 1.0f};
static const ALLEGRO_COLOR COL_CURSOR   = {0.90f, 0.80f, 0.20f, 1.0f};
static const ALLEGRO_COLOR COL_BUTTON   = {0.20f, 0.20f, 0.28f, 1.0f};
static const ALLEGRO_COLOR COL_BUTTON_H = {0.30f, 0.30f, 0.45f, 1.0f};
static const ALLEGRO_COLOR COL_WHITE    = {1.0f,  1.0f,  1.0f,  1.0f};
static const ALLEGRO_COLOR COL_DIM      = {0.55f, 0.55f, 0.60f, 1.0f};

// ── Layout constants ──────────────────────────────────────────────────────────
static constexpr float MENU_BTN_X   = 300.0f;
static constexpr float MENU_BTN_W   = 300.0f;
static constexpr float MENU_BTN_H   =  44.0f;
static constexpr float MENU_BTN_GAP =  14.0f;
static constexpr float MENU_START_Y = 150.0f;

static constexpr int   FILES_VISIBLE = 10;
static constexpr float FILE_ROW_H    =  44.0f;
static constexpr float FILE_X        =  80.0f;
static constexpr float FILE_W        = Graphic_Manager::WIN_W - 160.0f;
static constexpr float FILE_START_Y  = 100.0f;

// ── Transform helpers ─────────────────────────────────────────────────────────

// Clear the entire display (in screen space) to COL_BG, then restore the game transform.
// This eliminates letterbox bars — the background colour fills the whole window.
void Graphic_Manager::clear_full() {
    ALLEGRO_TRANSFORM identity;
    al_identity_transform(&identity);
    al_use_transform(&identity);
    clear_full();
    // Restore game transform
    ALLEGRO_TRANSFORM t;
    al_identity_transform(&t);
    al_scale_transform(&t, transform_scale_, transform_scale_);
    al_translate_transform(&t, transform_ox_, transform_oy_);
    al_use_transform(&t);
}

void Graphic_Manager::update_transform() {
    int dw = al_get_display_width(display_);
    int dh = al_get_display_height(display_);

    float scale_x = (float)dw / WIN_W;
    float scale_y = (float)dh / WIN_H;
    float scale   = std::min(scale_x, scale_y); // letterbox: fit whole game

    float ox = ((float)dw - WIN_W * scale) / 2.0f;
    float oy = ((float)dh - WIN_H * scale) / 2.0f;

    ALLEGRO_TRANSFORM t;
    al_identity_transform(&t);
    al_scale_transform(&t, scale, scale);
    al_translate_transform(&t, ox, oy);
    al_use_transform(&t);

    // Store offset so hit-tests can un-transform mouse coords
    transform_ox_ = ox;
    transform_oy_ = oy;
    transform_scale_ = scale;
}

// ── Constructor / Destructor ──────────────────────────────────────────────────
Graphic_Manager::Graphic_Manager() {
    Core& core = Core::instance();
    fullscreen_ = core.get_bool("fullscreen", false);

    al_set_new_display_flags(ALLEGRO_RESIZABLE |
                             (fullscreen_ ? ALLEGRO_FULLSCREEN_WINDOW : 0));
    display_ = al_create_display(WIN_W, WIN_H);
    al_set_new_display_flags(0);
    if (!display_) std::exit(1);
    al_set_window_title(display_, "Typespeed");
    update_transform();

    // Bundled fonts take priority; fall back to system Ubuntu fonts, then builtin
    font_ui_ = al_load_ttf_font("data/font.ttf", 18, 0);
    if (!font_ui_)
        font_ui_ = al_load_ttf_font(
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 18, 0);
    if (!font_ui_)
        font_ui_ = al_create_builtin_font();

    font_mono_ = al_load_ttf_font("data/mono.ttf", 16, 0);
    if (!font_mono_)
        font_mono_ = al_load_ttf_font(
            "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf", 16, 0);
    if (!font_mono_)
        font_mono_ = al_create_builtin_font();
}

void Graphic_Manager::toggle_fullscreen() {
    fullscreen_ = !fullscreen_;
    al_set_display_flag(display_, ALLEGRO_FULLSCREEN_WINDOW, fullscreen_);
    al_set_target_backbuffer(display_);
    update_transform();
}

void Graphic_Manager::toggle_maximized() {
    if (fullscreen_) return;
    int flags = al_get_display_flags(display_);
    bool currently_maximized = (flags & ALLEGRO_MAXIMIZED) != 0;
    al_set_display_flag(display_, ALLEGRO_MAXIMIZED, !currently_maximized);
    al_set_target_backbuffer(display_);
    update_transform();
}

void Graphic_Manager::draw_window_chrome() {
    // No-op — background is drawn full-display by clear_full(), no bars needed.
}

bool Graphic_Manager::maximize_btn_click(int x, int y) const {
    // No custom maximize button — OS titlebar handles this now
    (void)x; (void)y;
    return false;
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
    al_draw_rounded_rectangle(x, y, x+w, y+h, 6, 6, COL_DIM, 1.0f);
    float fh = (float)al_get_font_line_height(font_ui_);
    al_draw_text(font_ui_, COL_WHITE, x + w/2.0f, y + h/2.0f - fh/2.0f,
                 ALLEGRO_ALIGN_CENTRE, label.c_str());
}

void Graphic_Manager::draw_toolbar(double elapsed_sec, double wpm,
                                    int time_remaining_sec) {
    al_draw_filled_rectangle(0, 0, WIN_W, TOOLBAR_H, COL_TOOLBAR);
    char buf[64];
    int mins = (int)elapsed_sec / 60;
    int secs = (int)elapsed_sec % 60;
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
void Graphic_Manager::render_menu(RoundMode current_mode, ErrorMode current_emode,
                                   int time_limit_sec, int word_target) {
    clear_full();

    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 70, ALLEGRO_ALIGN_CENTRE, "TYPESPEED");

    const char* mode_labels[] = {"Paragraph", "Time Limit", "Word Count", "Endless"};
    for (int i = 0; i < 4; i++) {
        bool hi = ((int)current_mode == i);
        draw_button(MENU_BTN_X,
                    MENU_START_Y + (float)i*(MENU_BTN_H + MENU_BTN_GAP),
                    MENU_BTN_W, MENU_BTN_H, mode_labels[i], hi);
    }

    float ey = MENU_START_Y + 4*(MENU_BTN_H + MENU_BTN_GAP) + 10;
    draw_button(MENU_BTN_X,       ey, 140, 36, "Strict",
                current_emode == ErrorMode::Strict);
    draw_button(MENU_BTN_X+160,   ey, 140, 36, "Lenient",
                current_emode == ErrorMode::Lenient);

    // Show adjustable param hint for time/word modes
    char hint[64] = "";
    if (current_mode == RoundMode::TimeLimit)
        std::snprintf(hint, sizeof(hint), "Time: %d sec  (< / >)", time_limit_sec);
    else if (current_mode == RoundMode::WordCount)
        std::snprintf(hint, sizeof(hint), "Words: %d  (< / >)", word_target);
    if (hint[0])
        al_draw_text(font_ui_, COL_DIM, WIN_W/2, ey + 46, ALLEGRO_ALIGN_CENTRE, hint);

    // Start button — clicking any mode button goes to category; this is a shortcut
    draw_button(MENU_BTN_X, ey + 80, MENU_BTN_W, 44, "Start", true);

    // Global Stats
    draw_button(WIN_W - 160, WIN_H - 60, 140, 36, "Global Stats");

    draw_window_chrome();
    al_flip_display();
}

int Graphic_Manager::menu_click(int x, int y) const {
    float fx = (float)x, fy = (float)y;
    for (int i = 0; i < 4; i++) {
        float bx = MENU_BTN_X, by = MENU_START_Y + (float)i*(MENU_BTN_H + MENU_BTN_GAP);
        if (fx >= bx && fx <= bx+MENU_BTN_W && fy >= by && fy <= by+MENU_BTN_H) return i;
    }
    float ey = MENU_START_Y + 4*(MENU_BTN_H + MENU_BTN_GAP) + 10;
    if (fx >= MENU_BTN_X && fx <= MENU_BTN_X+140 && fy >= ey && fy <= ey+36) return 10;
    if (fx >= MENU_BTN_X+160 && fx <= MENU_BTN_X+300 && fy >= ey && fy <= ey+36) return 11;
    float sy = ey + 80;
    if (fx >= MENU_BTN_X && fx <= MENU_BTN_X+MENU_BTN_W && fy >= sy && fy <= sy+44) return 30;
    if (fx >= (float)(WIN_W-160) && fx <= (float)(WIN_W-20) && fy >= (float)(WIN_H-60) && fy <= (float)(WIN_H-24)) return 20;
    return -1;
}

// ── Category ──────────────────────────────────────────────────────────────────
static const char* CAT_LABELS[] = {
    "English","Spanish","Python","C++","LaTeX","HTML","JavaScript"
};

void Graphic_Manager::render_category(Category current) {
    clear_full();
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 60, ALLEGRO_ALIGN_CENTRE, "Select Category");
    for (int i = 0; i < 7; i++) {
        bool hi = ((int)current == i);
        draw_button(MENU_BTN_X,
                    120.0f + (float)i*(MENU_BTN_H + MENU_BTN_GAP),
                    MENU_BTN_W, MENU_BTN_H, CAT_LABELS[i], hi);
    }
    draw_window_chrome();
    al_flip_display();
}

int Graphic_Manager::category_click(int x, int y) const {
    float fx = (float)x, fy = (float)y;
    for (int i = 0; i < 7; i++) {
        float bx = MENU_BTN_X, by = 120.0f + (float)i*(MENU_BTN_H + MENU_BTN_GAP);
        if (fx >= bx && fx <= bx+MENU_BTN_W && fy >= by && fy <= by+MENU_BTN_H) return i;
    }
    return -1;
}

// ── File Pick ─────────────────────────────────────────────────────────────────
void Graphic_Manager::render_file_pick(const std::vector<FileInfo>& files,
                                        int scroll_offset) {
    clear_full();
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 50, ALLEGRO_ALIGN_CENTRE, "Select File");

    if (files.empty()) {
        al_draw_text(font_ui_, COL_DIM, WIN_W/2, 200, ALLEGRO_ALIGN_CENTRE,
                     "No .txt files found in data folder.");
        draw_window_chrome();
    al_flip_display();
        return;
    }

    int end = std::min(scroll_offset + FILES_VISIBLE, (int)files.size());
    for (int i = scroll_offset; i < end; i++) {
        float by = FILE_START_Y + (float)(i - scroll_offset)*FILE_ROW_H;
        al_draw_filled_rounded_rectangle(FILE_X, by, FILE_X+FILE_W, by+FILE_ROW_H-4,
                                          4, 4, COL_BUTTON);
        const auto& fi = files[(size_t)i];
        al_draw_text(font_ui_, COL_WHITE, FILE_X+12, by+12, 0, fi.filename.c_str());
        char meta[64];
        std::snprintf(meta, sizeof(meta), "%d words  %d lines",
                      fi.word_count, fi.line_count);
        al_draw_text(font_ui_, COL_DIM, FILE_X+FILE_W-8, by+12,
                     ALLEGRO_ALIGN_RIGHT, meta);
    }

    if (scroll_offset > 0)
        draw_button(WIN_W/2-60, WIN_H-70, 120, 34, "^ Up");
    if (scroll_offset + FILES_VISIBLE < (int)files.size())
        draw_button(WIN_W/2-60, WIN_H-30, 120, 34, "v Down");

    draw_window_chrome();
    al_flip_display();
}

int Graphic_Manager::file_click(int x, int y, int scroll_offset) const {
    float fx = (float)x, fy = (float)y;
    for (int i = 0; i < FILES_VISIBLE; i++) {
        float by = FILE_START_Y + (float)i*FILE_ROW_H;
        if (fx >= FILE_X && fx <= FILE_X+FILE_W && fy >= by && fy <= by+FILE_ROW_H-4)
            return scroll_offset + i;
    }
    return -1;
}

bool Graphic_Manager::file_scroll_up_click(int x, int y) const {
    return x >= WIN_W/2-60 && x <= WIN_W/2+60 && y >= WIN_H-70 && y <= WIN_H-36;
}
bool Graphic_Manager::file_scroll_down_click(int x, int y) const {
    return x >= WIN_W/2-60 && x <= WIN_W/2+60 && y >= WIN_H-30 && y <= WIN_H-4;
}

// ── Preview ───────────────────────────────────────────────────────────────────
void Graphic_Manager::render_preview(const FileInfo& fi, Category cat,
                                      int current_para, int total_para) {
    clear_full();
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
        float bx = ((float)WIN_W - total_w) / 2.0f;
        float by = 290.0f;
        draw_button(bx,            by, bw, bh, "Redo Last");
        draw_button(bx+bw+gap,     by, bw, bh, "Skip");
        draw_button(bx+2*(bw+gap), by, bw, bh, "Restart");
        draw_button(bx+3*(bw+gap), by, bw, bh, "Jump To...");
    }

    draw_window_chrome();
    al_flip_display();
}

bool Graphic_Manager::preview_confirm_click(int x, int y) const {
    return x >= WIN_W/2+20 && x <= WIN_W/2+160 && y >= 220 && y <= 264;
}
bool Graphic_Manager::preview_back_click(int x, int y) const {
    return x >= WIN_W/2-160 && x <= WIN_W/2-20 && y >= 220 && y <= 264;
}

// Shared helper — not exposed in header, used only in this file
static bool in_rect(float fx, float fy, float bx, float by, float bw, float bh) {
    return fx >= bx && fx <= bx+bw && fy >= by && fy <= by+bh;
}

static constexpr float PREV_BTN_W   = 130.0f;
static constexpr float PREV_BTN_H   =  36.0f;
static constexpr float PREV_BTN_GAP =  10.0f;
static constexpr float PREV_BTN_Y   = 290.0f;

static float prev_nav_x(int idx) {
    float total_w = 4*PREV_BTN_W + 3*PREV_BTN_GAP;
    float bx = ((float)Graphic_Manager::WIN_W - total_w) / 2.0f;
    return bx + (float)idx * (PREV_BTN_W + PREV_BTN_GAP);
}

bool Graphic_Manager::preview_redo_click(int x, int y) const {
    return in_rect((float)x, (float)y, prev_nav_x(0), PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H);
}
bool Graphic_Manager::preview_skip_click(int x, int y) const {
    return in_rect((float)x, (float)y, prev_nav_x(1), PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H);
}
bool Graphic_Manager::preview_restart_click(int x, int y) const {
    return in_rect((float)x, (float)y, prev_nav_x(2), PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H);
}
bool Graphic_Manager::preview_jump_click(int x, int y) const {
    return in_rect((float)x, (float)y, prev_nav_x(3), PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H);
}

// ── Jump overlay ─────────────────────────────────────────────────────────────
static constexpr float JUMP_W = 340.0f;
static constexpr float JUMP_H = 160.0f;
static constexpr float JUMP_X = ((float)Graphic_Manager::WIN_W - JUMP_W) / 2.0f;
static constexpr float JUMP_Y = ((float)Graphic_Manager::WIN_H - JUMP_H) / 2.0f;

void Graphic_Manager::render_jump_overlay(int current_para, int total_para,
                                           const std::string& input) {
    // Dim background
    al_draw_filled_rectangle(0, 0, (float)WIN_W, (float)WIN_H, {0,0,0,0.55f});
    // Panel
    al_draw_filled_rounded_rectangle(JUMP_X, JUMP_Y, JUMP_X+JUMP_W, JUMP_Y+JUMP_H,
                                      8, 8, {0.15f,0.15f,0.20f,1.0f});
    al_draw_rounded_rectangle(JUMP_X, JUMP_Y, JUMP_X+JUMP_W, JUMP_Y+JUMP_H,
                               8, 8, COL_DIM, 1.5f);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Jump to paragraph (1 - %d):", total_para);
    al_draw_text(font_ui_, COL_TEXT, JUMP_X+JUMP_W/2.0f, JUMP_Y+18.0f,
                 ALLEGRO_ALIGN_CENTRE, buf);

    // Input box
    float ix = JUMP_X+20, iy = JUMP_Y+52, iw = JUMP_W-40, ih = 36;
    al_draw_filled_rounded_rectangle(ix, iy, ix+iw, iy+ih, 4, 4, {0.08f,0.08f,0.10f,1.0f});
    al_draw_rounded_rectangle(ix, iy, ix+iw, iy+ih, 4, 4, COL_DIM, 1.0f);
    std::string display = input.empty() ? std::to_string(current_para + 1) : input;
    al_draw_text(font_ui_, input.empty() ? COL_DIM : COL_WHITE,
                 ix+iw/2.0f, iy+8.0f, ALLEGRO_ALIGN_CENTRE, display.c_str());

    draw_button(JUMP_X+20,          JUMP_Y+106, 130, 36, "Cancel");
    draw_button(JUMP_X+JUMP_W-150,  JUMP_Y+106, 130, 36, "Go!", true);
    draw_window_chrome();
    al_flip_display();
}

bool Graphic_Manager::jump_confirm_click(int x, int y) const {
    return in_rect((float)x, (float)y, JUMP_X+JUMP_W-150, JUMP_Y+106, 130, 36);
}
bool Graphic_Manager::jump_cancel_click(int x, int y) const {
    return in_rect((float)x, (float)y, JUMP_X+20, JUMP_Y+106, 130, 36);
}

// ── Playing ───────────────────────────────────────────────────────────────────
void Graphic_Manager::draw_passage(const Game& game, float x, float y,
                                    float max_w, bool cursor_visible) {
    const auto& chars = game.char_states();
    int cursor = game.cursor_pos();
    float fh = (float)al_get_font_line_height(font_mono_);
    float fw = (float)al_get_text_width(font_mono_, "M");

    float cx = x, cy = y;
    for (int i = 0; i <= (int)chars.size(); i++) {
        // Draw cursor before this position
        if (i == cursor && cursor_visible)
            al_draw_filled_rectangle(cx, cy, cx + 2.0f, cy + fh, COL_CURSOR);


        if (i == (int)chars.size()) break;

        char c = chars[(size_t)i].ch;
        ALLEGRO_COLOR col;
        switch (chars[(size_t)i].status) {
            case CharState::Status::Correct: col = COL_CORRECT; break;
            case CharState::Status::Wrong:   col = COL_WRONG;   break;
            default:                         col = COL_TEXT;    break;
        }

        if (c == '\n') {
            cx = x;
            cy += fh + 4;
            continue;
        }

        char buf[2] = {c, 0};
        al_draw_text(font_mono_, col, cx, cy, 0, buf);
        cx += fw;

        if (cx + fw > x + max_w) { cx = x; cy += fh + 4; }
    }
}

void Graphic_Manager::render_playing(const Game& game, double elapsed_sec,
                                      int time_remaining_sec, double live_wpm,
                                      bool cursor_visible) {
    clear_full();
    draw_toolbar(elapsed_sec, live_wpm, time_remaining_sec);
    draw_passage(game, 60, TOOLBAR_H + 30, WIN_W - 120, cursor_visible);
    draw_window_chrome();
    al_flip_display();
}

// ── Results ───────────────────────────────────────────────────────────────────
void Graphic_Manager::render_results(const SessionResult& r) {
    clear_full();
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 30, ALLEGRO_ALIGN_CENTRE, "Results");

    char buf[128];
    std::snprintf(buf, sizeof(buf), "%.1f WPM", r.wpm);
    al_draw_text(font_ui_, COL_CORRECT, WIN_W/2, 80, ALLEGRO_ALIGN_CENTRE, buf);

    std::snprintf(buf, sizeof(buf), "Accuracy: %.1f%%", r.accuracy * 100.0);
    al_draw_text(font_ui_, COL_TEXT, WIN_W/2, 110, ALLEGRO_ALIGN_CENTRE, buf);

    std::snprintf(buf, sizeof(buf), "Errors: %d", r.error_count);
    al_draw_text(font_ui_, COL_WRONG, WIN_W/2, 140, ALLEGRO_ALIGN_CENTRE, buf);

    // WPM graph
    if (!r.wpm_samples.empty()) {
        float gx = 80, gy = 175, gw = WIN_W - 160, gh = 280;
        al_draw_filled_rectangle(gx, gy, gx+gw, gy+gh, {0.07f,0.07f,0.09f,1.0f});
        al_draw_rectangle(gx, gy, gx+gw, gy+gh, COL_DIM, 1.0f);

        double max_wpm = 1.0;
        for (auto& s : r.wpm_samples) if (s.wpm > max_wpm) max_wpm = s.wpm;
        double max_t = r.wpm_samples.back().elapsed_sec;
        if (max_t <= 0) max_t = 1;

        for (int i = 1; i < (int)r.wpm_samples.size(); i++) {
            auto& a = r.wpm_samples[(size_t)(i-1)];
            auto& b = r.wpm_samples[(size_t)i];
            float x1 = gx + (float)(a.elapsed_sec / max_t) * gw;
            float y1 = gy + gh - (float)(a.wpm / max_wpm) * gh;
            float x2 = gx + (float)(b.elapsed_sec / max_t) * gw;
            float y2 = gy + gh - (float)(b.wpm / max_wpm) * gh;
            al_draw_line(x1, y1, x2, y2, COL_CORRECT, 2.0f);
        }

        // Axis labels
        std::snprintf(buf, sizeof(buf), "%.0f wpm", max_wpm);
        al_draw_text(font_ui_, COL_DIM, gx - 4, gy - 2, ALLEGRO_ALIGN_RIGHT, buf);
        std::snprintf(buf, sizeof(buf), "%.0fs", max_t);
        al_draw_text(font_ui_, COL_DIM, gx+gw, gy+gh+2, ALLEGRO_ALIGN_RIGHT, buf);
    }

    draw_button(WIN_W/2-160, WIN_H-60, 140, 40, "Play Again");
    draw_button(WIN_W/2+20,  WIN_H-60, 140, 40, "Menu");
    draw_window_chrome();
    al_flip_display();
}

bool Graphic_Manager::results_again_click(int x, int y) const {
    return x >= WIN_W/2-160 && x <= WIN_W/2-20 && y >= WIN_H-60 && y <= WIN_H-20;
}
bool Graphic_Manager::results_menu_click(int x, int y) const {
    return x >= WIN_W/2+20 && x <= WIN_W/2+160 && y >= WIN_H-60 && y <= WIN_H-20;
}

// ── Global Stats (Plan B placeholder) ────────────────────────────────────────
void Graphic_Manager::render_global_stats() {
    clear_full();
    al_draw_text(font_ui_, COL_WHITE, WIN_W/2, 60,
                 ALLEGRO_ALIGN_CENTRE, "Global Stats");
    al_draw_text(font_ui_, COL_DIM, WIN_W/2, 160,
                 ALLEGRO_ALIGN_CENTRE, "Coming soon — Plan B.");
    al_draw_text(font_ui_, COL_DIM, WIN_W/2, 200,
                 ALLEGRO_ALIGN_CENTRE, "Keyboard heatmap, bigrams, WPM trends,");
    al_draw_text(font_ui_, COL_DIM, WIN_W/2, 230,
                 ALLEGRO_ALIGN_CENTRE, "badges, lifetime totals and more.");
    draw_button(WIN_W/2-70, WIN_H-80, 140, 44, "Back");
    draw_window_chrome();
    al_flip_display();
}

bool Graphic_Manager::global_stats_back_click(int x, int y) const {
    return x >= WIN_W/2-70 && x <= WIN_W/2+70 && y >= WIN_H-80 && y <= WIN_H-36;
}
