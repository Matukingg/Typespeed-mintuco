#include "textbank.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <random>

namespace fs = std::filesystem;

std::string TextBank::category_folder(Category cat) {
    switch (cat) {
        case Category::English:    return "english";
        case Category::Spanish:    return "spanish";
        case Category::Python:     return "python";
        case Category::Cpp:        return "cpp";
        case Category::Latex:      return "latex";
        case Category::Html:       return "html";
        case Category::Javascript: return "javascript";
    }
    return "english";
}

bool TextBank::is_prose(Category cat) {
    return cat == Category::English || cat == Category::Spanish;
}

std::vector<std::string> TextBank::split_paragraphs(const std::string& text) const {
    std::vector<std::string> result;
    std::istringstream ss(text);
    std::string line, block;
    while (std::getline(ss, line)) {
        if (line.empty()) {
            if (!block.empty()) { result.push_back(block); block.clear(); }
        } else {
            if (!block.empty()) block += '\n';
            block += line;
        }
    }
    if (!block.empty()) result.push_back(block);
    return result;
}

FileInfo TextBank::compute_info(const std::string& full_path, Category cat) const {
    FileInfo fi;
    fi.full_path       = full_path;
    fi.filename        = fs::path(full_path).filename().string();
    fi.word_count      = 0;
    fi.line_count      = 0;
    fi.paragraph_count = 0;

    std::ifstream f(full_path);
    if (!f.is_open()) return fi;

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    std::istringstream ss(content);
    std::string line;
    bool in_para = false;
    while (std::getline(ss, line)) {
        fi.line_count++;
        std::istringstream ws(line);
        std::string word;
        while (ws >> word) fi.word_count++;
        if (is_prose(cat)) {
            if (!line.empty()) in_para = true;
            else if (in_para) { fi.paragraph_count++; in_para = false; }
        }
    }
    if (is_prose(cat) && in_para) fi.paragraph_count++;

    return fi;
}

void TextBank::scan(Category cat) {
    files_.clear();
    std::string folder = "data/" + category_folder(cat);
    if (!fs::exists(folder)) return;
    for (auto& entry : fs::directory_iterator(folder)) {
        if (entry.path().extension() == ".txt")
            files_.push_back(compute_info(entry.path().string(), cat));
    }
    std::sort(files_.begin(), files_.end(),
              [](const FileInfo& a, const FileInfo& b){ return a.filename < b.filename; });
}

int TextBank::current_paragraph(const FileInfo& fi) const {
    auto it = progress_.find(fi.full_path);
    if (it == progress_.end()) return 0;
    return (it->second < 0) ? 0 : it->second;
}

int TextBank::total_paragraphs(const FileInfo& fi) const {
    auto it = para_cache_.find(fi.full_path);
    if (it == para_cache_.end()) return fi.paragraph_count;
    return (int)it->second.size();
}

std::string TextBank::load_passage(const FileInfo& fi, Category cat) {
    std::ifstream f(fi.full_path);
    if (!f.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    if (!is_prose(cat)) return content;

    if (para_cache_.find(fi.full_path) == para_cache_.end())
        para_cache_[fi.full_path] = split_paragraphs(content);

    auto& paras = para_cache_[fi.full_path];
    if (paras.empty()) return content;

    int& idx = progress_[fi.full_path];
    if (idx < 0) {
        // completed — pick random
        std::mt19937 rng(std::random_device{}());
        idx = std::uniform_int_distribution<int>(0, (int)paras.size()-1)(rng);
    }
    std::string passage = paras[(size_t)idx];
    idx++;
    if (idx >= (int)paras.size()) idx = -1; // mark completed
    save_progress();
    return passage;
}

void TextBank::load_progress(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        int val = 0;
        try { val = std::stoi(line.substr(eq+1)); } catch (...) {}
        // key format: "english/1984.txt" → full_path "data/english/1984.txt"
        progress_["data/" + key] = val;
    }
}

void TextBank::save_progress(const std::string& filename) const {
    std::ofstream f(filename);
    if (!f.is_open()) return;
    for (auto& p : progress_) {
        // strip leading "data/" from key
        std::string key = p.first;
        if (key.substr(0, 5) == "data/") key = key.substr(5);
        f << key << "=" << p.second << "\n";
    }
}
