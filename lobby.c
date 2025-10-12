#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <stdbool.h>

#define GRAVIDADE 0.8f
#define FORCA_PULO -15.0f
#define VELOCIDADE_MOVIMENTO 6.0f
#define ALTURA_TELA 720
#define LARGURA_TELA 1280
#define FPS 60
#define NUM_SPRITES 8
#define TEMPO_ANIMACAO 5 // frames que cada sprite fica

typedef struct {
    float x, y;
    float vel_x, vel_y;
    int largura, altura;
    bool no_chao;
    int direcao; // 1 = direita, -1 = esquerda
    int sprite_corrida;
    int contador_frames_corrida;
    int sprite_pulo;
    int contador_frames_pulo;
} Jogador;

Jogador jogador;
ALLEGRO_BITMAP* sprites_corrida[NUM_SPRITES];
ALLEGRO_BITMAP* sprites_pulo[NUM_SPRITES];
ALLEGRO_BITMAP* sprite_parado = NULL;
ALLEGRO_BITMAP* fundo = NULL;

void atualizar_fisica_jogador() {
    if (!jogador.no_chao)
        jogador.vel_y += GRAVIDADE;

    jogador.y += jogador.vel_y;

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

    jogador.vel_x = (al_key_down(&kb, ALLEGRO_KEY_D) - al_key_down(&kb, ALLEGRO_KEY_A)) * VELOCIDADE_MOVIMENTO;

    if (jogador.vel_x != 0)
        jogador.direcao = (jogador.vel_x > 0) ? 1 : -1;

    if (al_key_down(&kb, ALLEGRO_KEY_W) && jogador.no_chao) {
        jogador.vel_y = FORCA_PULO;
        jogador.no_chao = false;
    }
}

void atualizar_jogador_movimento() {
    jogador.x += jogador.vel_x;

    if (jogador.x < 0) jogador.x = 0;
    if (jogador.x > LARGURA_TELA - jogador.largura)
        jogador.x = LARGURA_TELA - jogador.largura;

    atualizar_fisica_jogador();

    // Animação de corrida
    if (jogador.no_chao && jogador.vel_x != 0) {
        jogador.contador_frames_corrida++;
        if (jogador.contador_frames_corrida >= TEMPO_ANIMACAO) {
            jogador.contador_frames_corrida = 0;
            jogador.sprite_corrida = (jogador.sprite_corrida + 1) % NUM_SPRITES;
        }
    }
    else if (jogador.no_chao && jogador.vel_x == 0) {
        jogador.sprite_corrida = 0;
        jogador.contador_frames_corrida = 0;
    }

    // Animação de pulo
    if (!jogador.no_chao) {
        jogador.contador_frames_pulo++;
        if (jogador.contador_frames_pulo >= TEMPO_ANIMACAO) {
            jogador.contador_frames_pulo = 0;
            jogador.sprite_pulo = (jogador.sprite_pulo + 1) % NUM_SPRITES;
        }
    }
    else {
        jogador.sprite_pulo = 0;
        jogador.contador_frames_pulo = 0;
    }
}

