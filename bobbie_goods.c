#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>

#define GRAVIDADE 0.8f
#define FORCA_PULO -15.0f
#define VELOCIDADE_MOVIMENTO 6.0f
#define ALTURA_TELA 720
#define LARGURA_TELA 1280
#define FPS 60
#define NUM_PLATAFORMAS 16
#define MAX_ESTADOS 6
#define MAX_SPRITES_PULO 8
#define MAX_SPRITES_CORRIDA 8
#define MAX_SPRITES_MORTE 5
#define MAX_FANTASMAS 3  // máximo de fantasmas simultâneos

typedef struct {
    float x, y;
    float largura, altura;
} Plataforma;

typedef struct {
    float x, y;
    float vel_x, vel_y;
    int largura, altura;
    bool no_chao;
    int direcao; // 1 = direita, -1 = esquerda
    bool abaixado;
} Jogador;

typedef struct {
    float x, y;
    float raio;
    float velocidade;
    float largura;
    float altura;
} Inimigo;

// ------------------- VARIÁVEIS -------------------
Jogador jogador;
Plataforma plataformas[NUM_PLATAFORMAS];
Inimigo fantasmas[MAX_FANTASMAS];
int num_fantasmas = 1;

bool jogador_travado = false;
bool jogador_morrendo = false;

ALLEGRO_BITMAP* imgs_estados[MAX_ESTADOS];
ALLEGRO_BITMAP* img_balde = NULL;
ALLEGRO_BITMAP* img_instrucoes = NULL;
ALLEGRO_BITMAP* img_fantasma = NULL;

ALLEGRO_BITMAP* sprites_pulo[MAX_SPRITES_PULO];
int frame_pulo = 0;
float tempo_frame_pulo = 0.0f;
float duracao_frame_pulo = 0.08f;

ALLEGRO_BITMAP* sprites_corrida[MAX_SPRITES_CORRIDA];
int frame_corrida = 0;
float tempo_frame_corrida = 0.0f;
float duracao_frame_corrida = 0.08f;

ALLEGRO_BITMAP* sprite_parado = NULL;

ALLEGRO_BITMAP* sprites_morte[MAX_SPRITES_MORTE];
int frame_morte = 0;
float tempo_frame_morte = 0.0f;
float duracao_frame_morte = 0.1f;

bool cena_instrucoes = true;

float img_largura = 180;
float img_altura = 180;
float balde_largura = 50;
float balde_altura = 50;

// Configuração dos fantasmas
float fantasma_largura_padrao = 150;
float fantasma_altura_padrao = 150;
float fantasma_velocidade_padrao = 2.5f;

int estado_atual = 0;
bool balde_pego = false;
int plataforma_proximo_balde[MAX_ESTADOS] = { 0, 1, 2, 3, 4, -1 };

// ------------------- FÍSICA -------------------
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
    else jogador.no_chao = false;

    for (int i = 0; i < NUM_PLATAFORMAS; i++) {
        Plataforma p = plataformas[i];
        bool colide_horizontalmente = (jogador.x + jogador.largura > p.x) &&
            (jogador.x < p.x + p.largura);
        bool tocando_por_cima = (jogador.y + jogador.altura >= p.y) &&
            (jogador.y + jogador.altura <= p.y + p.altura + jogador.vel_y) &&
            (jogador.vel_y >= 0);
        if (jogador.abaixado) continue;
        if (colide_horizontalmente && tocando_por_cima) {
            jogador.y = p.y - jogador.altura;
            jogador.vel_y = 0;
            jogador.no_chao = true;
        }
    }
}

