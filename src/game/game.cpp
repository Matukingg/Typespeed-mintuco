#include "game.hpp"
#include <algorithm>

// ── UTF-8 helpers ─────────────────────────────────────────────────────────────

int32_t utf8_decode(const std::string& str, size_t& pos) {
    if (pos >= str.size()) return -1;
    unsigned char c = (unsigned char)str[pos];
    int32_t cp;
    size_t bytes;
    if (c < 0x80)        { cp = c;          bytes = 1; }
    else if (c < 0xE0)   { cp = c & 0x1F;  bytes = 2; }
    else if (c < 0xF0)   { cp = c & 0x0F;  bytes = 3; }
    else                  { cp = c & 0x07;  bytes = 4; }
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

static bool is_ws_cp(int32_t cp) { return cp == 32 || cp == 9; } // space or tab
static bool is_newline_cp(int32_t cp) { return cp == 10 || cp == 13; }

// ── Game implementation ───────────────────────────────────────────────────────

void Game::advance_past_newlines() {
    while (cursor_ < (int)chars_.size()
           && !chars_[(size_t)cursor_].extra_
           && is_newline_cp(chars_[(size_t)cursor_].codepoint)) {
        chars_[(size_t)cursor_].status = CharState::Status::Correct;
        cursor_++;
    }
}

void Game::start(const std::string& passage, RoundMode mode, ErrorMode emode,
                 int time_limit_sec, int word_target) {
    chars_.clear();
    // Decode passage as UTF-8 codepoints
    size_t pos = 0;
    while (pos < passage.size()) {
        size_t start = pos;
        int32_t cp = utf8_decode(passage, pos);
        std::string seq = passage.substr(start, pos - start);
        chars_.emplace_back(seq, cp);
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

bool Game::on_key(int32_t unichar) {
    // Accept printable + space (32) + tab (9). Reject other control chars.
    if (unichar < 9) return false;
    if (unichar > 9 && unichar < 32) return false;

    advance_past_newlines();

    bool at_end = (cursor_ >= passage_len_);

    if (!at_end) {
        int32_t expected_cp = chars_[(size_t)cursor_].codepoint;

        // Whitespace flexibility: space/tab match any whitespace in passage
        bool correct;
        if (is_ws_cp(unichar) && is_ws_cp(expected_cp))
            correct = true;
        // Space flexibility: skip spaces in passage when not typed (handled below)
        else
            correct = (unichar == expected_cp);

        if (correct) {
            chars_[(size_t)cursor_].status = CharState::Status::Correct;
            stats_.record_correct();
            cursor_++;
            advance_past_newlines();

            // Bracket auto-pair: after typing ( [ {, look ahead for the
            // matching closer in the passage and jump cursor past it,
            // leaving it pre-marked as correct so the user can move left
            // and type the contents. Mirrors how editors handle bracket pairs.
            static const int32_t PAIRS[][2] = {{'(',')'}, {'[',']'}, {'{','}'}};
            for (auto& p : PAIRS) {
                if (unichar != p[0]) continue;
                // Find the matching closer in the passage (skip extras)
                int depth = 1, look = cursor_;
                while (look < passage_len_ && depth > 0) {
                    int32_t lcp = chars_[(size_t)look].codepoint;
                    if (!chars_[(size_t)look].extra_) {
                        if (lcp == p[0]) depth++;
                        if (lcp == p[1]) depth--;
                    }
                    if (depth > 0) look++;
                }
                // Only auto-pair if the closer is the very next passage char
                // OR is immediately after whitespace (simple heuristic)
                if (depth == 0 && look == cursor_) {
                    // Closer is exactly at cursor — mark it correct and
                    // leave cursor before it so user types contents
                    chars_[(size_t)look].status = CharState::Status::Correct;
                    stats_.record_correct();
                    // Don't advance cursor — user types contents before closer
                }
                break;
            }
        } else {
            // Check if we should auto-skip spaces in passage (n+1 == n + 1)
            // If typed char matches the NEXT non-space passage char, skip spaces
            int lookahead = cursor_;
            while (lookahead < passage_len_ &&
                   is_ws_cp(chars_[(size_t)lookahead].codepoint))
                lookahead++;
            bool matches_after_spaces = (lookahead < passage_len_ &&
                                         unichar == chars_[(size_t)lookahead].codepoint &&
                                         lookahead > cursor_); // only if spaces were skipped

            if (matches_after_spaces) {
                // Mark skipped spaces as correct silently
                while (cursor_ < lookahead) {
                    chars_[(size_t)cursor_].status = CharState::Status::Correct;
                    cursor_++;
                }
                // Now type the matching char
                chars_[(size_t)cursor_].status = CharState::Status::Correct;
                stats_.record_correct();
                cursor_++;
                advance_past_newlines();
            } else {
                // Wrong — insert extra red char
                std::string typed_utf8 = utf8_encode(unichar);
                chars_.insert(chars_.begin() + cursor_,
                              CharState(typed_utf8, unichar, true));
                chars_[(size_t)cursor_].status = CharState::Status::Wrong;
                stats_.record_error();
                cursor_++;
            }
        }
    } else {
        // Past end — add extra char
        std::string typed_utf8 = utf8_encode(unichar);
        chars_.emplace_back(typed_utf8, unichar, true);
        chars_.back().status = CharState::Status::Wrong;
        stats_.record_error();
        cursor_++;
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

bool Game::on_backspace() {
    if (cursor_ <= 0) return false;
    cursor_--;
    auto& cs = chars_[(size_t)cursor_];
    if (cs.extra_) {
        chars_.erase(chars_.begin() + cursor_);
    } else {
        cs.status = CharState::Status::Neutral;
    }
    return true;
}

bool Game::has_errors() const {
    for (int i = 0; i < cursor_ && i < (int)chars_.size(); i++) {
        if (chars_[(size_t)i].status == CharState::Status::Wrong) return true;
        if (chars_[(size_t)i].extra_) return true;
    }
    return false;
}

void Game::tick_sample(double elapsed_sec) {
    stats_.sample_wpm(elapsed_sec);
}

bool Game::is_finished() const {
    if (has_errors()) return false;
    if (mode_ == RoundMode::WordCount)
        return words_typed() >= word_target_;
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
