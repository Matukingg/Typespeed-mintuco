#include "input.hpp"

Input::Input(ALLEGRO_DISPLAY* display) {
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());
}

Input::~Input() {
    al_destroy_event_queue(queue);
}
