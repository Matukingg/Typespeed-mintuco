#include "core.hpp"
#include <fstream>
#include <sstream>

Core::Core() {
    if (!al_init())                  { std::cerr << "al_init failed\n";       std::exit(1); }
    if (!al_init_primitives_addon()) { std::cerr << "primitives failed\n";    std::exit(1); }
    if (!al_init_image_addon())      { std::cerr << "image addon failed\n";   std::exit(1); }
    if (!al_init_font_addon())       { std::cerr << "font addon failed\n";    std::exit(1); }
    if (!al_init_ttf_addon())        { std::cerr << "ttf addon failed\n";     std::exit(1); }
    if (!al_install_keyboard())      { std::cerr << "keyboard failed\n";      std::exit(1); }
    if (!al_install_mouse())         { std::cerr << "mouse failed\n";         std::exit(1); }
}

Core::~Core() {
    al_shutdown_ttf_addon();
    al_shutdown_font_addon();
    al_shutdown_image_addon();
    al_shutdown_primitives_addon();
    al_uninstall_keyboard();
    al_uninstall_mouse();
    al_uninstall_system();
}

Core& Core::instance() {
    static Core c;
    return c;
}

bool Core::load_settings(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        settings[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return true;
}

bool Core::save_settings(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) return false;
    for (const auto& p : settings)
        out << p.first << "=" << p.second << "\n";
    return true;
}

std::string Core::get_string(const std::string& key, const std::string& def) const {
    auto it = settings.find(key);
    return (it != settings.end()) ? it->second : def;
}

int Core::get_int(const std::string& key, int def) const {
    auto s = get_string(key);
    if (s.empty()) return def;
    try { return std::stoi(s); } catch (...) { return def; }
}

bool Core::get_bool(const std::string& key, bool def) const {
    auto s = get_string(key);
    if (s == "1" || s == "true")  return true;
    if (s == "0" || s == "false") return false;
    return def;
}

void Core::set_string(const std::string& key, const std::string& value) { settings[key] = value; }
void Core::set_int   (const std::string& key, int value)   { settings[key] = std::to_string(value); }
void Core::set_bool  (const std::string& key, bool value)  { settings[key] = value ? "1" : "0"; }
