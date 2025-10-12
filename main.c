#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

// CONSTANTES
#define LARGURA_TELA 1280
#define ALTURA_TELA 720
#define FPS 60
#define GRAVIDADE 0.4f
#define FORCA_PULO -12.5f
#define VELOCIDADE_MOVIMENTO 3.0f
#define MAX_PLATAFORMAS 8
#define TAMANHO_BLOCO_PLATAFORMA 32

// Entidades
#define MAX_TIROS_JOGADOR 50
#define MAX_TIROS_BOSS 40
#define MAX_CAPANGAS 4
#define MAX_TIROS_CAPANGA 20
#define VIDA_CAPANGA 50
#define VELOCIDADE_CAPANGA 1.25f

// ESTRUTURAS
typedef struct { float x, y, vel_x, vel_y; bool ativo; } Tiro;
typedef struct { float x, y; int largura, altura; } Plataforma;
typedef struct { float x, y, vel_x, vel_y; int largura, altura; bool no_chao, abaixado; int direcao, vida; } Jogador;
typedef struct { float x, y; int vida, vida_maxima; bool ativo, segunda_fase; int tempo_tiro, largura, altura; } Boss;
typedef struct { float x, y, vel_x, vel_y; int largura, altura, vida, tempo_tiro; bool ativo, no_chao; } Capanga;

// PROTÓTIPOS DE FUNÇÕES
bool inicializar();
void finalizar();
void inicializar_jogo();
void processar_entrada(ALLEGRO_EVENT *ev);
void atualizar_jogo();
void desenhar_jogo();
void verificar_colisoes();

// Funções Auxiliares
void atualizar_fisica_entidade(float *x, float *y, float *vel_y, int largura, int altura, bool *no_chao, bool ignora_plataformas);
void criar_tiro(Tiro tiros[], int max_tiros, float x, float y, float vel_x, float vel_y);
void criar_tiros_direcionados(Tiro tiros[], int max_tiros, float start_x, float start_y, float target_x, float target_y, float speed);

// Funções de Lógica
void atualizar_jogador();
void atualizar_boss();
void atualizar_capangas();
void atualizar_tiros(Tiro tiros[], int max_tiros);

// VARIÁVEIS GLOBAIS
ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_EVENT_QUEUE *event_queue = NULL;
ALLEGRO_TIMER *timer = NULL;
ALLEGRO_FONT *font = NULL;
ALLEGRO_COLOR cor_jogador, cor_tiro_jogador, cor_boss, cor_tiro_boss, cor_capanga, cor_tiro_capanga;
ALLEGRO_BITMAP *background_image = NULL;
ALLEGRO_BITMAP *platform_block_image = NULL;

Jogador jogador;
Boss boss;
Capanga capangas[MAX_CAPANGAS];
Tiro tiros_jogador[MAX_TIROS_JOGADOR];
Tiro tiros_boss[MAX_TIROS_BOSS];
Tiro tiros_capangas[MAX_TIROS_CAPANGA];

int pontuacao = 0;
int tempo_spawn_capanga = 0;

Plataforma plataformas[MAX_PLATAFORMAS] = {
    {50, 270, 350, 10}, {550, 545, 400, 10},
    {100, 120, 400, 10}, {150, 420, 400, 10},
    {50, 545, 400, 10},  {500, 270, 400, 10},
    {600, 120, 400, 10}, {650, 420, 400, 10}
};

int main() {
    if (!inicializar()) return -1;
    inicializar_jogo();

    bool rodando = true, redesenhar = true;
    al_start_timer(timer);

    while (rodando) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(event_queue, &evento);

        if (evento.type == ALLEGRO_EVENT_TIMER) {
            atualizar_jogo();
            redesenhar = true;
        } else if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        } else {
            processar_entrada(&evento);
        }

        if (redesenhar && al_is_event_queue_empty(event_queue)) {
            redesenhar = false;
            desenhar_jogo();
            al_flip_display();
        }
    }
    finalizar();
    return 0;
}

