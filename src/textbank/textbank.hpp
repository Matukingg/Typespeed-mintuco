#pragma once
#include <string>
#include <vector>
#include <unordered_map>

enum class Category { English, Spanish, Python, Cpp, Latex, Html, Javascript };

struct FileInfo {
    std::string filename;        // bare name, e.g. "1984.txt"
    std::string full_path;       // e.g. "data/english/1984.txt"
    int         word_count;
    int         line_count;
    int         paragraph_count; // prose only; 0 for code
};

class TextBank {
public:
    // Scan data/<category>/ and populate file list
    void scan(Category cat);

    // All files found after scan()
    const std::vector<FileInfo>& files() const { return files_; }

    // Category folder name, e.g. "english"
    static std::string category_folder(Category cat);

    // True if category is prose (English/Spanish)
    static bool is_prose(Category cat);

    // Load a passage:
    //   Prose  — loads paragraph at current progress index, advances index, saves progress.txt
    //   Code   — loads the full file
    // Returns the passage text. Empty string on failure.
    std::string load_passage(const FileInfo& fi, Category cat);

    // For the preview screen
    int current_paragraph(const FileInfo& fi) const;  // prose only; 0 for code
    int total_paragraphs (const FileInfo& fi) const;  // prose only; 0 for code

    // Load/save progress.txt
    void load_progress(const std::string& filename = "data/progress.txt");
    void save_progress(const std::string& filename = "data/progress.txt") const;

private:
    std::vector<FileInfo> files_;

    // key: full_path  value: current paragraph index (-1 = completed)
    std::unordered_map<std::string, int> progress_;

    // key: full_path  value: all paragraphs split from the file
    std::unordered_map<std::string, std::vector<std::string>> para_cache_;

    std::vector<std::string> split_paragraphs(const std::string& text) const;
    FileInfo compute_info(const std::string& full_path, Category cat) const;
};
