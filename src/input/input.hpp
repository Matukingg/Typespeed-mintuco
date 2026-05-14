#pragma once
#include <allegro5/allegro5.h>

class Input {
    ALLEGRO_EVENT_QUEUE* queue;
public:
    explicit Input(ALLEGRO_DISPLAY* display);
    ~Input();
    ALLEGRO_EVENT_QUEUE* get_queue() const { return queue; }
};