int main(int argc, char** argv) {
    if (!al_init()) {
        printf("Erro ao inicializar Allegro.\n");
        return -1;
    }

    al_install_keyboard();
    al_init_primitives_addon();
    al_init_image_addon();
    al_init_native_dialog_addon();

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    if (!display) {
        printf("Erro ao criar display.\n");
        return -1;
    }

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    // Carrega fundo
    fundo = al_load_bitmap("assets/lobby.jpg");
    if (!fundo) {
        al_show_native_message_box(display, "Erro", "Imagem não encontrada",
            "Certifique-se de que 'assets/lobby.jpg' está na pasta 'assets'.",
            NULL, ALLEGRO_MESSAGEBOX_ERROR);
    }

    // Carrega sprites de corrida
    char caminho[100];
    for (int i = 0; i < NUM_SPRITES; i++) {
        snprintf(caminho, sizeof(caminho), "assets/correndo/run%d.png", i + 1);
        sprites_corrida[i] = al_load_bitmap(caminho);
        if (!sprites_corrida[i]) {
            al_show_native_message_box(display, "Erro", "Sprite de corrida não encontrado", caminho, NULL,
                ALLEGRO_MESSAGEBOX_ERROR);
            return -1;
        }
    }

    // Carrega sprites de pulo
    for (int i = 0; i < NUM_SPRITES; i++) {
        snprintf(caminho, sizeof(caminho), "assets/pulando/jump%d.png", i + 1);
        sprites_pulo[i] = al_load_bitmap(caminho);
        if (!sprites_pulo[i]) {
            al_show_native_message_box(display, "Erro", "Sprite de pulo não encontrado", caminho, NULL,
                ALLEGRO_MESSAGEBOX_ERROR);
            return -1;
        }
    }

    // Carrega sprite de parada
    sprite_parado = al_load_bitmap("assets/parada.png");
    if (!sprite_parado) {
        al_show_native_message_box(display, "Erro", "Sprite de parada não encontrado", "assets/parada.png", NULL,
            ALLEGRO_MESSAGEBOX_ERROR);
        return -1;
    }

    // Inicializa jogador
    jogador.x = 200;
    jogador.y = ALTURA_TELA - 250;
    jogador.vel_x = 0;
    jogador.vel_y = 0;
    jogador.largura = 120;
    jogador.altura = 160;
    jogador.no_chao = true;
    jogador.direcao = 1;
    jogador.sprite_corrida = 0;
    jogador.contador_frames_corrida = 0;
    jogador.sprite_pulo = 0;
    jogador.contador_frames_pulo = 0;

    al_start_timer(timer);

    bool sair = false;
    bool redesenhar = true;

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            sair = true;
        else if (evento.type == ALLEGRO_EVENT_TIMER) {
            processar_entrada_movimento();
            atualizar_jogador_movimento();
            redesenhar = true;
        }

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            // desenha fundo
            if (fundo)
                al_draw_scaled_bitmap(fundo, 0, 0,
                    al_get_bitmap_width(fundo),
                    al_get_bitmap_height(fundo),
                    0, 0,
                    LARGURA_TELA, ALTURA_TELA, 0);
            else
                al_clear_to_color(al_map_rgb(0, 0, 0));

            // Escolhe sprite de acordo com estado do jogador
            ALLEGRO_BITMAP* sprite;
            if (!jogador.no_chao) {
                sprite = sprites_pulo[jogador.sprite_pulo];
            }
            else if (jogador.vel_x != 0) {
                sprite = sprites_corrida[jogador.sprite_corrida];
            }
            else {
                sprite = sprite_parado;
            }

            // Desenha sprite com inversão se necessário
            if (jogador.direcao == 1) {
                al_draw_scaled_bitmap(sprite,
                    0, 0,
                    al_get_bitmap_width(sprite),
                    al_get_bitmap_height(sprite),
                    jogador.x, jogador.y,
                    jogador.largura, jogador.altura,
                    0);
            }
            else {
                al_draw_scaled_bitmap(sprite,
                    al_get_bitmap_width(sprite), 0,
                    -al_get_bitmap_width(sprite),
                    al_get_bitmap_height(sprite),
                    jogador.x, jogador.y,
                    jogador.largura, jogador.altura,
                    0);
            }

            al_flip_display();
        }
    }

    // Libera memória
    for (int i = 0; i < NUM_SPRITES; i++) {
        if (sprites_corrida[i]) al_destroy_bitmap(sprites_corrida[i]);
        if (sprites_pulo[i]) al_destroy_bitmap(sprites_pulo[i]);
    }
    if (sprite_parado) al_destroy_bitmap(sprite_parado);
    if (fundo) al_destroy_bitmap(fundo);
    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}
