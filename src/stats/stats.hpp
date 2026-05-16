#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>

struct WpmSample {
    double elapsed_sec;
    double wpm;
};

struct SessionResult {
    std::string category;
    std::string filename;
    double      wpm;
    double      accuracy;     // 0.0–1.0
    int         error_count;
    int         elapsed_ms;
    std::time_t timestamp;
    std::vector<WpmSample> wpm_samples;
};

// Per-key aggregate stats loaded from keylog.txt
struct KeyStat {
    int32_t codepoint;
    int     count;
    double  total_ms;   // sum of inter-key intervals when this key was hit
};

class Stats {
public:
    void record_correct();
    void record_error();
    void record_key(int32_t codepoint, double interval_ms);
    void sample_wpm(double elapsed_sec);

    SessionResult finish(const std::string& category,
                         const std::string& filename,
                         int elapsed_ms);
    void reset();

    static void append_history(const SessionResult& r,
                               const std::string& filename = "data/history.txt");
    static std::vector<SessionResult> load_history(
        const std::string& filename = "data/history.txt");

    static void append_keylog(const std::unordered_map<int32_t,KeyStat>& keys,
                              const std::string& filename = "data/keylog.txt");
    static std::unordered_map<int32_t,KeyStat> load_keylog(
        const std::string& filename = "data/keylog.txt");

    const std::unordered_map<int32_t,KeyStat>& key_stats() const { return key_stats_; }

    int correct_chars_proxy() const { return correct_chars_; }
    int error_count_proxy()   const { return error_count_; }

private:
    int correct_chars_ = 0;
    int total_chars_   = 0;
    int error_count_   = 0;
    std::vector<WpmSample> samples_;
    std::unordered_map<int32_t,KeyStat> key_stats_;
    double last_key_time_ms_ = -1.0;
};
