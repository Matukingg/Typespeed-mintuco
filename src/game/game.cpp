#include "game.hpp"
#include <algorithm>

// ── UTF-8 helpers ─────────────────────────────────────────────────────────────

int32_t utf8_decode(const std::string& str, size_t& pos) {
    if (pos >= str.size()) return -1;
    unsigned char c = (unsigned char)str[pos];
    int32_t cp;
    size_t bytes;
    if      (c < 0x80) { cp = c;         bytes = 1; }
    else if (c < 0xE0) { cp = c & 0x1F;  bytes = 2; }
    else if (c < 0xF0) { cp = c & 0x0F;  bytes = 3; }
    else               { cp = c & 0x07;  bytes = 4; }
    for (size_t i = 1; i < bytes && pos + i < str.size(); i++)
        cp = (cp << 6) | ((unsigned char)str[pos + i] & 0x3F);
    pos += bytes;
    return cp;
}

std::string utf8_encode(int32_t cp) {
    std::string s;
    if (cp < 0x80) {
        s += (char)cp;
    } else if (cp < 0x800) {
        s += (char)(0xC0 | (cp >> 6));
        s += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        s += (char)(0xE0 | (cp >> 12));
        s += (char)(0x80 | ((cp >> 6) & 0x3F));
        s += (char)(0x80 | (cp & 0x3F));
    } else {
        s += (char)(0xF0 | (cp >> 18));
        s += (char)(0x80 | ((cp >> 12) & 0x3F));
        s += (char)(0x80 | ((cp >> 6)  & 0x3F));
        s += (char)(0x80 | (cp & 0x3F));
    }
    return s;
}

static bool is_ws(int32_t cp)      { return cp == 32 || cp == 9; }
static bool is_newline(int32_t cp) { return cp == 10 || cp == 13; }

static const int32_t BRACKET_PAIRS[][2] = {{'(',')'}, {'[',']'}, {'{','}'}};
static int32_t matching_closer(int32_t opener) {
    for (auto& p : BRACKET_PAIRS)
        if (p[0] == opener) return p[1];
    return 0;
}

// ── Game ──────────────────────────────────────────────────────────────────────

void Game::advance_past_newlines() {
    while (cursor_ < (int)chars_.size()
           && !chars_[(size_t)cursor_].extra_
           && is_newline(chars_[(size_t)cursor_].codepoint)) {
        chars_[(size_t)cursor_].status = CharState::Status::Correct;
        cursor_++;
    }
}

void Game::start(const std::string& passage, RoundMode mode, ErrorMode emode,
                 int time_limit_sec, int word_target) {
    chars_.clear();
    size_t pos = 0;
    while (pos < passage.size()) {
        size_t start = pos;
        int32_t cp = utf8_decode(passage, pos);
        chars_.emplace_back(passage.substr(start, pos - start), cp);
    }
    passage_len_    = (int)chars_.size();
    cursor_         = 0;
    mode_           = mode;
    emode_          = emode;
    time_limit_sec_ = time_limit_sec;
    word_target_    = word_target;
    stats_.reset();
    advance_past_newlines();
}

// Mark a passage char at index as correct (without advancing cursor)
void Game::mark_correct_at(int idx) {
    chars_[(size_t)idx].status = CharState::Status::Correct;
    stats_.record_correct();
}