bool inicializar() {
    al_init();
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon();

    display = al_create_display(LARGURA_TELA, ALTURA_TELA);
    event_queue = al_create_event_queue();
    timer = al_create_timer(1.0 / FPS);
    font = al_create_builtin_font();

    if (!display || !event_queue || !timer || !font) {
        fprintf(stderr, "Falha ao inicializar Allegro.\n");
        return false;
    }

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    cor_jogador = al_map_rgb(0, 0, 0);
    cor_tiro_jogador = al_map_rgb(255, 255, 0);
    cor_boss = al_map_rgb(150, 0, 150);
    cor_tiro_boss = al_map_rgb(255, 100, 100);
    cor_capanga = al_map_rgb(50, 150, 50);
    cor_tiro_capanga = al_map_rgb(150, 255, 150);
    return true;
}

void finalizar() {
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(event_queue);
    al_destroy_display(display);
    if (background_image) {
        al_destroy_bitmap(background_image);
    }
    if (platform_block_image) { // USAR O IF PARA NÂO CRASHAR DENOVO O JOGO SE O PONTEIRO FOR NULLO
        al_destroy_bitmap(platform_block_image);
    }
}

void inicializar_jogo() {
    jogador = (Jogador){.x = 100, .y = ALTURA_TELA - 125, .largura = 40, .altura = 60, .no_chao = true, .abaixado = false, .direcao = 1, .vida = 100};
    boss = (Boss){.x = LARGURA_TELA - 300, .y = ALTURA_TELA - 550, .vida = 500, .vida_maxima = 500, .ativo = true, .largura = 300, .altura = 500};

    background_image = al_load_bitmap("assets/inferno.jpg");
    if (!background_image) {
        fprintf(stderr, "Problema ao carregar a imagem de background\n");
    }

    platform_block_image = al_load_bitmap("assets/floor.png");
    if (!platform_block_image) {
        fprintf(stderr, "Problema ao carregar a imagem da plataforma\n");
    }


    memset(tiros_jogador, 0, sizeof(tiros_jogador));
    memset(tiros_boss, 0, sizeof(tiros_boss));
    memset(capangas, 0, sizeof(capangas));
    memset(tiros_capangas, 0, sizeof(tiros_capangas));

    pontuacao = 0;
    tempo_spawn_capanga = 0;
}

// LÓGICA PRINCIPAL
void processar_entrada(ALLEGRO_EVENT *evento) {
    if (evento->type == ALLEGRO_EVENT_KEY_DOWN && evento->keyboard.keycode == ALLEGRO_KEY_SPACE) {
        criar_tiro(tiros_jogador, MAX_TIROS_JOGADOR, jogador.x + (jogador.direcao == 1 ? jogador.largura : 0), jogador.y + jogador.altura / 2, jogador.direcao * 6.0f, 0);
    }

    ALLEGRO_KEYBOARD_STATE kb;
    al_get_keyboard_state(&kb);

    jogador.vel_x = (al_key_down(&kb, ALLEGRO_KEY_D) - al_key_down(&kb, ALLEGRO_KEY_A)) * VELOCIDADE_MOVIMENTO;
    if (jogador.vel_x != 0) jogador.direcao = (jogador.vel_x > 0) ? 1 : -1;
    if (al_key_down(&kb, ALLEGRO_KEY_W) && jogador.no_chao) {
        jogador.vel_y = FORCA_PULO;
        jogador.no_chao = false;
    }
    jogador.abaixado = al_key_down(&kb, ALLEGRO_KEY_S);
}

void atualizar_jogo() {
    atualizar_jogador();
    atualizar_boss();
    atualizar_capangas();
    atualizar_tiros(tiros_jogador, MAX_TIROS_JOGADOR);
    atualizar_tiros(tiros_boss, MAX_TIROS_BOSS);
    atualizar_tiros(tiros_capangas, MAX_TIROS_CAPANGA);
    verificar_colisoes();

    if (boss.ativo && boss.vida <= boss.vida_maxima / 2) {
        tempo_spawn_capanga++;
        if (tempo_spawn_capanga > FPS * 4) {
            for (int i = 0; i < MAX_CAPANGAS; i++)
                if (!capangas[i].ativo) {
                    if ((rand() % 2 == 0))
                        capangas[i] = (Capanga){
                            .x = (25.0f), .y = 25, .largura = 25, .altura = 35, .vida = VIDA_CAPANGA, .ativo = true,
                            .tempo_tiro = rand() % 100
                        };
                    else
                        capangas[i] = (Capanga){
                            .x = (LARGURA_TELA - 50.0f), .y = 25, .largura = 25, .altura = 35, .vida = VIDA_CAPANGA, .ativo = true,
                            .tempo_tiro = rand() % 100
                        };
                    break;
                }
            tempo_spawn_capanga = 0;
        }
    }
}

