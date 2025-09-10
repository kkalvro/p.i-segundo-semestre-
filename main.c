#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#define LARGURA 2560
#define ALTURA 1440
#define MAX_TIROS 50

struct { float x, y, vel_x, vel_y; int largura, altura, direcao; bool no_chao; } jogador = {200, ALTURA-220, 0, 0, 80, 120, 1, true};

int main() {
    al_init(); al_install_keyboard(); al_init_primitives_addon();
    ALLEGRO_DISPLAY *tela = al_create_display(LARGURA, ALTURA);
    ALLEGRO_EVENT_QUEUE *fila_eventos = al_create_event_queue();
    ALLEGRO_TIMER *cronometro = al_create_timer(1.0/60);

    al_register_event_source(fila_eventos, al_get_display_event_source(tela));
    al_register_event_source(fila_eventos, al_get_timer_event_source(cronometro));
    al_register_event_source(fila_eventos, al_get_keyboard_event_source());
    al_start_timer(cronometro);

    bool rodando = true, redesenhar = true;
    while (rodando) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(fila_eventos, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) rodando = false;

        if (evento.type == ALLEGRO_EVENT_TIMER) {
            ALLEGRO_KEYBOARD_STATE teclado;
            al_get_keyboard_state(&teclado);
            jogador.vel_x = (al_key_down(&teclado, ALLEGRO_KEY_D) - al_key_down(&teclado, ALLEGRO_KEY_A)) * 6;
            if (jogador.vel_x) jogador.direcao = jogador.vel_x > 0 ? 1 : -1;
            if (al_key_down(&teclado, ALLEGRO_KEY_W) && jogador.no_chao) { jogador.vel_y = -25; jogador.no_chao = false; }

            if (!jogador.no_chao) jogador.vel_y += 0.8f;
            jogador.x += jogador.vel_x; jogador.y += jogador.vel_y;
            if (jogador.x < 0) jogador.x = 0;
            if (jogador.x > LARGURA - jogador.largura) jogador.x = LARGURA - jogador.largura;
            if (jogador.y >= ALTURA - 100 - jogador.altura) { jogador.y = ALTURA - 100 - jogador.altura; jogador.vel_y = 0; jogador.no_chao = true; }

            redesenhar = true;
        }

        if (redesenhar && al_is_event_queue_empty(fila_eventos)) {
            redesenhar = false;
            al_clear_to_color(al_map_rgb(180, 50, 50));
            al_draw_filled_rectangle(0, ALTURA-100, LARGURA, ALTURA, al_map_rgb(101, 67, 33));
            al_draw_filled_rectangle(jogador.x, jogador.y, jogador.x + jogador.largura, jogador.y + jogador.altura, al_map_rgb(0, 0, 0));
            al_flip_display();
        }
    }

    al_destroy_timer(cronometro); al_destroy_event_queue(fila_eventos); al_destroy_display(tela);
    return 0;
}