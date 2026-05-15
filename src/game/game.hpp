#pragma once
#include <string>
#include <vector>
#include "stats.hpp"

enum class ErrorMode { Strict, Lenient };
enum class RoundMode { Paragraph, TimeLimit, WordCount, Endless };

struct CharState {
    std::string utf8; // the character as a UTF-8 string (1-4 bytes)
    int32_t     codepoint; // Unicode codepoint for comparison
    bool        extra_;    // true = extra typed char beyond passage
    enum class Status { Neutral, Correct, Wrong } status = Status::Neutral;

    CharState(const std::string& u, int32_t cp, bool extra = false)
        : utf8(u), codepoint(cp), extra_(extra), status(Status::Neutral) {}
};

// Decode first UTF-8 codepoint from str starting at pos.
// Returns codepoint and advances pos past it.
int32_t utf8_decode(const std::string& str, size_t& pos);

// Encode a codepoint to UTF-8 string
std::string utf8_encode(int32_t cp);

class Game {
public:
    void start(const std::string& passage, RoundMode mode, ErrorMode emode,
               int time_limit_sec = 60, int word_target = 50);

    // unichar = Unicode codepoint from Allegro KEY_CHAR event
    bool on_key(int32_t unichar);
    bool on_backspace();
    bool on_left();   // move cursor left (for bracket workflow)
    bool on_right();  // move cursor right

    void tick_sample(double elapsed_sec);

    bool is_finished() const;
    bool is_strict()   const { return emode_ == ErrorMode::Strict; }
    bool has_errors()  const;

    const std::vector<CharState>& char_states() const { return chars_; }
    int cursor_pos()    const { return cursor_; }
    int error_count()   const { return stats_.error_count_proxy(); }
    int words_typed()   const;
    RoundMode round_mode() const { return mode_; }

    double live_wpm(double elapsed_sec) const;

    SessionResult finish(const std::string& category,
                         const std::string& filename,
                         int elapsed_ms);
    void reset();

private:
    std::vector<CharState> chars_;
    int         passage_len_ = 0;
    int         cursor_      = 0;
    RoundMode   mode_        = RoundMode::Paragraph;
    ErrorMode   emode_       = ErrorMode::Strict;
    int         time_limit_sec_ = 60;
    int         word_target_    = 50;
    Stats       stats_;

    void advance_past_newlines();
};