void atualizar_jogador() {
    jogador.x += jogador.vel_x;
    if (jogador.x < 0) jogador.x = 0;
    if (jogador.x > LARGURA_TELA - jogador.largura) jogador.x = LARGURA_TELA - jogador.largura;

    atualizar_fisica_entidade(&jogador.x, &jogador.y, &jogador.vel_y, jogador.largura, jogador.altura, &jogador.no_chao, jogador.abaixado);
}

void atualizar_boss() {
    if (!boss.ativo) return;
    if (boss.vida <= boss.vida_maxima / 2 && !boss.segunda_fase) boss.segunda_fase = true;

    boss.tempo_tiro++;
    if (boss.tempo_tiro >= (boss.segunda_fase ? 30 : 60)) {
        float centro_boss_x = boss.x + boss.largura / 2.0f;
        float centro_boss_y = boss.y + boss.altura / 2.0f;
        float centro_jogador_x = jogador.x + jogador.largura / 2.0f;
        float centro_jogador_y = jogador.y + jogador.altura / 2.0f;

        criar_tiros_direcionados(tiros_boss, MAX_TIROS_BOSS, centro_boss_x, centro_boss_y, centro_jogador_x, centro_jogador_y, 4.0f);
        if (boss.segunda_fase)
             criar_tiros_direcionados(tiros_boss, MAX_TIROS_BOSS, centro_boss_x, centro_boss_y, centro_jogador_x, centro_jogador_y, 4.0f);

        boss.tempo_tiro = 0;
    }
}

void atualizar_capangas() {
    for (int i = 0; i < MAX_CAPANGAS; i++) {
        if (!capangas[i].ativo) continue;

        capangas[i].vel_x = (jogador.x > capangas[i].x) ? VELOCIDADE_CAPANGA : -VELOCIDADE_CAPANGA;
        capangas[i].x += capangas[i].vel_x;
        atualizar_fisica_entidade(&capangas[i].x, &capangas[i].y, &capangas[i].vel_y, capangas[i].largura, capangas[i].altura, &capangas[i].no_chao, false);

        capangas[i].tempo_tiro++;
        if (capangas[i].tempo_tiro > FPS * 2) {
            criar_tiros_direcionados(tiros_capangas, MAX_TIROS_CAPANGA, capangas[i].x + capangas[i].largura / 2.0f, capangas[i].y + capangas[i].altura / 2.0f, jogador.x + jogador.largura / 2.0f, jogador.y + jogador.altura / 2.0f, 3.0f);
            capangas[i].tempo_tiro = 0;
        }
    }
}

void atualizar_tiros(Tiro tiros[], int max_tiros) {
    for (int i = 0; i < max_tiros; i++) {
        if (!tiros[i].ativo) continue;
        tiros[i].x += tiros[i].vel_x;
        tiros[i].y += tiros[i].vel_y;
        if (tiros[i].x < 0 || tiros[i].x > LARGURA_TELA || tiros[i].y < 0 || tiros[i].y > ALTURA_TELA)
            tiros[i].ativo = false;
    }
}

// FUNÇÕES AUXILIARES
void atualizar_fisica_entidade(float *x, float *y, float *vel_y, int largura, int altura, bool *no_chao, bool ignora_plataformas) {
    if (!*no_chao) *vel_y += GRAVIDADE;
    *y += *vel_y;

    float chao_y_pos = ALTURA_TELA - 50; // Posição Y do "chão" base

    bool em_contato = false;

    // COLISÃO COM O CHÃO
    if (*y >= chao_y_pos - altura) {
        *y = chao_y_pos - altura;
        em_contato = true;
    } else if (*vel_y >= 0 && !ignora_plataformas) {
        // Colisão com as plataformas
        for (int i = 0; i < MAX_PLATAFORMAS; i++) {
            if (*x + largura > plataformas[i].x && *x < plataformas[i].x + plataformas[i].largura && *y + altura >= plataformas[i].y && *y + altura <= plataformas[i].y + 10) {
                *y = plataformas[i].y - altura;
                em_contato = true;
                break;
            }
        }
    }
    if (em_contato) *vel_y = 0;
    *no_chao = em_contato;
}

