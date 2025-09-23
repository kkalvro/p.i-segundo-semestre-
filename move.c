#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h> 

int main(){
if (!al_init()) {
    printf("Erro ao inicializar Allegro!\n");
    return -1;
}


al_install_keyboard();
al_init_primitives_addon();


ALLEGRO_DISPLAY* display = al_create_display(640, 480);
if (!display) {
    printf("Erro ao criar janela!\n");
    return -1;
}


ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();
al_register_event_source(fila, al_get_keyboard_event_source());


float x = 320, y = 240;
float velocidade = 5;

int rodando = 1;
while (rodando) {
    ALLEGRO_EVENT ev;
    if (al_get_next_event(fila, &ev)) {
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch (ev.keyboard.keycode) {
            case ALLEGRO_KEY_W: y -= velocidade; break;
            case ALLEGRO_KEY_S: y += velocidade; break;
            case ALLEGRO_KEY_A: x -= velocidade; break;
            case ALLEGRO_KEY_D: x += velocidade; break;
            case ALLEGRO_KEY_ESCAPE: rodando = 0; break;
            }
        }
    }


    al_clear_to_color(al_map_rgb(0, 0, 0));


    al_draw_filled_rectangle(x, y, x + 50, y + 50, al_map_rgb(255, 0, 0));


    al_flip_display();
}


al_destroy_display(display);
al_destroy_event_queue(fila);

return 0;
}