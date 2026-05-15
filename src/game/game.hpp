#pragma once
#include <string>
#include <vector>
#include "stats.hpp"

enum class ErrorMode { Strict, Lenient };
enum class RoundMode { Paragraph, TimeLimit, WordCount, Endless };

// Status of each display slot in the passage
struct CharState {
    char ch;     // expected character (or typed extra char if extra_)
    bool extra_; // true = this slot is an extra typed char beyond the passage
    enum class Status { Neutral, Correct, Wrong } status = Status::Neutral;
    CharState(char c, bool extra = false)
        : ch(c), extra_(extra), status(Status::Neutral) {}
};

class Game {
public:
    void start(const std::string& passage, RoundMode mode, ErrorMode emode,
               int time_limit_sec = 60, int word_target = 50);

    bool on_key(int unichar);
    bool on_backspace();

    void tick_sample(double elapsed_sec);

    bool is_finished() const;
    bool is_strict()   const { return emode_ == ErrorMode::Strict; }
    bool has_errors()  const; // true if any wrong/extra chars exist before cursor

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
    std::vector<CharState> chars_;   // passage chars + any extra typed chars
    int         passage_len_ = 0;    // length of original passage (no extras)
    int         cursor_      = 0;    // current display position
    RoundMode   mode_        = RoundMode::Paragraph;
    ErrorMode   emode_       = ErrorMode::Strict;
    int         time_limit_sec_ = 60;
    int         word_target_    = 50;
    Stats       stats_;

    void advance_past_newlines();
};
