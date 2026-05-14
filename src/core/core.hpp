#pragma once
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <iostream>
#include <unordered_map>
#include <string>

class Core {
    std::unordered_map<std::string, std::string> settings = {
        {"mode",           "paragraph"},
        {"error_handling", "strict"},
        {"category",       "english"},
    };
public:
    Core();
    ~Core();
    static Core& instance();

    bool load_settings(const std::string& filename = "settings.txt");
    bool save_settings(const std::string& filename = "settings.txt") const;

    std::string get_string(const std::string& key, const std::string& def = "") const;
    int         get_int   (const std::string& key, int def = 0)                  const;
    bool        get_bool  (const std::string& key, bool def = false)             const;

    void set_string(const std::string& key, const std::string& value);
    void set_int   (const std::string& key, int value);
    void set_bool  (const std::string& key, bool value);
};
