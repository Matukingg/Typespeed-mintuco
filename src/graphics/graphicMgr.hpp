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
    static constexpr int WIN_W     = 900;
    static constexpr int WIN_H     = 600;
    static constexpr int TOOLBAR_H =  44;

    explicit Graphic_Manager();
    ~Graphic_Manager();

    ALLEGRO_DISPLAY* get_display() const { return display_; }

    // ── Menu ──────────────────────────────────────────────────────────────────
    // btn codes: 0-3=mode, 10=strict, 11=lenient, 20=global stats
    void render_menu(RoundMode current_mode, ErrorMode current_emode,
                     int time_limit_sec = 60, int word_target = 50);
    int  menu_click(int x, int y) const;

    // ── Category ──────────────────────────────────────────────────────────────
    void render_category(Category current);
    int  category_click(int x, int y) const;

    // ── File pick ─────────────────────────────────────────────────────────────
    void render_file_pick(const std::vector<FileInfo>& files, int scroll_offset);
    int  file_click(int x, int y, int scroll_offset) const;
    bool file_scroll_up_click(int x, int y)   const;
    bool file_scroll_down_click(int x, int y) const;

    // ── Preview ───────────────────────────────────────────────────────────────
    void render_preview(const FileInfo& fi, Category cat,
                        int current_para, int total_para);
    bool preview_confirm_click (int x, int y) const;
    bool preview_back_click    (int x, int y) const;
    bool preview_redo_click    (int x, int y) const;
    bool preview_skip_click    (int x, int y) const;
    bool preview_restart_click (int x, int y) const;
    bool preview_jump_click    (int x, int y) const;

    // ── Jump overlay ──────────────────────────────────────────────────────────
    void render_jump_overlay(int current_para, int total_para,
                             const std::string& input);
    bool jump_confirm_click(int x, int y) const;
    bool jump_cancel_click (int x, int y) const;

    // ── Playing ───────────────────────────────────────────────────────────────
    void render_playing(const Game& game, double elapsed_sec,
                        int time_remaining_sec,
                        double live_wpm,
                        bool cursor_visible);

    // ── Results ───────────────────────────────────────────────────────────────
    void render_results(const SessionResult& r);
    bool results_again_click(int x, int y) const;
    bool results_menu_click(int x, int y)  const;

    // ── Global Stats ──────────────────────────────────────────────────────────
    void render_global_stats();
    bool global_stats_back_click(int x, int y) const;

    // ── Fullscreen / window chrome ────────────────────────────────────────────
    void update_transform(); // recompute centered scale transform — call after resize/mode change
    void toggle_fullscreen();
    void toggle_maximized();
    bool is_fullscreen() const { return fullscreen_; }

    // Returns true if the click hit the maximize button (top-right chrome)
    bool maximize_btn_click(int x, int y) const;

    // Must be called after every render to draw the window chrome overlay
    void draw_window_chrome();

    // Convert raw mouse coordinates to game-space coordinates
    void screen_to_game(int sx, int sy, int& gx, int& gy) const {
        gx = (int)(((float)sx - transform_ox_) / transform_scale_);
        gy = (int)(((float)sy - transform_oy_) / transform_scale_);
    }

private:
    ALLEGRO_DISPLAY* display_;
    ALLEGRO_FONT*    font_ui_;
    ALLEGRO_FONT*    font_mono_;
    bool             fullscreen_       = false;
    float            transform_ox_     = 0.0f;
    float            transform_oy_     = 0.0f;
    float            transform_scale_  = 1.0f;

    void clear_full(); // clear entire display to COL_BG, preserve game transform
    void draw_toolbar(double elapsed_sec, double wpm, int time_remaining_sec);
    void draw_button(float x, float y, float w, float h,
                     const std::string& label, bool highlighted = false);
    void draw_passage(const Game& game, float x, float y,
                      float max_w, bool cursor_visible);
};
