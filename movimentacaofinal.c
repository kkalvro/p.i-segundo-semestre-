#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <stdbool.h>

#define GRAVIDADE 0.8f
#define FORCA_PULO -25.0f
#define VELOCIDADE_MOVIMENTO 6.0f
#define ALTURA_TELA 720
#define LARGURA_TELA 1280
#define FPS 60

typedef struct {
    float x, y;
    float vel_x, vel_y;
    int largura, altura;
    bool no_chao;
    int direcao;
} Jogador;

Jogador jogador;

void atualizar_fisica_jogador() {
    // Aplica gravidade
    if (!jogador.no_chao) {
        jogador.vel_y += GRAVIDADE;
    }
    jogador.y += jogador.vel_y;

    // Colisão com o chão
    float chao = ALTURA_TELA - 100;
    if (jogador.y >= chao - jogador.altura) {
        jogador.y = chao - jogador.altura;
        jogador.vel_y = 0;
        jogador.no_chao = true;
    }
    else {
        jogador.no_chao = false;
    }
}

void processar_entrada_movimento() {
    ALLEGRO_KEYBOARD_STATE kb;
    al_get_keyboard_state(&kb);

    // Movimento horizontal (A e D)
    jogador.vel_x = (al_key_down(&kb, ALLEGRO_KEY_D) - al_key_down(&kb, ALLEGRO_KEY_A)) * VELOCIDADE_MOVIMENTO;

    // Atualiza direção
    if (jogador.vel_x != 0) {
        jogador.direcao = (jogador.vel_x > 0) ? 1 : -1;
    }

    // Pulo (W) - apenas se estiver no chão
    if (al_key_down(&kb, ALLEGRO_KEY_W) && jogador.no_chao) {
        jogador.vel_y = FORCA_PULO;
        jogador.no_chao = false;
    }
}

void atualizar_jogador_movimento() {
    // Movimento horizontal
    jogador.x += jogador.vel_x;

    // Limites da tela
    if (jogador.x < 0) {
        jogador.x = 0;
    }
    if (jogador.x > LARGURA_TELA - jogador.largura) {
        jogador.x = LARGURA_TELA - jogador.largura;
    }

    // Atualiza física (gravidade e colisão com chão)
    atualizar_fisica_jogador();
}

int main(int argc, char** argv) {
    // Inicializar Allegro
    if (!al_init()) {
        return -1;
    }

    if (!al_install_keyboard()) {
        return -1;
    }

    if (!al_init_primitives_addon()) {
        return -1;
    }

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    if (!display) {
        return -1;
    }

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    // Inicializar jogador
    jogador.x = 200;
    jogador.y = ALTURA_TELA - 250;
    jogador.vel_x = 0;
    jogador.vel_y = 0;
    jogador.largura = 80;
    jogador.altura = 120;
    jogador.no_chao = true;
    jogador.direcao = 1;

    al_start_timer(timer);

    bool sair = false;
    bool redesenhar = true;

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_TIMER) {
            processar_entrada_movimento();
            atualizar_jogador_movimento();
            redesenhar = true;
        }

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            // Desenhar fundo
            al_clear_to_color(al_map_rgb(180, 50, 50));

            // Desenhar chão
            al_draw_filled_rectangle(0, ALTURA_TELA - 100, LARGURA_TELA, ALTURA_TELA, al_map_rgb(101, 67, 33));

            // Desenhar jogador
            al_draw_filled_rectangle(jogador.x, jogador.y,
                jogador.x + jogador.largura,
                jogador.y + jogador.altura,
                al_map_rgb(0, 0, 0));

            al_flip_display();
        }
    }

    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}