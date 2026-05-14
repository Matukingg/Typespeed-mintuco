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
    bool preview_confirm_click(int x, int y) const;
    bool preview_back_click(int x, int y)    const;

    // ── Playing ───────────────────────────────────────────────────────────────
    void render_playing(const Game& game, double elapsed_sec,
                        int time_remaining_sec,
                        double live_wpm,
                        bool cursor_visible);

    // ── Results ───────────────────────────────────────────────────────────────
    void render_results(const SessionResult& r);
    bool results_again_click(int x, int y) const;
    bool results_menu_click(int x, int y)  const;

private:
    ALLEGRO_DISPLAY* display_;
    ALLEGRO_FONT*    font_ui_;
    ALLEGRO_FONT*    font_mono_;

    void draw_toolbar(double elapsed_sec, double wpm, int time_remaining_sec);
    void draw_button(float x, float y, float w, float h,
                     const std::string& label, bool highlighted = false);
    void draw_passage(const Game& game, float x, float y,
                      float max_w, bool cursor_visible);
};