bool Game::on_key(int32_t unichar, double now_ms) {
    // Accept: tab (9), space (32), printable (33+). Reject other control chars.
    if (unichar < 9)                    return false;
    if (unichar > 9 && unichar < 32)    return false;

    advance_past_newlines();

    // ── Past end of passage ───────────────────────────────────────────────────
    if (cursor_ >= passage_len_) {
        chars_.emplace_back(utf8_encode(unichar), unichar, true);
        chars_.back().status = CharState::Status::Wrong;
        stats_.record_error();
        cursor_++;
        return true;
    }

    // ── Cursor is on a passage character ─────────────────────────────────────
    int32_t expected = chars_[(size_t)cursor_].codepoint;

    // Rule 1: space typed — always skip ALL consecutive whitespace in the passage,
    // landing cursor on the first non-whitespace character (word-jump behaviour).
    if (is_ws(unichar)) {
        if (is_ws(expected)) {
            // Mark all consecutive whitespace chars as correct and jump past them
            while (cursor_ < passage_len_
                   && !chars_[(size_t)cursor_].extra_
                   && is_ws(chars_[(size_t)cursor_].codepoint)) {
                chars_[(size_t)cursor_].status = CharState::Status::Correct;
                stats_.record_correct();
                cursor_++;
            }
            advance_past_newlines();
        } else {
            // Space where a non-space is expected — error
            chars_.insert(chars_.begin() + cursor_,
                          CharState(utf8_encode(unichar), unichar, true));
            chars_[(size_t)cursor_].status = CharState::Status::Wrong;
            stats_.record_error();
            cursor_++;
        }
        return true;
    }

    // Rule 2: non-whitespace char — exact match, or skip leading whitespace if present
    bool correct = (unichar == expected);

    // Rule 3: space-skip — non-whitespace typed, but passage has whitespace here.
    // Look past all whitespace to see if the typed char matches beyond.
    // Lets you type "n+1" when the passage says "n + 1".
    if (!correct && is_ws(expected)) {
        int look = cursor_;
        while (look < passage_len_ && is_ws(chars_[(size_t)look].codepoint)
               && !chars_[(size_t)look].extra_)
            look++;
        if (look < passage_len_ && unichar == chars_[(size_t)look].codepoint
            && !chars_[(size_t)look].extra_) {
            while (cursor_ < look) {
                chars_[(size_t)cursor_].status = CharState::Status::Correct;
                stats_.record_correct();
                cursor_++;
            }
            correct = true;
        }
    }

    if (correct) {
        chars_[(size_t)cursor_].status = CharState::Status::Correct;
        stats_.record_correct();
        stats_.record_key(unichar, now_ms);
        cursor_++;
        advance_past_newlines();

        // Rule 3: bracket auto-complete — after typing ( [ {, find the
        // matching closer in the passage and pre-mark it correct.
        // Cursor stays before the closer so the user types the contents first.
        int32_t closer = matching_closer(unichar);
        if (closer != 0) {
            // Walk forward through passage (non-extra) chars to find the matching closer
            int depth = 1;
            for (int look = cursor_; look < passage_len_; look++) {
                if (chars_[(size_t)look].extra_) continue;
                int32_t lcp = chars_[(size_t)look].codepoint;
                if (lcp == unichar) depth++;
                else if (lcp == closer) {
                    depth--;
                    if (depth == 0) {
                        // Mark the closer correct and leave cursor before it
                        chars_[(size_t)look].status = CharState::Status::Correct;
                        stats_.record_correct();
                        break;
                    }
                }
            }
        }
    } else {
        // Wrong — insert a red extra character at cursor position
        chars_.insert(chars_.begin() + cursor_,
                      CharState(utf8_encode(unichar), unichar, true));
        chars_[(size_t)cursor_].status = CharState::Status::Wrong;
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
        chars_.erase(chars_.begin() + cursor_);
    } else {
        cs.status = CharState::Status::Neutral;
        // If this was a bracket closer that was auto-completed, un-mark it
    }
    return true;
}

bool Game::on_left() {
    if (cursor_ <= 0) return false;
    cursor_--;
    return true;
}

bool Game::on_right() {
    if (cursor_ >= (int)chars_.size()) return false;
    cursor_++;
    return true;
}

bool Game::has_errors() const {
    for (int i = 0; i < (int)chars_.size(); i++) {
        const auto& cs = chars_[(size_t)i];
        if (cs.extra_ || cs.status == CharState::Status::Wrong) return true;
    }
    return false;
}

void Game::tick_sample(double elapsed_sec) { stats_.sample_wpm(elapsed_sec); }

bool Game::is_finished() const {
    if (has_errors()) return false;
    if (mode_ == RoundMode::WordCount) return words_typed() >= word_target_;
    // All passage chars must be correct
    for (auto& cs : chars_)
        if (!cs.extra_ && cs.status != CharState::Status::Correct) return false;
    return true;
}

int Game::words_typed() const {
    if (cursor_ == 0) return 0;
    int spaces = 0;
    int limit = std::min(cursor_, (int)chars_.size());
    for (int i = 0; i < limit; i++)
        if (!chars_[(size_t)i].extra_ && chars_[(size_t)i].codepoint == 32)
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
