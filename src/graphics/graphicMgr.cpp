#include "graphicMgr.hpp"
#include "core.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

// ── Colours ───────────────────────────────────────────────────────────────────
static const ALLEGRO_COLOR COL_BG       = {0.10f, 0.10f, 0.12f, 1.0f};
static const ALLEGRO_COLOR COL_TOOLBAR  = {0.14f, 0.14f, 0.18f, 1.0f};
static const ALLEGRO_COLOR COL_TEXT     = {0.55f, 0.55f, 0.58f, 1.0f}; // untyped — light gray
static const ALLEGRO_COLOR COL_CORRECT  = {1.0f,  1.0f,  1.0f,  1.0f}; // correct — pure white
static const ALLEGRO_COLOR COL_WRONG    = {0.90f, 0.20f, 0.20f, 1.0f}; // wrong — red
static const ALLEGRO_COLOR COL_CURSOR   = {0.90f, 0.80f, 0.20f, 1.0f};
static const ALLEGRO_COLOR COL_BUTTON   = {0.20f, 0.20f, 0.28f, 1.0f};
static const ALLEGRO_COLOR COL_BUTTON_H = {0.30f, 0.30f, 0.45f, 1.0f};
static const ALLEGRO_COLOR COL_WHITE    = {1.0f,  1.0f,  1.0f,  1.0f};
static const ALLEGRO_COLOR COL_DIM      = {0.45f, 0.45f, 0.50f, 1.0f};

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
    al_clear_to_color(COL_BG);  // must call Allegro directly, not clear_full()
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
    transform_scale_ = std::min(scale_x, scale_y);
    transform_ox_    = ((float)dw - WIN_W * transform_scale_) / 2.0f;
    transform_oy_    = ((float)dh - WIN_H * transform_scale_) / 2.0f;

    // Allegro transform handles all coordinate scaling automatically.
    // Fonts are reloaded at scale*base_px so they're rasterized sharp.
    ALLEGRO_TRANSFORM t;
    al_identity_transform(&t);
    al_scale_transform(&t, transform_scale_, transform_scale_);
    al_translate_transform(&t, transform_ox_, transform_oy_);
    al_use_transform(&t);

    reload_fonts();
}

