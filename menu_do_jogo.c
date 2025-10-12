#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>

#define SCREEN_W 1280
#define SCREEN_H 720
#define FPS 60

int main(void) {
    if (!al_init()) {
        fprintf(stderr, "Falha ao inicializar Allegro!\n");
        return -1;
    }

    al_init_image_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_mouse();

    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    if (!display) {
        fprintf(stderr, "Falha ao criar display!\n");
        return -1;
    }


    ALLEGRO_BITMAP* background = al_load_bitmap("assets/background.jpg");
    ALLEGRO_BITMAP* instrucoes = al_load_bitmap("assets/instrucoes.jpg");
    if (!background || !instrucoes) {
        al_show_native_message_box(display, "Erro", "Falha ao carregar imagem.",
            "Verifique se os arquivos 'background.jpg' e 'instrucoes.jpg' existem na pasta assets.",
            NULL, ALLEGRO_MESSAGEBOX_ERROR);
        al_destroy_display(display);
        return -1;
    }

    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_mouse_event_source());

    al_start_timer(timer);

    bool running = true;
    bool redraw = true;

    //Configurações dos retângulos
    float rect_w = 354;
    float rect_h = 100;
    float spacing = 23;
    float pos_x = 150;
    float pos_y = 279;
    ALLEGRO_COLOR red = al_map_rgba(0, 0, 0, 0);

    int scene = 0; // 0 = tela principal, 1 = instrucoes

    while (running) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            running = false;

        else if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            running = false;

        else if (event.type == ALLEGRO_EVENT_TIMER)
            redraw = true;

        else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            float mouse_x = event.mouse.x;
            float mouse_y = event.mouse.y;

            if (scene == 0) {
                // Último retângulo (fechar)
                float last_top = pos_y + 2 * (rect_h + spacing);
                float last_bottom = last_top + rect_h;
                if (mouse_x >= pos_x && mouse_x <= pos_x + rect_w &&
                    mouse_y >= last_top && mouse_y <= last_bottom) {
                    running = false;
                }

                // Retângulo do meio (abrir instruções)
                float middle_top = pos_y + rect_h + spacing;
                float middle_bottom = middle_top + rect_h;
                if (mouse_x >= pos_x && mouse_x <= pos_x + rect_w &&
                    mouse_y >= middle_top && mouse_y <= middle_bottom) {
                    scene = 1;
                }
            }
            else if (scene == 1) {
                // volta pro menu com o clique do mouse nas instruções
                scene = 0;
            }
        }

        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;

            if (scene == 0) {
                al_draw_bitmap(background, 0, 0, 0);
				//desenha os retângulos
                for (int i = 0; i < 3; i++) {
                    float y = pos_y + i * (rect_h + spacing);
                    al_draw_filled_rectangle(pos_x, y, pos_x + rect_w, y + rect_h, red);
                }
            }
            else if (scene == 1) {
                al_draw_bitmap(instrucoes, 0, 0, 0);
            }

            al_flip_display();
        }
    }

    al_destroy_bitmap(background);
    al_destroy_bitmap(instrucoes);
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);
    al_destroy_timer(timer);
    al_shutdown_primitives_addon();

    return 0;
}