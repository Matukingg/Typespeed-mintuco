#include "game.hpp"

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
    if (unichar < 32) return false;

    char expected = chars_[cursor_].ch;
    bool correct  = ((char)unichar == expected);

    if (!correct && emode_ == ErrorMode::Strict) {
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
    if (mode_ == RoundMode::WordCount)
        return words_typed() >= word_target_;
    // Paragraph, Endless, TimeLimit (time checked by caller): finish on passage end
    return cursor_ >= (int)chars_.size();
}

int Game::words_typed() const {
    int spaces = 0;
    for (int i = 0; i < cursor_ && i < (int)chars_.size(); i++)
        if (chars_[i].ch == ' ' && chars_[i].status == CharState::Status::Correct)
            spaces++;
    return spaces;
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
    cursor_    = 0;
    has_error_ = false;
    stats_.reset();
}