// ------------------- CONTROLES -------------------
void processar_entrada_movimento() {
    ALLEGRO_KEYBOARD_STATE kb;
    al_get_keyboard_state(&kb);

    if (jogador_travado) {
        if (al_key_down(&kb, ALLEGRO_KEY_R)) {
            jogador_travado = false;
            jogador_morrendo = false;
            jogador.x = 200;
            jogador.y = ALTURA_TELA - 200;
            estado_atual = 0;
            balde_pego = false;
            frame_morte = 0;
            tempo_frame_morte = 0;
            num_fantasmas = 1;
            // Resetar posição dos fantasmas
            for (int i = 0; i < MAX_FANTASMAS; i++) {
                fantasmas[i].x = LARGURA_TELA / 2;
                fantasmas[i].y = ALTURA_TELA / 2;
                fantasmas[i].velocidade = fantasma_velocidade_padrao;
                fantasmas[i].largura = fantasma_largura_padrao;
                fantasmas[i].altura = fantasma_altura_padrao;
            }
        }
        return;
    }

    jogador.vel_x = (al_key_down(&kb, ALLEGRO_KEY_D) - al_key_down(&kb, ALLEGRO_KEY_A)) * VELOCIDADE_MOVIMENTO;
    if (jogador.vel_x != 0)
        jogador.direcao = (jogador.vel_x > 0) ? 1 : -1;

    if (al_key_down(&kb, ALLEGRO_KEY_W) && jogador.no_chao) {
        jogador.vel_y = FORCA_PULO;
        jogador.no_chao = false;
    }

    jogador.abaixado = al_key_down(&kb, ALLEGRO_KEY_S);

    if (al_key_down(&kb, ALLEGRO_KEY_E)) {
        float img_x = (LARGURA_TELA - img_largura) / 2;
        float img_y = ALTURA_TELA - 100 - img_altura;
        if (jogador.x + jogador.largura > img_x && jogador.x < img_x + img_largura &&
            jogador.y + jogador.altura > img_y && jogador.y < img_y + img_altura) {
            if (balde_pego && estado_atual < 5) {
                estado_atual++;
                balde_pego = false;
                // Atualizar número de fantasmas conforme estado
                if (estado_atual == 1) num_fantasmas = 2;
                if (estado_atual == 2) num_fantasmas = 3;
                if (estado_atual >= 5) num_fantasmas = 0;
            }
        }
    }
}

// ------------------- MOVIMENTO -------------------
void atualizar_jogador_movimento() {
    if (jogador_travado || jogador_morrendo) return;

    jogador.x += jogador.vel_x;
    if (jogador.x < 0) jogador.x = 0;
    if (jogador.x > LARGURA_TELA - jogador.largura) jogador.x = LARGURA_TELA - jogador.largura;

    atualizar_fisica_jogador();
}

// ------------------- ANIMAÇÕES -------------------
void atualizar_animacao_pulo(float delta) {
    if (!jogador.no_chao) {
        tempo_frame_pulo += delta;
        if (tempo_frame_pulo >= duracao_frame_pulo) {
            tempo_frame_pulo = 0;
            frame_pulo++;
            if (frame_pulo >= MAX_SPRITES_PULO)
                frame_pulo = MAX_SPRITES_PULO - 1;
        }
    }
    else {
        frame_pulo = 0;
        tempo_frame_pulo = 0;
    }
}

void atualizar_animacao_corrida(float delta) {
    if (jogador.no_chao && jogador.vel_x != 0) {
        tempo_frame_corrida += delta;
        if (tempo_frame_corrida >= duracao_frame_corrida) {
            tempo_frame_corrida = 0;
            frame_corrida++;
            if (frame_corrida >= MAX_SPRITES_CORRIDA)
                frame_corrida = 0;
        }
    }
    else {
        frame_corrida = 0;
        tempo_frame_corrida = 0;
    }
}

void atualizar_animacao_morte(float delta) {
    if (jogador_morrendo) {
        tempo_frame_morte += delta;
        if (tempo_frame_morte >= duracao_frame_morte) {
            tempo_frame_morte = 0;
            frame_morte++;
            if (frame_morte >= MAX_SPRITES_MORTE) {
                frame_morte = MAX_SPRITES_MORTE - 1; // mantém última imagem
                jogador_travado = true; // trava após terminar animação
            }
        }
    }
}

