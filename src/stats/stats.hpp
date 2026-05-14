#pragma once
#include <string>
#include <vector>
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

class Stats {
public:
    void record_correct();
    void record_error();
    void sample_wpm(double elapsed_sec);

    SessionResult finish(const std::string& category,
                         const std::string& filename,
                         int elapsed_ms);
    void reset();

    static void append_history(const SessionResult& r,
                               const std::string& filename = "data/history.txt");
    static std::vector<SessionResult> load_history(
        const std::string& filename = "data/history.txt");

    int correct_chars_proxy() const { return correct_chars_; }
    int error_count_proxy()   const { return error_count_; }

private:
    int correct_chars_ = 0;
    int total_chars_   = 0;
    int error_count_   = 0;
    std::vector<WpmSample> samples_;
};
