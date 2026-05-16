#include <algorithm>
#include <random>
#include "core.hpp"
#include "input.hpp"
#include "textbank.hpp"
#include "game.hpp"
#include "stats.hpp"
#include "graphicMgr.hpp"

enum class Phase {
    MENU, CATEGORY, FILE_PICK, PREVIEW, JUMP, PLAYING, RESULTS, GLOBAL_STATS
};

int main() {
    Core& core = Core::instance();
    core.load_settings();

    Graphic_Manager gfx;
    Input input(gfx.get_display());

    TextBank bank;
    bank.load_progress();

    Game          game;
    SessionResult last_result{};

    RoundMode sel_mode  = (RoundMode)core.get_int("mode_idx", 0);
    ErrorMode sel_emode = core.get_bool("strict", true)
                              ? ErrorMode::Strict : ErrorMode::Lenient;
    Category  sel_cat   = (Category)core.get_int("cat_idx", 0);
    int       file_scroll = 0;

    int time_limit_sec = core.get_int("time_limit_sec", 60);
    int word_target    = core.get_int("word_target",    50);

    Phase phase = Phase::MENU;

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 20.0);
    al_register_event_source(input.get_queue(), al_get_timer_event_source(timer));
    al_start_timer(timer);

    bool   running      = true;
    bool   cursor_vis   = true;
    int    tick_count   = 0;
    int    sample_ticks = 0;
    double start_time   = 0.0;
    double elapsed_sec  = 0.0;
    double live_wpm     = 0.0;

    FileInfo    current_file{};
    std::string jump_input;

    // Re-render the current phase — called after window mode changes
    auto force_redraw = [&]() {
        switch (phase) {
            case Phase::MENU:
                gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target); break;
            case Phase::CATEGORY:
                gfx.render_category(sel_cat); break;
            case Phase::FILE_PICK:
                gfx.render_file_pick(bank.files(), file_scroll); break;
            case Phase::PREVIEW: {
                int cp = bank.current_paragraph(current_file);
                int tp = bank.total_paragraphs(current_file);
                gfx.render_preview(current_file, sel_cat, cp, tp); break;
            }
            case Phase::JUMP: {
                int cp = bank.current_paragraph(current_file);
                int tp = bank.total_paragraphs(current_file);
                gfx.render_jump_overlay(cp, tp, jump_input); break;
            }
            case Phase::PLAYING:
                gfx.render_playing(game, elapsed_sec,
                    sel_mode == RoundMode::TimeLimit
                        ? std::max(0, (int)(time_limit_sec - elapsed_sec)) : -1,
                    live_wpm, cursor_vis); break;
            case Phase::RESULTS:
                gfx.render_results(last_result); break;
            case Phase::GLOBAL_STATS:
                gfx.render_global_stats(); break;
        }
    };

    gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(input.get_queue(), &ev);

        // ── Timer ─────────────────────────────────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_TIMER) {
            tick_count++;
            if (tick_count % 10 == 0) cursor_vis = !cursor_vis;

            if (phase == Phase::PLAYING) {
                if (start_time > 0.0)
                    elapsed_sec = al_get_time() - start_time;
                live_wpm = game.live_wpm(elapsed_sec);

                if (++sample_ticks >= 100) {
                    sample_ticks = 0;
                    game.tick_sample(elapsed_sec);
                }

                if (sel_mode == RoundMode::TimeLimit &&
                    elapsed_sec >= time_limit_sec) {
                    al_stop_timer(timer);
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

        // ── Display close / resize ───────────────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) { running = false; break; }

        if (ev.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
            al_acknowledge_resize(gfx.get_display());
            gfx.update_transform();
            force_redraw();
        }

        // ── Mouse ─────────────────────────────────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP && ev.mouse.button == 1) {
            int mx, my;
            gfx.screen_to_game(ev.mouse.x, ev.mouse.y, mx, my);

            if (phase == Phase::MENU) {
                int btn = gfx.menu_click(mx, my);
                if (btn >= 0 && btn <= 3) {
                    sel_mode = (RoundMode)btn;
                    core.set_int("mode_idx", btn);
                    core.save_settings();
                    gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);
                } else if (btn == 10) {
                    sel_emode = ErrorMode::Strict;
                    core.set_bool("strict", true); core.save_settings();
                    gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);
                } else if (btn == 11) {
                    sel_emode = ErrorMode::Lenient;
                    core.set_bool("strict", false); core.save_settings();
                    gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);
                } else if (btn == 30) {
                    // Start button → go to category
                    phase = Phase::CATEGORY;
                    gfx.render_category(sel_cat);
                } else if (btn == 20) {
                    phase = Phase::GLOBAL_STATS;
                    gfx.render_global_stats();
                }

            } else if (phase == Phase::CATEGORY) {
                int c = gfx.category_click(mx, my);
                if (c == -2) {
                    phase = Phase::MENU;
                    gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);
                } else if (c >= 0) {
                    sel_cat = (Category)c;
                    core.set_int("cat_idx", c); core.save_settings();
                    bank.scan(sel_cat);
                    file_scroll = 0;
                    phase = Phase::FILE_PICK;
                    gfx.render_file_pick(bank.files(), file_scroll);
                }

            } else if (phase == Phase::FILE_PICK) {
                if (gfx.file_back_click(mx, my)) {
                    phase = Phase::CATEGORY;
                    gfx.render_category(sel_cat);
                } else if (gfx.file_scroll_up_click(mx, my)) {
                    if (file_scroll > 0) file_scroll--;
                    gfx.render_file_pick(bank.files(), file_scroll);
                } else if (gfx.file_scroll_down_click(mx, my)) {
                    if (file_scroll + 10 < (int)bank.files().size())
                        file_scroll++;
                    gfx.render_file_pick(bank.files(), file_scroll);
                } else {
                    int fi = gfx.file_click(mx, my, file_scroll);
                    if (fi >= 0 && fi < (int)bank.files().size()) {
                        current_file = bank.files()[(size_t)fi];
                        int cp = bank.current_paragraph(current_file);
                        int tp = bank.total_paragraphs(current_file);
                        phase = Phase::PREVIEW;
                        gfx.render_preview(current_file, sel_cat, cp, tp);
                    }
                }

            } else if (phase == Phase::PREVIEW) {
                if (gfx.preview_confirm_click(mx, my)) {
                    std::string passage = bank.load_passage(current_file, sel_cat);
                    if (passage.empty()) {
                        phase = Phase::FILE_PICK;
                        gfx.render_file_pick(bank.files(), file_scroll);
                    } else {
                        game.start(passage, sel_mode, sel_emode,
                                   time_limit_sec, word_target);
                        start_time   = 0.0; // set on first keypress
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

            } else if (phase == Phase::GLOBAL_STATS) {
                if (gfx.global_stats_back_click(mx, my)) {
                    phase = Phase::MENU;
                    gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);
                }

            } else if (phase == Phase::RESULTS) {
                if (gfx.results_again_click(mx, my)) {
                    int cp = bank.current_paragraph(current_file);
                    int tp = bank.total_paragraphs(current_file);
                    phase = Phase::PREVIEW;
                    gfx.render_preview(current_file, sel_cat, cp, tp);
                } else if (gfx.results_menu_click(mx, my)) {
                    phase = Phase::MENU;
                    gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);
                }
            }
        }

        // ── F11 fullscreen toggle (all phases) ────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN &&
            ev.keyboard.keycode == ALLEGRO_KEY_F11) {
            gfx.toggle_fullscreen();
            core.set_bool("fullscreen", gfx.is_fullscreen());
            core.save_settings();
            force_redraw();
        }

        // ── Keyboard ──────────────────────────────────────────────────────────
        if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {

            // Menu: < / > adjust time limit / word target
            if (phase == Phase::MENU) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_COMMA ||
                    ev.keyboard.keycode == ALLEGRO_KEY_LEFT) {
                    if (sel_mode == RoundMode::TimeLimit)
                        time_limit_sec = std::max(10, time_limit_sec - 10);
                    else if (sel_mode == RoundMode::WordCount)
                        word_target = std::max(10, word_target - 10);
                    core.set_int("time_limit_sec", time_limit_sec);
                    core.set_int("word_target",    word_target);
                    core.save_settings();
                    gfx.render_menu(sel_mode, sel_emode, time_limit_sec, word_target);
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_FULLSTOP ||
                           ev.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
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

            if (phase == Phase::PLAYING) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_LEFT) {
                    game.on_left();
                    int remaining = (sel_mode == RoundMode::TimeLimit)
                        ? std::max(0,(int)(time_limit_sec - elapsed_sec)) : -1;
                    gfx.render_playing(game, elapsed_sec, remaining, live_wpm, cursor_vis);
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
                    game.on_right();
                    int remaining = (sel_mode == RoundMode::TimeLimit)
                        ? std::max(0,(int)(time_limit_sec - elapsed_sec)) : -1;
                    gfx.render_playing(game, elapsed_sec, remaining, live_wpm, cursor_vis);
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    al_stop_timer(timer);
                    last_result = game.finish(
                        TextBank::category_folder(sel_cat),
                        current_file.filename,
                        (int)(elapsed_sec * 1000));
                    Stats::append_history(last_result);
                    phase = Phase::RESULTS;
                    gfx.render_results(last_result);

                } else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE) {
                    game.on_backspace();
                    int remaining = (sel_mode == RoundMode::TimeLimit)
                        ? std::max(0, (int)(time_limit_sec - elapsed_sec)) : -1;
                    gfx.render_playing(game, elapsed_sec, remaining, live_wpm, cursor_vis);

                } else if (ev.keyboard.unichar == 9 || ev.keyboard.unichar >= 32) {
                    if (start_time <= 0.0)
                        start_time = al_get_time(); // start clock on first keypress
                    game.on_key(ev.keyboard.unichar);
                    live_wpm = game.live_wpm(elapsed_sec);

                    if (game.is_finished()) {
                        if (sel_mode == RoundMode::Endless) {
                            // Load next passage
                            std::string next = bank.load_passage(current_file, sel_cat);
                            if (next.empty()) {
                                // Exhausted — pick another file from category
                                bank.scan(sel_cat);
                                if (!bank.files().empty()) {
                                    std::mt19937 rng(std::random_device{}());
                                    std::uniform_int_distribution<int> dist(
                                        0, (int)bank.files().size() - 1);
                                    current_file = bank.files()[(size_t)dist(rng)];
                                    next = bank.load_passage(current_file, sel_cat);
                                }
                            }
                            if (!next.empty())
                                game.start(next, sel_mode, sel_emode,
                                           time_limit_sec, word_target);
                        } else {
                            al_stop_timer(timer);
                            last_result = game.finish(
                                TextBank::category_folder(sel_cat),
                                current_file.filename,
                                (int)(elapsed_sec * 1000));
                            Stats::append_history(last_result);
                            phase = Phase::RESULTS;
                            gfx.render_results(last_result);
                        }
                    } else {
                        int remaining = (sel_mode == RoundMode::TimeLimit)
                            ? std::max(0, (int)(time_limit_sec - elapsed_sec)) : -1;
                        gfx.render_playing(game, elapsed_sec, remaining,
                                           live_wpm, cursor_vis);
                    }
                }
            }
        }
    }

    al_destroy_timer(timer);
    core.save_settings();
    return 0;
}
