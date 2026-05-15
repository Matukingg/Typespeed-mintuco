#include "game.hpp"
#include <algorithm>

void Game::advance_past_newlines() {
    while (cursor_ < (int)chars_.size()
           && !chars_[(size_t)cursor_].extra_
           && chars_[(size_t)cursor_].ch == '\n') {
        chars_[(size_t)cursor_].status = CharState::Status::Correct;
        cursor_++;
    }
}

void Game::start(const std::string& passage, RoundMode mode, ErrorMode emode,
                 int time_limit_sec, int word_target) {
    chars_.clear();
    for (char c : passage)
        chars_.emplace_back(c);
    passage_len_    = (int)chars_.size();
    cursor_         = 0;
    mode_           = mode;
    emode_          = emode;
    time_limit_sec_ = time_limit_sec;
    word_target_    = word_target;
    stats_.reset();
    advance_past_newlines();
}

// Returns true if the character is a whitespace the user can type
static bool is_typeable_ws(int unichar) {
    return unichar == 32 || unichar == 9; // space or tab
}

// Returns true if the passage character is whitespace
static bool is_passage_ws(char c) {
    return c == ' ' || c == '\t';
}

bool Game::on_key(int unichar) {
    // Accept printable chars + space (32) + tab (9).
    // Reject everything else (control chars, enter, escape handled by caller).
    if (unichar < 9) return false;
    if (unichar > 9 && unichar < 32) return false;

    // Skip past any newlines at current position
    advance_past_newlines();

    // Are we past the end of the passage?
    bool at_end = (cursor_ >= passage_len_);

    if (!at_end) {
        char expected = chars_[(size_t)cursor_].ch;

        // Whitespace flexibility: space and tab are interchangeable
        bool correct;
        if (is_typeable_ws(unichar) && is_passage_ws(expected))
            correct = true;
        else
            correct = ((char)unichar == expected);

        if (correct) {
            chars_[(size_t)cursor_].status = CharState::Status::Correct;
            stats_.record_correct();
            cursor_++;
            advance_past_newlines();
        } else {
            // Wrong character — insert as an extra red char at cursor position
            char typed = is_typeable_ws(unichar) ? ' ' : (char)unichar;
            chars_.insert(chars_.begin() + cursor_,
                          CharState(typed, true));
            chars_[(size_t)cursor_].status = CharState::Status::Wrong;
            stats_.record_error();
            cursor_++;
        }
    } else {
        // Typed past end — add extra char
        char typed = is_typeable_ws(unichar) ? ' ' : (char)unichar;
        chars_.emplace_back(typed, true);
        chars_.back().status = CharState::Status::Wrong;
        stats_.record_error();
        cursor_++;
    }
    return true;
}

bool Game::on_backspace() {
    if (cursor_ <= 0) return false;

    cursor_--;
    auto& cs = chars_[(size_t)cursor_];

    if (cs.extra_) {
        // Remove extra inserted character
        chars_.erase(chars_.begin() + cursor_);
    } else {
        // Reset passage character to neutral
        cs.status = CharState::Status::Neutral;
    }
    return true;
}

bool Game::has_errors() const {
    for (int i = 0; i < cursor_ && i < (int)chars_.size(); i++) {
        if (chars_[(size_t)i].status == CharState::Status::Wrong)
            return true;
        if (chars_[(size_t)i].extra_)
            return true;
    }
    return false;
}

void Game::tick_sample(double elapsed_sec) {
    stats_.sample_wpm(elapsed_sec);
}

bool Game::is_finished() const {
    // Can't finish if there are errors — must fix them first
    if (has_errors()) return false;

    if (mode_ == RoundMode::WordCount)
        return words_typed() >= word_target_;

    // Paragraph / Endless / TimeLimit: all passage chars must be correct
    // Count only non-extra chars that are correct
    int correct_passage = 0;
    for (auto& cs : chars_)
        if (!cs.extra_ && cs.status == CharState::Status::Correct)
            correct_passage++;
    return correct_passage >= passage_len_;
}

int Game::words_typed() const {
    if (cursor_ == 0) return 0;
    int spaces = 0;
    int limit = std::min(cursor_, (int)chars_.size());
    for (int i = 0; i < limit; i++)
        if (!chars_[(size_t)i].extra_ && chars_[(size_t)i].ch == ' ')
            spaces++;
    return spaces + 1;
}

double Game::live_wpm(double elapsed_sec) const {
    double mins = elapsed_sec / 60.0;
    if (mins <= 0.0) return 0.0;
    return stats_.correct_chars_proxy() / 5.0 / mins;
}

SessionResult Game::finish(const std::string& category,
                            const std::string& filename,
                            int elapsed_ms) {
    return stats_.finish(category, filename, elapsed_ms);
}

void Game::reset() {
    chars_.clear();
    passage_len_ = 0;
    cursor_      = 0;
    stats_.reset();
}
