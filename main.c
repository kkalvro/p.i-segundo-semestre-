#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <stdbool.h>
#include <stdio.h>

int main() {
    if (!al_init()) {
        printf("Falha ao inicializar Allegro!\n");
        return -1;
    }

    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    al_install_keyboard();

    ALLEGRO_DISPLAY *display = al_create_display(800, 600);
    if (!display) {
        printf("Falha ao criar a janela!\n");
        return -1;
    }

    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60); // 60 FPS

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));

    ALLEGRO_FONT *fonte = al_create_builtin_font();

    bool rodando = true;
    bool mostrar_balao = false;

    al_start_timer(timer);

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }

        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_E) {
                mostrar_balao = !mostrar_balao;
            }
        }

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            al_clear_to_color(al_map_rgb(30, 30, 30));

            if (mostrar_balao) {
                al_draw_filled_rounded_rectangle(200, 200, 600, 300,
                                                 20, 20, al_map_rgb(255, 255, 255));
                al_draw_text(fonte, al_map_rgb(0, 0, 0), 400, 240,
                             ALLEGRO_ALIGN_CENTER, "Olá Aline! :)");
            }

            al_flip_display();
        }
    }

    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    return 0;
}