// ------------------- INIMIGO -------------------
void atualizar_inimigo() {
    if (jogador_travado || estado_atual == 5 || num_fantasmas == 0) return;

    for (int i = 0; i < num_fantasmas; i++) {
        float dx = (jogador.x + jogador.largura / 2) - fantasmas[i].x;
        float dy = (jogador.y + jogador.altura / 2) - fantasmas[i].y;
        float distancia = sqrtf(dx * dx + dy * dy);

        if (distancia > 0) {
            fantasmas[i].x += (dx / distancia) * fantasmas[i].velocidade;
            fantasmas[i].y += (dy / distancia) * fantasmas[i].velocidade;
        }

        float dx_c = (jogador.x + jogador.largura / 2) - fantasmas[i].x;
        float dy_c = (jogador.y + jogador.altura / 2) - fantasmas[i].y;
        float distancia_c = sqrtf(dx_c * dx_c + dy_c * dy_c);

        if (distancia_c < fantasmas[i].raio + jogador.largura / 2 && !jogador_morrendo) {
            jogador_morrendo = true;
            frame_morte = 0;
            tempo_frame_morte = 0;
        }
    }
}

// ------------------- MAIN -------------------
int main() {
    if (!al_init()) return -1;
    if (!al_install_keyboard()) return -1;
    if (!al_init_primitives_addon()) return -1;
    if (!al_init_image_addon()) return -1;
    if (!al_install_mouse()) return -1;

    ALLEGRO_DISPLAY* display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    if (!display) return -1;

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    // ---------- CARREGAR IMAGENS ----------
    const char* arquivos_estados[MAX_ESTADOS] = {
        "assets/estado_zero.png",
        "assets/estado_um.png",
        "assets/estado_dois.png",
        "assets/estado_tres.png",
        "assets/estado_quatro.png",
        "assets/estado_cinco.png"
    };
    for (int i = 0; i < MAX_ESTADOS; i++) {
        imgs_estados[i] = al_load_bitmap(arquivos_estados[i]);
        if (!imgs_estados[i]) { printf("Erro: não foi possível carregar %s\n", arquivos_estados[i]); return -1; }
    }

    img_balde = al_load_bitmap("assets/balde.png");
    img_instrucoes = al_load_bitmap("assets/instrucoes.jpg");
    img_fantasma = al_load_bitmap("assets/fantasma.png");
    sprite_parado = al_load_bitmap("assets/parada.png");

    // Sprites do pulo
    const char* arquivos_pulo[MAX_SPRITES_PULO] = {
        "assets/pulando/jump1.png","assets/pulando/jump2.png","assets/pulando/jump3.png",
        "assets/pulando/jump4.png","assets/pulando/jump5.png","assets/pulando/jump6.png",
        "assets/pulando/jump7.png","assets/pulando/jump8.png"
    };
    for (int i = 0; i < MAX_SPRITES_PULO; i++) {
        sprites_pulo[i] = al_load_bitmap(arquivos_pulo[i]);
        if (!sprites_pulo[i]) { printf("Erro: não foi possível carregar %s\n", arquivos_pulo[i]); return -1; }
    }

    // Sprites de corrida
    const char* arquivos_corrida[MAX_SPRITES_CORRIDA] = {
        "assets/correndo/run1.png","assets/correndo/run2.png","assets/correndo/run3.png",
        "assets/correndo/run4.png","assets/correndo/run5.png","assets/correndo/run6.png",
        "assets/correndo/run7.png","assets/correndo/run8.png"
    };
    for (int i = 0; i < MAX_SPRITES_CORRIDA; i++) {
        sprites_corrida[i] = al_load_bitmap(arquivos_corrida[i]);
        if (!sprites_corrida[i]) { printf("Erro: não foi possível carregar %s\n", arquivos_corrida[i]); return -1; }
    }

    // Sprites de morte
    const char* arquivos_morte[MAX_SPRITES_MORTE] = {
        "assets/morrendo/die1.png","assets/morrendo/die2.png","assets/morrendo/die3.png",
        "assets/morrendo/die4.png","assets/morrendo/die5.png"
    };
    for (int i = 0; i < MAX_SPRITES_MORTE; i++) {
        sprites_morte[i] = al_load_bitmap(arquivos_morte[i]);
        if (!sprites_morte[i]) { printf("Erro: não foi possível carregar %s\n", arquivos_morte[i]); return -1; }
    }

    // ---------- INICIALIZAÇÃO ----------
    jogador.x = 200; jogador.y = ALTURA_TELA - 200; jogador.vel_x = 0; jogador.vel_y = 0;
    jogador.largura = 50; jogador.altura = 60; jogador.no_chao = true; jogador.direcao = 1; jogador.abaixado = false;

    plataformas[0] = (Plataforma){ 70, 100, 100, 15 };//primeiro balde
    plataformas[1] = (Plataforma){ 1100, 100, 100, 15 };//segundo balde
    plataformas[2] = (Plataforma){ 50, 460, 100, 15 }; //terceiro balde
    plataformas[3] = (Plataforma){ 700, 250, 100, 15 }; //quarto balde
    plataformas[4] = (Plataforma){ 1200, 500, 100, 15 }; // quinto balde
    plataformas[5] = (Plataforma){ 360, 480, 100, 15 };
    plataformas[6] = (Plataforma){ 260, 390, 100, 15 };
    plataformas[7] = (Plataforma){ 180, 180, 100, 15 };
    plataformas[8] = (Plataforma){ 900, 140, 100, 15 };
    plataformas[9] = (Plataforma){ 490, 300, 100, 15 };
    plataformas[10] = (Plataforma){ 970, 430, 100, 15 };
    plataformas[11] = (Plataforma){ 920, 260, 100, 15 };
    plataformas[12] = (Plataforma){ 800, 400, 100, 15 };
    plataformas[13] = (Plataforma){ 1100, 300, 100, 15 };
    plataformas[14] = (Plataforma){ 50, 320, 100, 15 };
    plataformas[15] = (Plataforma){ 400, 200, 100, 15 };

    // Inicializar fantasmas
    for (int i = 0; i < MAX_FANTASMAS; i++) {
        fantasmas[i].x = LARGURA_TELA / 2;
        fantasmas[i].y = ALTURA_TELA / 2;
        fantasmas[i].velocidade = fantasma_velocidade_padrao;
        fantasmas[i].largura = fantasma_largura_padrao;
        fantasmas[i].altura = fantasma_altura_padrao;
        fantasmas[i].raio = fantasmas[i].largura / 2;
    }

    al_start_timer(timer);

    bool sair = false;
    bool redesenhar = true;

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && cena_instrucoes) {
            cena_instrucoes = false;
        }
        else if (evento.type == ALLEGRO_EVENT_TIMER && !cena_instrucoes) {
            processar_entrada_movimento();
            atualizar_jogador_movimento();
            atualizar_inimigo();

            float delta = 1.0 / FPS;
            atualizar_animacao_pulo(delta);
            atualizar_animacao_corrida(delta);
            atualizar_animacao_morte(delta);

            if (!balde_pego && estado_atual < 5) {
                int plat_index = plataforma_proximo_balde[estado_atual];
                float balde_x = plataformas[plat_index].x + (plataformas[plat_index].largura - balde_largura) / 2;
                float balde_y = plataformas[plat_index].y - balde_altura;

                float dx = (jogador.x + jogador.largura / 2) - (balde_x + balde_largura / 2);
                float dy = (jogador.y + jogador.altura / 2) - (balde_y + balde_altura / 2);
                float distancia = sqrtf(dx * dx + dy * dy);
                if (distancia < (jogador.largura + balde_largura) / 2) {
                    balde_pego = true;
                }
            }

            redesenhar = true;
        }

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            if (cena_instrucoes) {
                al_clear_to_color(al_map_rgb(0, 0, 0));
                al_draw_scaled_bitmap(
                    img_instrucoes,
                    0, 0,
                    al_get_bitmap_width(img_instrucoes),
                    al_get_bitmap_height(img_instrucoes),
                    0, 0,
                    LARGURA_TELA, ALTURA_TELA,
                    0
                );
                al_flip_display();
            }
            else {
                al_clear_to_color(al_map_rgb(137, 137, 137));

                float img_x = (LARGURA_TELA - img_largura) / 2;
                float img_y = ALTURA_TELA - 100 - img_altura;
                al_draw_scaled_bitmap(
                    imgs_estados[estado_atual],
                    0, 0,
                    al_get_bitmap_width(imgs_estados[estado_atual]),
                    al_get_bitmap_height(imgs_estados[estado_atual]),
                    img_x, img_y,
                    img_largura, img_altura,
                    0
                );

                al_draw_filled_rectangle(0, ALTURA_TELA - 100, LARGURA_TELA, ALTURA_TELA, al_map_rgb(101, 67, 33));

                for (int i = 0; i < NUM_PLATAFORMAS; i++) {
                    al_draw_filled_rectangle(
                        plataformas[i].x, plataformas[i].y,
                        plataformas[i].x + plataformas[i].largura, plataformas[i].y + plataformas[i].altura,
                        al_map_rgb(70, 70, 70)
                    );
                }

                if (!balde_pego && estado_atual < 5) {
                    int plat_index = plataforma_proximo_balde[estado_atual];
                    float balde_x = plataformas[plat_index].x + (plataformas[plat_index].largura - balde_largura) / 2;
                    float balde_y = plataformas[plat_index].y - balde_altura;
                    al_draw_scaled_bitmap(
                        img_balde,
                        0, 0,
                        al_get_bitmap_width(img_balde),
                        al_get_bitmap_height(img_balde),
                        balde_x, balde_y,
                        balde_largura, balde_altura,
                        0
                    );
                }

                // Fantasmas
                for (int i = 0; i < num_fantasmas; i++) {
                    al_draw_scaled_bitmap(
                        img_fantasma,
                        0, 0,
                        al_get_bitmap_width(img_fantasma),
                        al_get_bitmap_height(img_fantasma),
                        fantasmas[i].x - fantasmas[i].largura / 2,
                        fantasmas[i].y - fantasmas[i].altura / 2,
                        fantasmas[i].largura,
                        fantasmas[i].altura,
                        0
                    );
                }

                // Jogador
                ALLEGRO_BITMAP* sprite_jogador = NULL;
                if (jogador_morrendo) sprite_jogador = sprites_morte[frame_morte];
                else if (!jogador.no_chao) sprite_jogador = sprites_pulo[frame_pulo];
                else if (jogador.vel_x != 0) sprite_jogador = sprites_corrida[frame_corrida];
                else sprite_jogador = sprite_parado;

                if (!sprite_jogador) {
                    al_draw_filled_rectangle(
                        jogador.x, jogador.y,
                        jogador.x + jogador.largura, jogador.y + jogador.altura,
                        jogador_travado ? al_map_rgb(100, 0, 0) : al_map_rgb(0, 0, 0)
                    );
                }
                else {
                    if (jogador.direcao == 1)
                        al_draw_scaled_bitmap(sprite_jogador, 0, 0, al_get_bitmap_width(sprite_jogador), al_get_bitmap_height(sprite_jogador), jogador.x, jogador.y, jogador.largura, jogador.altura, 0);
                    else
                        al_draw_scaled_bitmap(sprite_jogador, 0, 0, al_get_bitmap_width(sprite_jogador), al_get_bitmap_height(sprite_jogador), jogador.x + jogador.largura, jogador.y, -jogador.largura, jogador.altura, 0);
                }

                al_flip_display();
            }
        }
    }

    for (int i = 0; i < MAX_ESTADOS; i++) al_destroy_bitmap(imgs_estados[i]);
    al_destroy_bitmap(img_balde);
    al_destroy_bitmap(img_instrucoes);
    al_destroy_bitmap(img_fantasma);
    al_destroy_bitmap(sprite_parado);
    for (int i = 0; i < MAX_SPRITES_PULO; i++) al_destroy_bitmap(sprites_pulo[i]);
    for (int i = 0; i < MAX_SPRITES_CORRIDA; i++) al_destroy_bitmap(sprites_corrida[i]);
    for (int i = 0; i < MAX_SPRITES_MORTE; i++) al_destroy_bitmap(sprites_morte[i]);

    al_destroy_display(display);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    return 0;
}