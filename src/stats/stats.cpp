#include "stats.hpp"
#include <fstream>
#include <sstream>

void Stats::record_correct() { correct_chars_++; total_chars_++; }
void Stats::record_error()   { error_count_++;   total_chars_++; }

void Stats::record_key(int32_t codepoint, double now_ms) {
    if (now_ms < 0.0) return; // no timestamp — skip entirely, don't corrupt chain
    if (last_key_time_ms_ >= 0.0) {
        double interval = now_ms - last_key_time_ms_;
        if (interval > 0.0 && interval < 3000.0) {
            auto& ks = key_stats_[codepoint];
            ks.codepoint = codepoint;
            ks.count++;
            ks.total_ms += interval;
        }
        // Always advance the timestamp so the next interval is measured from
        // now, not from a stale point before a long pause.
    }
    last_key_time_ms_ = now_ms;
}

void Stats::sample_wpm(double elapsed_sec) {
    if (elapsed_sec <= 0.0) return;
    double minutes = elapsed_sec / 60.0;
    double wpm = (correct_chars_ / 5.0) / minutes;
    samples_.push_back({elapsed_sec, wpm});
}

SessionResult Stats::finish(const std::string& category,
                             const std::string& filename,
                             int elapsed_ms) {
    SessionResult r;
    r.category    = category;
    r.filename    = filename;
    r.elapsed_ms  = elapsed_ms;
    r.error_count = error_count_;
    r.timestamp   = std::time(nullptr);
    r.wpm_samples = samples_;

    double minutes = elapsed_ms / 60000.0;
    r.wpm      = (minutes > 0.0) ? (correct_chars_ / 5.0) / minutes : 0.0;
    r.accuracy = (total_chars_ > 0) ? (double)correct_chars_ / total_chars_ : 1.0;
    return r;
}

void Stats::reset() {
    correct_chars_    = 0;
    total_chars_      = 0;
    error_count_      = 0;
    last_key_time_ms_ = -1.0;
    samples_.clear();
    key_stats_.clear();
}

void Stats::append_history(const SessionResult& r, const std::string& filename) {
    std::ofstream f(filename, std::ios::app);
    if (!f.is_open()) return;
    f << r.timestamp << "," << r.category << "," << r.filename << ","
      << r.wpm << "," << r.accuracy << "," << r.error_count << ","
      << r.elapsed_ms;
    for (auto& s : r.wpm_samples)
        f << "," << s.elapsed_sec << ":" << s.wpm;
    f << "\n";
}

void Stats::append_keylog(const std::unordered_map<int32_t,KeyStat>& keys,
                          const std::string& filename) {
    // Load existing, merge, rewrite
    auto existing = load_keylog(filename);
    for (auto& [cp, ks] : keys) {
        auto& e = existing[cp];
        e.codepoint = cp;
        e.count    += ks.count;
        e.total_ms += ks.total_ms;
    }
    std::ofstream f(filename, std::ios::trunc);
    if (!f.is_open()) return;
    for (auto& [cp, ks] : existing)
        f << cp << "," << ks.count << "," << ks.total_ms << "\n";
}

std::unordered_map<int32_t,KeyStat> Stats::load_keylog(const std::string& filename) {
    std::unordered_map<int32_t,KeyStat> out;
    std::ifstream f(filename);
    if (!f.is_open()) return out;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string tok;
        KeyStat ks{};
        std::getline(ss, tok, ','); try { ks.codepoint = (int32_t)std::stoll(tok); } catch (...) { continue; }
        std::getline(ss, tok, ','); try { ks.count     = std::stoi(tok); }           catch (...) { continue; }
        std::getline(ss, tok, ','); try { ks.total_ms  = std::stod(tok); }           catch (...) { continue; }
        out[ks.codepoint] = ks;
    }
    return out;
}

std::vector<SessionResult> Stats::load_history(const std::string& filename) {
    std::vector<SessionResult> results;
    std::ifstream f(filename);
    if (!f.is_open()) return results;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string tok;
        SessionResult r;
        std::getline(ss, tok, ','); try { r.timestamp   = std::stoll(tok); } catch (...) { continue; }
        std::getline(ss, r.category,  ',');
        std::getline(ss, r.filename,  ',');
        std::getline(ss, tok, ','); try { r.wpm         = std::stod(tok);  } catch (...) {}
        std::getline(ss, tok, ','); try { r.accuracy    = std::stod(tok);  } catch (...) {}
        std::getline(ss, tok, ','); try { r.error_count = std::stoi(tok);  } catch (...) {}
        std::getline(ss, tok, ','); try { r.elapsed_ms  = std::stoi(tok);  } catch (...) {}
        while (std::getline(ss, tok, ',')) {
            auto colon = tok.find(':');
            if (colon == std::string::npos) continue;
            WpmSample s;
            try {
                s.elapsed_sec = std::stod(tok.substr(0, colon));
                s.wpm         = std::stod(tok.substr(colon+1));
            } catch (...) { continue; }
            r.wpm_samples.push_back(s);
        }
        results.push_back(r);
    }
    return results;
}
