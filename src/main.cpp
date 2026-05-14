#include "core.hpp"
#include "input.hpp"

int main() {
    Core& core = Core::instance();
    core.load_settings();

    ALLEGRO_DISPLAY* display = al_create_display(800, 500);
    if (!display) { return -1; }

    Input input(display);

    bool running = true;
    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(input.get_queue(), &ev);
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) running = false;
    }

    al_destroy_display(display);
    return 0;
}