void Graphic_Manager::reload_fonts() {
    if (font_ui_)   { if (!font_ui_builtin_)   al_destroy_font(font_ui_);   font_ui_   = nullptr; }
    if (font_mono_) { if (!font_mono_builtin_)  al_destroy_font(font_mono_); font_mono_ = nullptr; }

    int ui_px   = std::max(8, (int)(18.0f * transform_scale_));
    int mono_px = std::max(8, (int)(16.0f * transform_scale_));

    font_ui_ = al_load_ttf_font("data/font.ttf", ui_px, 0);
    if (!font_ui_)
        font_ui_ = al_load_ttf_font(
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", ui_px, 0);
    if (!font_ui_) { font_ui_ = al_create_builtin_font(); font_ui_builtin_ = true; }
    else             font_ui_builtin_ = false;

    font_mono_ = al_load_ttf_font("data/mono.ttf", mono_px, 0);
    if (!font_mono_)
        font_mono_ = al_load_ttf_font(
            "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf", mono_px, 0);
    if (!font_mono_) { font_mono_ = al_create_builtin_font(); font_mono_builtin_ = true; }
    else               font_mono_builtin_ = false;
}

// Draw text in screen space (bypasses scale transform so font is sharp).
// gx/gy are game-space coordinates; converted to screen space here.
void Graphic_Manager::draw_text_s(ALLEGRO_FONT* font, ALLEGRO_COLOR col,
                                   float gx, float gy, int flags,
                                   const char* text) const {
    ALLEGRO_TRANSFORM identity;
    al_identity_transform(&identity);
    al_use_transform(&identity);
    float sx = transform_ox_ + gx * transform_scale_;
    float sy = transform_oy_ + gy * transform_scale_;
    al_draw_text(font, col, sx, sy, flags, text);
    // Restore scale transform
    ALLEGRO_TRANSFORM t;
    al_identity_transform(&t);
    al_scale_transform(&t, transform_scale_, transform_scale_);
    al_translate_transform(&t, transform_ox_, transform_oy_);
    al_use_transform(&t);
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
    font_ui_   = nullptr;
    font_mono_ = nullptr;
    update_transform(); // loads fonts at correct scale
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
    // Caps lock indicator — bottom-right dot, always drawn
    float cx = WIN_W - 14.0f, cy = WIN_H - 14.0f, r = 5.0f;
    ALLEGRO_COLOR col = caps_lock_
        ? al_map_rgb_f(1.0f, 0.85f, 0.1f)   // bright amber when on
        : al_map_rgb_f(0.25f, 0.25f, 0.28f); // dim when off
    al_draw_filled_circle(cx, cy, r, col);
    al_draw_circle(cx, cy, r, COL_DIM, 1.0f);
    if (caps_lock_)
        draw_text_s(font_ui_, al_map_rgb_f(1.0f,0.85f,0.1f),
                    WIN_W - 60.0f, WIN_H - 23.0f, 0, "CAPS");
}

bool Graphic_Manager::maximize_btn_click(int x, int y) const {
    // No custom maximize button — OS titlebar handles this now
    (void)x; (void)y;
    return false;
}

Graphic_Manager::~Graphic_Manager() {
    if (font_ui_   && !font_ui_builtin_)   al_destroy_font(font_ui_);
    if (font_mono_ && !font_mono_builtin_)  al_destroy_font(font_mono_);
    al_destroy_display(display_);
}

// ── Helpers ───────────────────────────────────────────────────────────────────
void Graphic_Manager::draw_button(float x, float y, float w, float h,
                                   const std::string& label, bool highlighted) {
    ALLEGRO_COLOR col = highlighted ? COL_BUTTON_H : COL_BUTTON;
    al_draw_filled_rounded_rectangle(x, y, x+w, y+h, 6, 6, col);
    al_draw_rounded_rectangle(x, y, x+w, y+h, 6, 6, COL_DIM, 1.0f);
    // Font height is in physical pixels; convert to game-space for centering
    float fh_game = (float)al_get_font_line_height(font_ui_) / transform_scale_;
    draw_text_s(font_ui_, COL_WHITE, x + w/2.0f, y + h/2.0f - fh_game/2.0f,
                ALLEGRO_ALIGN_CENTRE, label.c_str());
}

void Graphic_Manager::draw_toolbar(double elapsed_sec, double wpm,
                                    int time_remaining_sec) {
    al_draw_filled_rectangle(0, 0, WIN_W, TOOLBAR_H, COL_TOOLBAR);
    char buf[64];
    int mins = (int)elapsed_sec / 60;
    int secs = (int)elapsed_sec % 60;
    std::snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
    draw_text_s(font_ui_, COL_TEXT, 10, 12, 0, buf);

    std::snprintf(buf, sizeof(buf), "%.0f WPM", wpm);
    draw_text_s(font_ui_, COL_TEXT, WIN_W/2, 12, ALLEGRO_ALIGN_CENTRE, buf);

    if (time_remaining_sec >= 0) {
        std::snprintf(buf, sizeof(buf), "-%02d:%02d",
                      time_remaining_sec/60, time_remaining_sec%60);
        draw_text_s(font_ui_, COL_TEXT, WIN_W-10, 12, ALLEGRO_ALIGN_RIGHT, buf);
    }
}

// ── Menu ──────────────────────────────────────────────────────────────────────
void Graphic_Manager::render_menu(RoundMode current_mode, ErrorMode current_emode,
                                   int time_limit_sec, int word_target) {
    clear_full();

    draw_text_s(font_ui_, COL_WHITE, WIN_W/2, 70, ALLEGRO_ALIGN_CENTRE, "TYPESPEED");

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
        draw_text_s(font_ui_, COL_DIM, WIN_W/2, ey + 46, ALLEGRO_ALIGN_CENTRE, hint);

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
    if (fx >= (float)(WIN_W-160) && fx <= (float)(WIN_W-20) && fy >= (float)(WIN_H-60) && fy <= (float)(WIN_H-24)) return 20;
    return -1;
}

// ── Category ──────────────────────────────────────────────────────────────────
static const char* CAT_LABELS[] = {
    "English","Spanish","Python","C++","LaTeX","HTML","JavaScript"
};

void Graphic_Manager::render_category(Category current) {
    clear_full();
    draw_text_s(font_ui_, COL_WHITE, WIN_W/2, 60, ALLEGRO_ALIGN_CENTRE, "Select Category");
    for (int i = 0; i < 7; i++) {
        bool hi = ((int)current == i);
        draw_button(MENU_BTN_X,
                    120.0f + (float)i*(MENU_BTN_H + MENU_BTN_GAP),
                    MENU_BTN_W, MENU_BTN_H, CAT_LABELS[i], hi);
    }
    draw_button(20, WIN_H - 56, 100, 36, "Back");
    draw_window_chrome();
    al_flip_display();
}

int Graphic_Manager::category_click(int x, int y) const {
    float fx = (float)x, fy = (float)y;
    // Back button = -2
    if (fx >= 20 && fx <= 120 && fy >= WIN_H-56 && fy <= WIN_H-20) return -2;
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
    draw_text_s(font_ui_, COL_WHITE, WIN_W/2, 50, ALLEGRO_ALIGN_CENTRE, "Select File");

    if (files.empty()) {
        draw_text_s(font_ui_, COL_DIM, WIN_W/2, 200, ALLEGRO_ALIGN_CENTRE,
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
        draw_text_s(font_ui_, COL_WHITE, FILE_X+12, by+12, 0, fi.filename.c_str());
        char meta[64];
        std::snprintf(meta, sizeof(meta), "%d words  %d lines",
                      fi.word_count, fi.line_count);
        draw_text_s(font_ui_, COL_DIM, FILE_X+FILE_W-8, by+12,
                     ALLEGRO_ALIGN_RIGHT, meta);
    }

    draw_button(20, WIN_H-56, 100, 36, "Back");
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

bool Graphic_Manager::file_back_click(int x, int y) const {
    return (float)x >= 20 && (float)x <= 120 && (float)y >= WIN_H-56 && (float)y <= WIN_H-20;
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
    draw_text_s(font_ui_, COL_WHITE, WIN_W/2, 80, ALLEGRO_ALIGN_CENTRE,
                 fi.filename.c_str());
    char buf[128];
    if (TextBank::is_prose(cat) && total_para > 0)
        std::snprintf(buf, sizeof(buf), "Paragraph %d of %d",
                      current_para + 1, total_para);
    else
        std::strcpy(buf, "Full file");
    draw_text_s(font_ui_, COL_DIM, WIN_W/2, 120, ALLEGRO_ALIGN_CENTRE, buf);

    std::snprintf(buf, sizeof(buf), "%d words  |  %d lines",
                  fi.word_count, fi.line_count);
    draw_text_s(font_ui_, COL_TEXT, WIN_W/2, 160, ALLEGRO_ALIGN_CENTRE, buf);

    // Back / Start always present
    draw_button(WIN_W/2-160, 220, 140, 44, "Back");
    draw_button(WIN_W/2+20,  220, 140, 44, "Start!", true);

    // Prose-only navigation row
    if (TextBank::is_prose(cat) && total_para > 0) {
        float bw = 104.0f, bh = 36.0f, gap = 10.0f;
        float total_w = 5*bw + 4*gap;
        float bx = ((float)WIN_W - total_w) / 2.0f;
        float by = 290.0f;
        draw_button(bx,            by, bw, bh, "Redo Last");
        draw_button(bx+bw+gap,     by, bw, bh, "Skip");
        draw_button(bx+2*(bw+gap), by, bw, bh, "Restart");
        draw_button(bx+3*(bw+gap), by, bw, bh, "Jump To...");
        draw_button(bx+4*(bw+gap), by, bw, bh, "Next >>", true);
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

static constexpr float PREV_BTN_W   = 104.0f;
static constexpr float PREV_BTN_H   =  36.0f;
static constexpr float PREV_BTN_GAP =  10.0f;
static constexpr float PREV_BTN_Y   = 290.0f;

static float prev_nav_x(int idx) {
    float total_w = 5*PREV_BTN_W + 4*PREV_BTN_GAP;
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
bool Graphic_Manager::preview_next_click(int x, int y) const {
    return in_rect((float)x, (float)y, prev_nav_x(4), PREV_BTN_Y, PREV_BTN_W, PREV_BTN_H);
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
    draw_text_s(font_ui_, COL_TEXT, JUMP_X+JUMP_W/2.0f, JUMP_Y+18.0f,
                 ALLEGRO_ALIGN_CENTRE, buf);

    // Input box
    float ix = JUMP_X+20, iy = JUMP_Y+52, iw = JUMP_W-40, ih = 36;
    al_draw_filled_rounded_rectangle(ix, iy, ix+iw, iy+ih, 4, 4, {0.08f,0.08f,0.10f,1.0f});
    al_draw_rounded_rectangle(ix, iy, ix+iw, iy+ih, 4, 4, COL_DIM, 1.0f);
    std::string display = input.empty() ? std::to_string(current_para + 1) : input;
    draw_text_s(font_ui_, input.empty() ? COL_DIM : COL_WHITE,
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
// Compute the line number (0-based) that character index `target` falls on,
// given the layout parameters. Also returns total line count.
static void compute_line_of(const std::vector<CharState>& chars,
                              float x, float max_w, float fw,
                              int target, int& out_line, int& out_total_lines) {
    float cx = x;
    int line = 0;
    for (int i = 0; i < (int)chars.size(); i++) {
        if (i == target) out_line = line;
        int32_t cp = chars[(size_t)i].codepoint;
        if (cp == '\n' || cp == '\r') { cx = x; line++; continue; }
        if (cp == 9) {
            cx += fw * 4.0f;
            if (cx + fw > x + max_w) { cx = x; line++; }
            continue;
        }
        cx += fw;
        if (cx + fw > x + max_w) { cx = x; line++; }
    }
    if (target >= (int)chars.size()) out_line = line;
    out_total_lines = line + 1;
}

void Graphic_Manager::draw_passage(const Game& game, float x, float y,
                                    float max_w, float area_h, bool cursor_visible) {
    const auto& chars = game.char_states();
    int cursor = game.cursor_pos();
    float fh = (float)al_get_font_line_height(font_mono_) / transform_scale_;
    float fw = (float)al_get_text_width(font_mono_, "M")  / transform_scale_;
    float line_h = fh + 4.0f;

    // Find which line the cursor is on
    int cursor_line = 0, total_lines = 1;
    compute_line_of(chars, x, max_w, fw, cursor, cursor_line, total_lines);

    // Offset y so cursor line stays vertically centred in the available area
    float centre_y = y + area_h / 2.0f - fh / 2.0f;
    float start_y  = centre_y - (float)cursor_line * line_h;

    float cx = x, cy = start_y;
    for (int i = 0; i <= (int)chars.size(); i++) {
        if (i == cursor && cursor_visible)
            al_draw_filled_rectangle(cx, cy, cx + 2.0f, cy + fh, COL_CURSOR);

        if (i == (int)chars.size()) break;

        const auto& cs = chars[(size_t)i];
        ALLEGRO_COLOR col;
        switch (cs.status) {
            case CharState::Status::Correct: col = COL_CORRECT; break;
            case CharState::Status::Wrong:   col = COL_WRONG;   break;
            default:                         col = COL_TEXT;    break;
        }

        if (cs.codepoint == '\n' || cs.codepoint == '\r') {
            cx = x; cy += line_h; continue;
        }
        if (cs.codepoint == 9) {
            cx += fw * 4.0f;
            if (cx + fw > x + max_w) { cx = x; cy += line_h; }
            continue;
        }

        // Only draw chars that are visible in the area (clip above/below)
        if (cy + fh > y - line_h && cy < y + area_h + line_h)
            draw_text_s(font_mono_, col, cx, cy, 0, cs.utf8.c_str());

        cx += fw;
        if (cx + fw > x + max_w) { cx = x; cy += line_h; }
    }
}

void Graphic_Manager::render_playing(const Game& game, double elapsed_sec,
                                      int time_remaining_sec, double live_wpm,
                                      bool cursor_visible) {
    clear_full();
    draw_toolbar(elapsed_sec, live_wpm, time_remaining_sec);
    float passage_area_h = WIN_H - TOOLBAR_H - 30;
    draw_passage(game, 60, TOOLBAR_H + 30, WIN_W - 120, passage_area_h, cursor_visible);
    draw_window_chrome();
    al_flip_display();
}

// ── Results ───────────────────────────────────────────────────────────────────
void Graphic_Manager::render_results(const SessionResult& r) {
    clear_full();
    draw_text_s(font_ui_, COL_WHITE, WIN_W/2, 30, ALLEGRO_ALIGN_CENTRE, "Results");

    char buf[128];
    std::snprintf(buf, sizeof(buf), "%.1f WPM", r.wpm);
    draw_text_s(font_ui_, COL_CORRECT, WIN_W/2, 80, ALLEGRO_ALIGN_CENTRE, buf);

    std::snprintf(buf, sizeof(buf), "Accuracy: %.1f%%", r.accuracy * 100.0);
    draw_text_s(font_ui_, COL_TEXT, WIN_W/2, 110, ALLEGRO_ALIGN_CENTRE, buf);

    std::snprintf(buf, sizeof(buf), "Errors: %d", r.error_count);
    draw_text_s(font_ui_, COL_WRONG, WIN_W/2, 140, ALLEGRO_ALIGN_CENTRE, buf);

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
        draw_text_s(font_ui_, COL_DIM, gx - 4, gy - 2, ALLEGRO_ALIGN_RIGHT, buf);
        std::snprintf(buf, sizeof(buf), "%.0fs", max_t);
        draw_text_s(font_ui_, COL_DIM, gx+gw, gy+gh+2, ALLEGRO_ALIGN_RIGHT, buf);
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

// ── Keyboard heatmap ─────────────────────────────────────────────────────────
void Graphic_Manager::draw_keyboard_heatmap(
        float ox, float oy,
        const std::unordered_map<int32_t,KeyStat>& ks) {

    struct Key { const char* label; int32_t cp; };
    static const Key ROW0[] = {
        {"`",96},{"1",49},{"2",50},{"3",51},{"4",52},{"5",53},
        {"6",54},{"7",55},{"8",56},{"9",57},{"0",48},{"-",45},{"=",61}
    };
    static const Key ROW1[] = {
        {"q",113},{"w",119},{"e",101},{"r",114},{"t",116},{"y",121},
        {"u",117},{"i",105},{"o",111},{"p",112},{"[",91},{"]",93},{"\\",92}
    };
    static const Key ROW2[] = {
        {"a",97},{"s",115},{"d",100},{"f",102},{"g",103},{"h",104},
        {"j",106},{"k",107},{"l",108},{";",59},{"'",39}
    };
    static const Key ROW3[] = {
        {"z",122},{"x",120},{"c",99},{"v",118},{"b",98},{"n",110},
        {"m",109},{",",44},{".",46},{"/",47}
    };

    int max_count = 1;
    for (auto& [cp, k] : ks) if (k.count > max_count) max_count = k.count;

    // Key dimensions — sized to fit 13 keys + stagger across ~840px
    static constexpr float U   = 30.0f; // key cell size (square)
    static constexpr float GAP =  3.0f; // gap between keys
    static constexpr float R   =  4.0f; // corner radius

    float fh = (float)al_get_font_line_height(font_ui_) / transform_scale_;

    // Heat colour: near-black → deep indigo → teal → amber
    // Uses a perceptually smoother ramp than the old blue→cyan→yellow→red
    auto heat_color = [](float t) -> ALLEGRO_COLOR {
        t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        float r, g, b;
        if (t < 0.25f) {
            // black → deep indigo
            float u = t / 0.25f;
            r = u * 0.20f; g = 0.0f; b = u * 0.45f;
        } else if (t < 0.55f) {
            // deep indigo → teal
            float u = (t - 0.25f) / 0.30f;
            r = 0.20f - u * 0.10f; g = u * 0.65f; b = 0.45f + u * 0.25f;
        } else if (t < 0.80f) {
            // teal → amber
            float u = (t - 0.55f) / 0.25f;
            r = 0.10f + u * 0.85f; g = 0.65f + u * 0.20f; b = 0.70f - u * 0.70f;
        } else {
            // amber → warm red
            float u = (t - 0.80f) / 0.20f;
            r = 0.95f; g = 0.85f - u * 0.65f; b = 0.0f;
        }
        return al_map_rgb_f(r, g, b);
    };

    auto draw_key = [&](float kx, float ky, float kw, int32_t cp,
                        const char* label) {
        auto it = ks.find(cp);
        float t = (it != ks.end()) ? (float)it->second.count / (float)max_count : 0.0f;

        float x1 = kx, y1 = ky, x2 = kx + kw - GAP, y2 = ky + U - GAP;
        float cx = (x1 + x2) * 0.5f;

        // Shadow strip (1 px offset, slightly darker)
        al_draw_filled_rounded_rectangle(x1+1, y1+1, x2+1, y2+1, R, R,
                                         al_map_rgba_f(0,0,0,0.45f));

        // Key face
        ALLEGRO_COLOR fill = heat_color(t);
        al_draw_filled_rounded_rectangle(x1, y1, x2, y2, R, R, fill);

        // Subtle top-edge highlight to give a slight 3-D lift
        ALLEGRO_COLOR hi = al_map_rgba_f(1,1,1, t < 0.1f ? 0.12f : 0.06f);
        al_draw_filled_rounded_rectangle(x1, y1, x2, y1 + (U-GAP)*0.35f, R, R, hi);

        // Border — brighter on hot keys
        ALLEGRO_COLOR border = t > 0.5f
            ? al_map_rgba_f(1,1,1,0.25f)
            : al_map_rgba_f(1,1,1,0.10f);
        al_draw_rounded_rectangle(x1, y1, x2, y2, R, R, border, 1.0f);

        // Key label (centred vertically when no avg line, shifted up when avg shown)
        ALLEGRO_COLOR txt_col = t > 0.55f
            ? al_map_rgb_f(0.05f, 0.05f, 0.05f)  // dark text on bright keys
            : COL_WHITE;

        bool has_avg = (it != ks.end() && it->second.count > 0);
        float label_y = has_avg
            ? y1 + 2.0f
            : y1 + (U - GAP - fh) * 0.5f;
        draw_text_s(font_ui_, txt_col, cx, label_y, ALLEGRO_ALIGN_CENTRE, label);

        // Avg ms in a smaller dim line beneath the label
        if (has_avg) {
            char avg_buf[12];
            std::snprintf(avg_buf, sizeof(avg_buf), "%.0f",
                          it->second.total_ms / it->second.count);
            ALLEGRO_COLOR dim = t > 0.55f
                ? al_map_rgba_f(0,0,0,0.6f)
                : al_map_rgba_f(1,1,1,0.45f);
            draw_text_s(font_ui_, dim, cx, label_y + fh, ALLEGRO_ALIGN_CENTRE, avg_buf);
        }
    };

    // Standard QWERTY stagger in pixels (each unit = U+GAP)
    float step = U + GAP;
    float stagger[] = { 0.0f, step*0.5f, step*0.75f, step*1.25f };

    auto draw_row = [&](const Key* keys, int n, int row) {
        float kx = ox + stagger[row];
        float ky = oy + (float)row * step;
        for (int i = 0; i < n; i++) {
            draw_key(kx, ky, step, keys[i].cp, keys[i].label);
            kx += step;
        }
    };

    draw_row(ROW0, 13, 0);
    draw_row(ROW1, 13, 1);
    draw_row(ROW2, 11, 2);
    draw_row(ROW3, 10, 3);

    // Space bar — 5 units wide, centred under the B/N area
    float space_x = ox + stagger[3] + 2.5f * step;
    float space_w = 5.0f * step;
    draw_key(space_x, oy + 4 * step, space_w, 32, "space");

    // ── Vertical colour bar on the right ─────────────────────────────────────
    // Maps colour → actual ms value so the reader can decode any key's speed.
    // t=0 (cold/fast) at top, t=1 (hot/slow) at bottom.

    // Find max avg ms across all keys to label the axis
    double max_avg_ms = 0.0;
    for (auto& [cp, k] : ks)
        if (k.count > 0) {
            double avg = k.total_ms / k.count;
            if (avg > max_avg_ms) max_avg_ms = avg;
        }
    if (max_avg_ms < 1.0) max_avg_ms = 500.0; // fallback when no data

    float bar_x  = ox + 13 * step + 10.0f; // just right of the keyboard
    float bar_y  = oy;
    float bar_h  = 5 * step;               // full keyboard height
    float bar_w  = 10.0f;
    int   segs   = 120;
    float seg_h  = bar_h / segs;

    // Draw gradient segments top→bottom (t goes 0→1)
    for (int i = 0; i < segs; i++) {
        float t = (float)i / (segs - 1);
        al_draw_filled_rectangle(bar_x, bar_y + i * seg_h,
                                 bar_x + bar_w, bar_y + (i+1) * seg_h,
                                 heat_color(t));
    }
    // Thin border around the bar
    al_draw_rectangle(bar_x, bar_y, bar_x + bar_w, bar_y + bar_h,
                      al_map_rgba_f(1,1,1,0.15f), 1.0f);

    // Tick marks + ms labels at 0%, 25%, 50%, 75%, 100%
    float tick_x  = bar_x + bar_w + 3.0f;
    float label_x = bar_x + bar_w + 6.0f;
    for (int i = 0; i <= 4; i++) {
        float frac = (float)i / 4.0f;
        float ty2  = bar_y + frac * bar_h;
        // tick
        al_draw_line(bar_x + bar_w, ty2, tick_x, ty2,
                     al_map_rgba_f(1,1,1,0.35f), 1.0f);
        // label — ms value at this fraction
        char ms_buf[16];
        double ms_val = frac * max_avg_ms;
        if (ms_val < 10.0)
            std::snprintf(ms_buf, sizeof(ms_buf), "%.0f ms", ms_val);
        else
            std::snprintf(ms_buf, sizeof(ms_buf), "%.0f ms", ms_val);
        // vertical align: top label hangs below tick, bottom hangs above
        float label_y2 = ty2 - fh * 0.5f;
        draw_text_s(font_ui_, COL_DIM, label_x, label_y2, 0, ms_buf);
    }

    // "fast" / "slow" captions outside the ticks
    draw_text_s(font_ui_, al_map_rgba_f(0.5f,0.8f,0.9f,0.8f),
                label_x + 28.0f, bar_y - fh - 1.0f, 0, "fast");
    draw_text_s(font_ui_, al_map_rgba_f(0.95f,0.5f,0.1f,0.8f),
                label_x + 28.0f, bar_y + bar_h + 1.0f, 0, "slow");
}

// ── WPM bar chart ─────────────────────────────────────────────────────────────
void Graphic_Manager::draw_wpm_chart(float ox, float oy, float w, float h,
                                      const std::vector<SessionResult>& history) {
    if (history.empty()) {
        draw_text_s(font_ui_, COL_DIM, ox + w/2, oy + h/2,
                    ALLEGRO_ALIGN_CENTRE, "No sessions yet");
        return;
    }

    // Take last 20 sessions
    int n = (int)std::min((int)history.size(), 20);
    const int start = (int)history.size() - n;

    double max_wpm = 10.0;
    for (int i = start; i < (int)history.size(); i++)
        if (history[(size_t)i].wpm > max_wpm) max_wpm = history[(size_t)i].wpm;

    float bar_w = (w - 2.0f) / (float)n;
    float fh    = (float)al_get_font_line_height(font_ui_) / transform_scale_;

    for (int i = 0; i < n; i++) {
        const auto& r = history[(size_t)(start + i)];
        float t   = (float)(r.wpm / max_wpm);
        float bh  = (h - fh - 4.0f) * t;
        float bx  = ox + (float)i * bar_w + 1.0f;
        float by  = oy + h - fh - 4.0f - bh;
        // colour by accuracy
        float acc = (float)r.accuracy;
        ALLEGRO_COLOR col = al_map_rgb_f(1.0f - acc, acc * 0.8f, 0.3f);
        al_draw_filled_rectangle(bx, by, bx + bar_w - 2.0f, oy + h - fh - 4.0f, col);

        // WPM label on last bar
        if (i == n - 1) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%.0f", r.wpm);
            draw_text_s(font_ui_, COL_WHITE, bx + bar_w/2 - 1.0f, by - fh - 2.0f,
                        ALLEGRO_ALIGN_CENTRE, buf);
        }
    }

    // axis labels
    char top[16]; std::snprintf(top, sizeof(top), "%.0f wpm", max_wpm);
    draw_text_s(font_ui_, COL_DIM, ox,        oy,          0,                    top);
    draw_text_s(font_ui_, COL_DIM, ox,        oy + h - fh, 0,                    "0");
    draw_text_s(font_ui_, COL_DIM, ox + w/2,  oy + h,      ALLEGRO_ALIGN_CENTRE, "last 20 sessions");
}

void Graphic_Manager::render_global_stats() {
    clear_full();

    auto history = Stats::load_history();
    auto keylog  = Stats::load_keylog();

    draw_text_s(font_ui_, COL_WHITE, WIN_W/2, 18, ALLEGRO_ALIGN_CENTRE, "Global Stats");

    // ── Lifetime totals ───────────────────────────────────────────────────────
    int    sessions   = (int)history.size();
    double total_min  = 0.0;
    double sum_wpm    = 0.0;
    double sum_acc    = 0.0;
    double best_wpm   = 0.0;
    for (auto& r : history) {
        total_min += r.elapsed_ms / 60000.0;
        sum_wpm   += r.wpm;
        sum_acc   += r.accuracy;
        if (r.wpm > best_wpm) best_wpm = r.wpm;
    }
    double avg_wpm = sessions > 0 ? sum_wpm / sessions : 0.0;
    double avg_acc = sessions > 0 ? sum_acc / sessions : 0.0;

    char buf[128];
    float ty = 44.0f;
    float fh = (float)al_get_font_line_height(font_ui_) / transform_scale_;

    std::snprintf(buf, sizeof(buf), "Sessions: %d    Time: %.0f min    Best: %.0f wpm    Avg: %.0f wpm    Accuracy: %.1f%%",
                  sessions, total_min, best_wpm, avg_wpm, avg_acc * 100.0);
    draw_text_s(font_ui_, COL_DIM, WIN_W/2, ty, ALLEGRO_ALIGN_CENTRE, buf);

    // ── WPM chart ─────────────────────────────────────────────────────────────
    float chart_y = ty + fh + 8.0f;
    float chart_h = 90.0f;
    draw_wpm_chart(40.0f, chart_y, WIN_W - 80.0f, chart_h, history);

    // ── Keyboard heatmap ──────────────────────────────────────────────────────
    // 13 keys × (30+3)px = 429px wide; centre in WIN_W=900
    static constexpr float HEAT_W = 13.0f * 33.0f;
    float heat_ox = (WIN_W - HEAT_W) * 0.5f;
    float heat_y  = chart_y + chart_h + 10.0f;
    draw_text_s(font_ui_, COL_DIM, WIN_W/2, heat_y, ALLEGRO_ALIGN_CENTRE,
                "key frequency  |  number = avg ms between presses");
    draw_keyboard_heatmap(heat_ox, heat_y + fh + 4.0f, keylog);

    draw_button(WIN_W/2-70, WIN_H-52, 140, 36, "Back");
    draw_window_chrome();
    al_flip_display();
}

bool Graphic_Manager::global_stats_back_click(int x, int y) const {
    return x >= WIN_W/2-70 && x <= WIN_W/2+70 && y >= WIN_H-52 && y <= WIN_H-16;
}