void criar_tiro(Tiro tiros[], int max_tiros, float x, float y, float vel_x, float vel_y) {
    for (int i = 0; i < max_tiros; i++) {
        if (!tiros[i].ativo) {
            tiros[i] = (Tiro){.x = x, .y = y, .vel_x = vel_x, .vel_y = vel_y, .ativo = true};
            break;
        }
    }
}

void criar_tiros_direcionados(Tiro tiros[], int max_tiros, float start_x, float start_y, float target_x, float target_y, float speed) {
    float dx = target_x - start_x;
    float dy = target_y - start_y;
    float dist = sqrt(dx * dx + dy * dy);
    if (dist > 0)
        criar_tiro(tiros, max_tiros, start_x, start_y, (dx / dist) * speed, (dy / dist) * speed);
}

bool colidiu(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void verificar_colisoes() {
    for (int i = 0; i < MAX_TIROS_JOGADOR; i++) {
        if (!tiros_jogador[i].ativo) continue;
        if (boss.ativo && colidiu(tiros_jogador[i].x, tiros_jogador[i].y, 4, 2, boss.x, boss.y, boss.largura, boss.altura)) {
            tiros_jogador[i].ativo = false; boss.vida -= 2; pontuacao += 10;
            if (boss.vida <= 0) { boss.ativo = false; pontuacao += 1000; }
        }
        for (int j = 0; j < MAX_CAPANGAS; j++) {
            if (capangas[j].ativo && colidiu(tiros_jogador[i].x, tiros_jogador[i].y, 4, 2, capangas[j].x, capangas[j].y, capangas[j].largura, capangas[j].altura)) {
                tiros_jogador[i].ativo = false; capangas[j].vida -= 20; pontuacao += 5;
                if (capangas[j].vida <= 0) { capangas[j].ativo = false; pontuacao += 50; }
                break;
            }
        }
    }

    for (int i = 0; i < MAX_TIROS_BOSS; i++)
        if (tiros_boss[i].ativo && colidiu(tiros_boss[i].x, tiros_boss[i].y, 6, 6, jogador.x, jogador.y, jogador.largura, jogador.altura)) {
            tiros_boss[i].ativo = false; jogador.vida -= 15;
            if (jogador.vida <= 0) inicializar_jogo();
        }

    for (int i = 0; i < MAX_TIROS_CAPANGA; i++)
        if (tiros_capangas[i].ativo && colidiu(tiros_capangas[i].x, tiros_capangas[i].y, 5, 5, jogador.x, jogador.y, jogador.largura, jogador.altura)) {
            tiros_capangas[i].ativo = false; jogador.vida -= 5;
            if (jogador.vida <= 0) inicializar_jogo();
        }

    if (boss.ativo && colidiu(jogador.x, jogador.y, jogador.largura, jogador.altura, boss.x, boss.y, boss.largura, boss.altura)) {
        jogador.vida = 0; inicializar_jogo();
    }
}

// DESENHO
void desenhar_jogo() {    if (background_image) {
        al_draw_scaled_bitmap(background_image,
                              0, 0, al_get_bitmap_width(background_image), al_get_bitmap_height(background_image),
                              0, 0, LARGURA_TELA, ALTURA_TELA, 0);
    }

    // CHÃO PRINCIPAL COM OS BLOCOS DO MARIO KK
    if (platform_block_image) {
        float chao_y_pos = ALTURA_TELA - 50;
        int num_blocos_chao = LARGURA_TELA / TAMANHO_BLOCO_PLATAFORMA;
        if (LARGURA_TELA % TAMANHO_BLOCO_PLATAFORMA != 0) num_blocos_chao++; // Cobre toda a largura do chão

        for (int i = 0; i < num_blocos_chao; i++) {
            al_draw_scaled_bitmap(platform_block_image,
                                  0, 0, al_get_bitmap_width(platform_block_image), al_get_bitmap_height(platform_block_image),
                                  i * TAMANHO_BLOCO_PLATAFORMA, chao_y_pos,
                                  TAMANHO_BLOCO_PLATAFORMA, 50, // Ajusta a altura para 50, como o chão original
                                  0);
        }
    }


    // PLATAFORMAS COM OS BLOCOS DO MARIO TBM
    if (platform_block_image) {
        for (int i = 0; i < MAX_PLATAFORMAS; i++) {
            int num_blocos_plataforma = plataformas[i].largura / TAMANHO_BLOCO_PLATAFORMA;
            if (plataformas[i].largura % TAMANHO_BLOCO_PLATAFORMA != 0) num_blocos_plataforma++;

            for (int j = 0; j < num_blocos_plataforma; j++) {
                al_draw_scaled_bitmap(platform_block_image,
                                      0, 0, al_get_bitmap_width(platform_block_image), al_get_bitmap_height(platform_block_image),
                                      plataformas[i].x + (j * TAMANHO_BLOCO_PLATAFORMA), plataformas[i].y,
                                      TAMANHO_BLOCO_PLATAFORMA, plataformas[i].altura, // Usa a altura original da plataforma
                                      0);
            }
        }
    }


    float altura_desenhada = jogador.abaixado ? jogador.altura / 2.0f : jogador.altura;
    al_draw_filled_rectangle(jogador.x, jogador.y + (jogador.abaixado ? altura_desenhada : 0), jogador.x + jogador.largura, jogador.y + jogador.altura, cor_jogador);

    if (boss.ativo) {
        al_draw_filled_rectangle(boss.x, boss.y, boss.x + boss.largura, boss.y + boss.altura, cor_boss);
        float vida_pc = (float)boss.vida / boss.vida_maxima;
        al_draw_filled_rectangle(boss.x, boss.y - 15, boss.x + boss.largura, boss.y - 5, al_map_rgb(100,0,0));
        al_draw_filled_rectangle(boss.x, boss.y - 15, boss.x + boss.largura * vida_pc, boss.y - 5, al_map_rgb(255,0,0));
    }
    for (int i = 0; i < MAX_CAPANGAS; i++)
        if (capangas[i].ativo)
            al_draw_filled_rectangle(capangas[i].x, capangas[i].y, capangas[i].x + capangas[i].largura, capangas[i].y + capangas[i].altura, cor_capanga);

    for (int i=0; i < MAX_TIROS_JOGADOR; i++) if (tiros_jogador[i].ativo) al_draw_filled_circle(tiros_jogador[i].x, tiros_jogador[i].y, 2, cor_tiro_jogador);
    for (int i=0; i < MAX_TIROS_BOSS; i++) if (tiros_boss[i].ativo) al_draw_filled_circle(tiros_boss[i].x, tiros_boss[i].y, 3, cor_tiro_boss);
    for (int i=0; i < MAX_TIROS_CAPANGA; i++) if (tiros_capangas[i].ativo) al_draw_filled_circle(tiros_capangas[i].x, tiros_capangas[i].y, 3, cor_tiro_capanga);

    char buffer[100];
    sprintf(buffer, "VIDA: %d", jogador.vida);
    al_draw_text(font, al_map_rgb(255, 255, 255), 20, 20, 0, buffer);
    sprintf(buffer, "PONTOS: %d", pontuacao);
    al_draw_text(font, al_map_rgb(255, 255, 255), 20, 50, 0, buffer);

    if (boss.ativo) {
        sprintf(buffer, "BOSS: %d/%d", boss.vida, boss.vida_maxima);
        al_draw_text(font, al_map_rgb(255, 255, 255), LARGURA_TELA - 150, 20, ALLEGRO_ALIGN_RIGHT, buffer);
    } else {
        al_draw_text(font, al_map_rgb(255, 255, 0), LARGURA_TELA/2, ALTURA_TELA/2, ALLEGRO_ALIGN_CENTER, "VILÃO DERROTADO!");
    }
}