#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <stdbool.h>

#define SCREEN_W 1280
#define SCREEN_H 720
#define FPS 60

#define GRAVIDADE 0.8f
#define FORCA_PULO -15.0f
#define VELOCIDADE_MOVIMENTO 6.0f
#define ALTURA_TELA SCREEN_H
#define LARGURA_TELA SCREEN_W
#define NUM_SPRITES 8
#define TEMPO_ANIMACAO 5

// Variáveis globais para controle de chaves
bool chave_quebra_cabeca = false;
bool chave_memoria = false;


typedef struct {
    float x, y;
    float largura, altura;
    bool jogador_proximo;
    char nome[50];
} Porta;

typedef struct {
    float x, y;
    float vel_x, vel_y;
    int largura, altura;
    bool no_chao;
    int direcao;
    int sprite_corrida;
    int contador_frames_corrida;
    int sprite_pulo;
    int contador_frames_pulo;
} Jogador;

// ----------------------- Variáveis Globais -----------------------
Jogador jogador;
ALLEGRO_BITMAP* sprites_corrida[NUM_SPRITES] = { NULL };
ALLEGRO_BITMAP* sprites_pulo[NUM_SPRITES] = { NULL };
ALLEGRO_BITMAP* sprite_parado = NULL;
ALLEGRO_BITMAP* fundo = NULL;
ALLEGRO_FONT* font = NULL;

// Array de portas no lobby
Porta portas[3];
int numero_portas = 3;

// Variáveis para os quadrinhos
ALLEGRO_BITMAP* quadrinho_um = NULL;
ALLEGRO_BITMAP* quadrinho_dois = NULL;
ALLEGRO_BITMAP* quadrinho_tres = NULL;
ALLEGRO_BITMAP* quadrinho_quatro = NULL;
ALLEGRO_BITMAP* quadrinho_cinco = NULL;
ALLEGRO_BITMAP* quadrinho_seis = NULL;
ALLEGRO_BITMAP* quadrinho_oito = NULL;
ALLEGRO_BITMAP* quadrinho_nove = NULL;
ALLEGRO_BITMAP* quadrinho_dez = NULL;

int cena_quadrinho = 0;

// ----------------------- Estruturas e funções do Quebra-Cabeça -----------------------
typedef struct {
    ALLEGRO_BITMAP* bmp;
    float x, y;
    float target_x, target_y;
    int placed;
    int w, h;
} Piece;

typedef struct {
    const char* filename;
    int time_limit_sec;
} PhaseDef;

static void shuffle_positions_outside(Piece* pieces, int total, int piece_w_s, int grid_x, int grid_y, int grid_total_h) {
    int maxX = grid_x - piece_w_s - 5;
    if (maxX < 0) maxX = 0;
    for (int i = 0; i < total; i++) {
        pieces[i].placed = 0;
        pieces[i].x = (float)(rand() % (maxX + 1));
        pieces[i].y = (float)(grid_y + (rand() % ((grid_total_h - piece_w_s > 0) ? (grid_total_h - piece_w_s) : 1)));
    }
}

// ----------------------- Função para mostrar quadrinho após quebra-cabeça -----------------------
void mostrar_quadrinho_seis(ALLEGRO_DISPLAY* display) {
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    bool sair = false;
    bool redesenhar = true;

    if (!quadrinho_seis) {
        quadrinho_seis = al_load_bitmap("assets/6.jpg");
        if (!quadrinho_seis) {
            printf("ERRO: Não foi possível carregar assets/6.jpg\n");
        }
    }

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE ||
                evento.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                sair = true;
            }
        }
        else if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            sair = true;
        }

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            if (quadrinho_seis) {
                al_draw_scaled_bitmap(quadrinho_seis, 0, 0,
                    al_get_bitmap_width(quadrinho_seis),
                    al_get_bitmap_height(quadrinho_seis),
                    0, 0, SCREEN_W, SCREEN_H, 0);
            }
            else {
                al_clear_to_color(al_map_rgb(0, 0, 0));
                ALLEGRO_FONT* font_temp = al_create_builtin_font();
                if (font_temp) {
                    al_draw_text(font_temp, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2,
                        ALLEGRO_ALIGN_CENTER, "Imagem 6.jpg não encontrada");
                    al_destroy_font(font_temp);
                }
            }

            ALLEGRO_FONT* font_temp = al_create_builtin_font();
            if (font_temp) {
                al_draw_text(font_temp, al_map_rgb(255, 255, 255),
                    SCREEN_W / 2, SCREEN_H - 50,
                    ALLEGRO_ALIGN_CENTER, "Pressione ENTER, ESPAÇO ou clique para voltar ao lobby");
                al_destroy_font(font_temp);
            }

            al_flip_display();
        }
    }

    al_destroy_event_queue(queue);
}

// ----------------------- Função do Quebra-Cabeça -----------------------
int executar_quebra_cabeca(ALLEGRO_DISPLAY* display_main) {
    srand((unsigned)time(NULL));

    ALLEGRO_DISPLAY* display = display_main;
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);

    ALLEGRO_BITMAP* instrucoes_img = al_load_bitmap("assets/instrucoesqdc.jpg");
    ALLEGRO_BITMAP* fundo_puzzle = al_load_bitmap("assets/fundo.jpg");

    if (!instrucoes_img) {
        al_show_native_message_box(display, "Erro", "Não foi possível carregar assets/instrucoesqdc.jpg", NULL, NULL, 0);
        return 0;
    }
    if (!fundo_puzzle) {
        al_show_native_message_box(display, "Erro", "Não foi possível carregar assets/fundo.jpg", NULL, NULL, 0);
        return 0;
    }

    bool started = false;
    while (!started) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            al_destroy_timer(timer);
            al_destroy_event_queue(queue);
            al_destroy_bitmap(instrucoes_img);
            al_destroy_bitmap(fundo_puzzle);
            al_destroy_font(font);
            return 0;
        }
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER || ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                started = true;
            }
        }
        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            started = true;
        }

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            al_clear_to_color(al_map_rgb(0, 0, 0));
            al_draw_scaled_bitmap(instrucoes_img, 0, 0,
                al_get_bitmap_width(instrucoes_img), al_get_bitmap_height(instrucoes_img),
                0, 0, SCREEN_W, SCREEN_H, 0);
            al_flip_display();
        }
    }

    PhaseDef phases[3] = {
        { "assets/exemplo_arte_1.jpg", 120 },
        { "assets/exemplo_arte_2.jpg", 90 },
        { "assets/exemplo_arte_3.jpg", 60 }
    };

    bool todas_fases_concluidas = false;

    for (int phase_idx = 0; phase_idx < 3; ++phase_idx) {
        bool retry_phase = true;
        bool fase_concluida = false;

        while (retry_phase && !fase_concluida) {
            retry_phase = false;

            const char* image_path = phases[phase_idx].filename;
            int TIME_LIMIT = phases[phase_idx].time_limit_sec;

            ALLEGRO_BITMAP* source = al_load_bitmap(image_path);
            if (!source) {
                char err[256];
                snprintf(err, sizeof(err), "Falha ao carregar: %s", image_path);
                al_show_native_message_box(display, "Erro", err, NULL, NULL, 0);
                al_destroy_bitmap(instrucoes_img);
                al_destroy_bitmap(fundo_puzzle);
                al_destroy_font(font);
                al_destroy_timer(timer);
                al_destroy_event_queue(queue);
                return 0;
            }

            const int rows = 5, cols = 5;
            const int total = rows * cols;
            int src_w = al_get_bitmap_width(source);
            int src_h = al_get_bitmap_height(source);
            int piece_w = src_w / cols;
            int piece_h = src_h / rows;

            const int ref_width = 320;
            float ref_scale = (float)ref_width / (float)src_w;
            int ref_height = (int)(src_h * ref_scale + 0.5f);

            float maxGridW = SCREEN_W * 0.50f;
            float maxGridH = SCREEN_H * 0.85f;
            float scaleX = maxGridW / (float)src_w;
            float scaleY = maxGridH / (float)src_h;
            float scale = (scaleX < scaleY) ? scaleX : scaleY;
            if (scale > 1.0f) scale = 1.0f;

            int piece_w_s = (int)(piece_w * scale + 0.5f);
            int piece_h_s = (int)(piece_h * scale + 0.5f);

            int grid_total_w = cols * piece_w_s + (cols - 1) * 2;
            int grid_total_h = rows * piece_h_s + (rows - 1) * 2;
            int grid_x = (SCREEN_W - grid_total_w - ref_width - 20) / 2;
            int grid_y = (SCREEN_H - grid_total_h) / 2;

            const int gap = 2;

            Piece* pieces = (Piece*)malloc(sizeof(Piece) * total);
            if (!pieces) {
                al_show_native_message_box(display, "Erro", "Memória insuficiente", NULL, NULL, 0);
                al_destroy_bitmap(source);
                al_destroy_bitmap(instrucoes_img);
                al_destroy_bitmap(fundo_puzzle);
                al_destroy_font(font);
                al_destroy_timer(timer);
                al_destroy_event_queue(queue);
                return 0;
            }

            int idx = 0;
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    ALLEGRO_BITMAP* sub = al_create_sub_bitmap(source, c * piece_w, r * piece_h, piece_w, piece_h);
                    pieces[idx].bmp = sub;
                    pieces[idx].w = piece_w;
                    pieces[idx].h = piece_h;
                    pieces[idx].target_x = (float)(grid_x + c * (piece_w_s + gap));
                    pieces[idx].target_y = (float)(grid_y + r * (piece_h_s + gap));
                    pieces[idx].placed = 0;
                    idx++;
                }
            }

            shuffle_positions_outside(pieces, total, piece_w_s, grid_x, grid_y, grid_total_h);

            int grabbed = -1;
            float grab_off_x = 0.0f, grab_off_y = 0.0f;
            int victory_flag = 0;
            float elapsed_time = 0.0f;
            bool phase_running = true;

            while (phase_running) {
                ALLEGRO_EVENT ev;
                al_wait_for_event(queue, &ev);

                if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                    phase_running = false;
                    retry_phase = false;
                    fase_concluida = false;
                    todas_fases_concluidas = false;
                    break;
                }
                else if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    phase_running = false;
                    retry_phase = false;
                    fase_concluida = false;
                    break;
                }
                else if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_SPACE && !victory_flag) {
                    for (int i = 0; i < total; i++) {
                        pieces[i].placed = 1;
                        pieces[i].x = pieces[i].target_x;
                        pieces[i].y = pieces[i].target_y;
                    }
                    victory_flag = 1;
                }
                else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && !victory_flag) {
                    float mx = ev.mouse.x, my = ev.mouse.y;
                    for (int i = total - 1; i >= 0; i--) {
                        if (pieces[i].placed) continue;
                        if (mx >= pieces[i].x && mx <= pieces[i].x + piece_w_s &&
                            my >= pieces[i].y && my <= pieces[i].y + piece_h_s) {
                            Piece tmp = pieces[i];
                            for (int j = i; j < total - 1; j++) pieces[j] = pieces[j + 1];
                            pieces[total - 1] = tmp;
                            grabbed = total - 1;
                            grab_off_x = mx - pieces[grabbed].x;
                            grab_off_y = my - pieces[grabbed].y;
                            break;
                        }
                    }
                }
                else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES && grabbed >= 0 && !victory_flag) {
                    pieces[grabbed].x = ev.mouse.x - grab_off_x;
                    pieces[grabbed].y = ev.mouse.y - grab_off_y;
                }
                else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP && grabbed >= 0 && !victory_flag) {
                    float dx = pieces[grabbed].x - pieces[grabbed].target_x;
                    float dy = pieces[grabbed].y - pieces[grabbed].target_y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist <= 36.0f) {
                        pieces[grabbed].placed = 1;
                        pieces[grabbed].x = pieces[grabbed].target_x;
                        pieces[grabbed].y = pieces[grabbed].target_y;
                    }
                    grabbed = -1;
                }

                else if (ev.type == ALLEGRO_EVENT_TIMER) {
                    if (!victory_flag) {
                        elapsed_time += 1.0f / FPS;
                        if (elapsed_time >= TIME_LIMIT) victory_flag = 2;
                    }

                    al_draw_scaled_bitmap(fundo_puzzle, 0, 0,
                        al_get_bitmap_width(fundo_puzzle), al_get_bitmap_height(fundo_puzzle),
                        0, 0, SCREEN_W, SCREEN_H, 0);

                    for (int i = 0; i < total; i++)
                        al_draw_rectangle(pieces[i].target_x - 1, pieces[i].target_y - 1,
                            pieces[i].target_x + piece_w_s + 1, pieces[i].target_y + piece_h_s + 1,
                            al_map_rgb(90, 90, 90), 1);

                    for (int i = 0; i < total; i++)
                        if (pieces[i].placed)
                            al_draw_scaled_bitmap(pieces[i].bmp, 0, 0, pieces[i].w, pieces[i].h,
                                pieces[i].target_x, pieces[i].target_y, piece_w_s, piece_h_s, 0);

                    for (int i = 0; i < total; i++)
                        if (!pieces[i].placed && i != grabbed)
                            al_draw_scaled_bitmap(pieces[i].bmp, 0, 0, pieces[i].w, pieces[i].h,
                                pieces[i].x, pieces[i].y, piece_w_s, piece_h_s, 0);

                    if (grabbed >= 0)
                        al_draw_scaled_bitmap(pieces[grabbed].bmp, 0, 0, pieces[grabbed].w, pieces[grabbed].h,
                            pieces[grabbed].x, pieces[grabbed].y, piece_w_s, piece_h_s, 0);

                    float ref_x = (float)(grid_x + grid_total_w + 20);
                    float ref_y = (float)grid_y;
                    al_draw_scaled_bitmap(source, 0, 0, src_w, src_h, (int)ref_x, (int)ref_y, ref_width, ref_height, 0);

                    char timer_text[64];
                    int remaining = (int)ceilf(TIME_LIMIT - elapsed_time);
                    if (remaining < 0) remaining = 0;
                    snprintf(timer_text, sizeof(timer_text), "Tempo: %02d:%02d", remaining / 60, remaining % 60);
                    al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W - 300, 8, 0, timer_text);

                    if (!victory_flag) {
                        int all_placed = 1;
                        for (int i = 0; i < total; i++)
                            if (!pieces[i].placed) { all_placed = 0; break; }
                        if (all_placed) victory_flag = 1;
                    }

                    if (victory_flag == 1) {
                        al_draw_filled_rectangle(SCREEN_W / 2 - 300, SCREEN_H / 2 - 40, SCREEN_W / 2 + 300, SCREEN_H / 2 + 40, al_map_rgba(0, 0, 0, 180));
                        al_draw_text(font, al_map_rgb(0, 255, 0), SCREEN_W / 2, SCREEN_H / 2 - 6, ALLEGRO_ALIGN_CENTRE, "Fase concluída!");
                        al_flip_display();
                        al_rest(2.0);
                        phase_running = false;
                        fase_concluida = true;
                        if (phase_idx == 2) todas_fases_concluidas = true;
                    }
                    else if (victory_flag == 2) {
                        al_draw_filled_rectangle(SCREEN_W / 2 - 360, SCREEN_H / 2 - 60, SCREEN_W / 2 + 360, SCREEN_H / 2 + 60, al_map_rgba(0, 0, 0, 180));
                        al_draw_text(font, al_map_rgb(255, 50, 50), SCREEN_W / 2, SCREEN_H / 2 - 10, ALLEGRO_ALIGN_CENTRE, "Tempo esgotado! Reiniciando fase...");
                        al_flip_display();
                        al_rest(2.0);
                        retry_phase = true;
                        phase_running = false;
                    }

                    al_flip_display();
                }
            }

            for (int i = 0; i < total; i++) if (pieces[i].bmp) al_destroy_bitmap(pieces[i].bmp);
            free(pieces);
            al_destroy_bitmap(source);

            if (!phase_running && !retry_phase && !fase_concluida) {
                break;
            }
        }

        if (phase_idx < 2 && fase_concluida) {
            al_clear_to_color(al_map_rgb(12, 12, 12));
            char tx[128]; snprintf(tx, sizeof(tx), "Preparando fase %d...", phase_idx + 2);
            al_draw_text(font, al_map_rgb(255, 215, 0), SCREEN_W / 2, SCREEN_H / 2, ALLEGRO_ALIGN_CENTRE, tx);
            al_flip_display();
            al_rest(1.5);
        }
    }

    if (todas_fases_concluidas) {
        al_clear_to_color(al_map_rgb(12, 12, 12));
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 - 30,
            ALLEGRO_ALIGN_CENTRE, "Parabéns! Você concluiu todas as fases!");
        al_draw_text(font, al_map_rgb(255, 215, 0), SCREEN_W / 2, SCREEN_H / 2 + 30,
            ALLEGRO_ALIGN_CENTRE, "🔑 Você ganhou uma CHAVE! 🔑");
        al_flip_display();
        al_rest(3.0);

        chave_quebra_cabeca = true;
    }

    al_destroy_bitmap(instrucoes_img);
    al_destroy_bitmap(fundo_puzzle);
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return todas_fases_concluidas ? 1 : 0;
}

// ----------------------- Estruturas e funções do Jogo da Memória -----------------------
#define LINHAS_MEMORIA 4
#define COLUNAS_MEMORIA 4
#define LARGURA_CARTA 100
#define ALTURA_CARTA 100
#define ESPACO_MEMORIA 10
#define AREA_LARGURA_MEMORIA (COLUNAS_MEMORIA * (LARGURA_CARTA + ESPACO_MEMORIA) + ESPACO_MEMORIA)
#define AREA_ALTURA_MEMORIA (LINHAS_MEMORIA * (ALTURA_CARTA + ESPACO_MEMORIA) + ESPACO_MEMORIA)
#define TEMPO_ESPERA_MEMORIA 1.0

typedef struct {
    int id;
    int virada;
    int encontrada;
    ALLEGRO_BITMAP* imagem;
} Carta;

void embaralhar_memoria(int* v, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = v[i];
        v[i] = v[j];
        v[j] = temp;
    }
}

void mostrar_quadrinho_oito(ALLEGRO_DISPLAY* display) {
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    bool sair = false;
    bool redesenhar = true;

    if (!quadrinho_oito) {
        quadrinho_oito = al_load_bitmap("assets/8.jpg");
        if (!quadrinho_oito) {
            printf("ERRO: Não foi possível carregar assets/8.jpg\n");
        }
    }

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE ||
                evento.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                sair = true;
            }
        }
        else if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            sair = true;
        }

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            if (quadrinho_oito) {
                al_draw_scaled_bitmap(quadrinho_oito, 0, 0,
                    al_get_bitmap_width(quadrinho_oito),
                    al_get_bitmap_height(quadrinho_oito),
                    0, 0, SCREEN_W, SCREEN_H, 0);
            }
            else {
                al_clear_to_color(al_map_rgb(0, 0, 0));
                ALLEGRO_FONT* font_temp = al_create_builtin_font();
                if (font_temp) {
                    al_draw_text(font_temp, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2,
                        ALLEGRO_ALIGN_CENTER, "Imagem 8.jpg não encontrada");
                    al_destroy_font(font_temp);
                }
            }

            ALLEGRO_FONT* font_temp = al_create_builtin_font();
            if (font_temp) {
                al_draw_text(font_temp, al_map_rgb(255, 255, 255),
                    SCREEN_W / 2, SCREEN_H - 50,
                    ALLEGRO_ALIGN_CENTER, "Pressione ENTER, ESPAÇO ou clique para voltar ao lobby");
                al_destroy_font(font_temp);
            }

            al_flip_display();
        }
    }

    al_destroy_event_queue(queue);
}

// ----------------------- Função do Jogo da Memória -----------------------
int executar_jogo_memoria(ALLEGRO_DISPLAY* display_main) {
    srand((unsigned)time(NULL));

    ALLEGRO_DISPLAY* display = display_main;
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    ALLEGRO_BITMAP* imagem_sete = al_load_bitmap("assets/7.jpg");
    if (!imagem_sete) {
        fprintf(stderr, "Erro ao carregar imagem 7.jpg\n");
        al_show_native_message_box(display, "Erro", "Arquivo não encontrado",
            "Verifique se assets/7.jpg existe", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return 0;
    }

    ALLEGRO_BITMAP* background_memoria = al_load_bitmap("assets/backgroundjdm.jpg");
    if (!background_memoria) {
        fprintf(stderr, "Erro ao carregar imagem de fundo do jogo da memória\n");
        al_show_native_message_box(display, "Erro", "Arquivo não encontrado",
            "Verifique se assets/backgroundjdm.jpg existe", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return 0;
    }

    const char* nomes_imagens[8] = {
        "assets/img1.jpg",
        "assets/img2.jpg",
        "assets/img3.jpg",
        "assets/jc.jpg",
        "assets/pintinho.jpg",
        "assets/img5.jpg",
        "assets/img6.jpg",
        "assets/img7.jpg"
    };

    for (int i = 0; i < 8; i++) {
        ALLEGRO_BITMAP* test = al_load_bitmap(nomes_imagens[i]);
        if (!test) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg),
                "Arquivo não encontrado: %s\n\nVerifique se o arquivo existe na pasta assets.",
                nomes_imagens[i]);
            al_show_native_message_box(display, "Erro", "Asset não encontrado", err_msg, NULL, 0);
            al_destroy_bitmap(background_memoria);
            al_destroy_font(font);
            al_destroy_timer(timer);
            al_destroy_event_queue(queue);
            return 0;
        }
        al_destroy_bitmap(test);
    }

    int total_cartas = LINHAS_MEMORIA * COLUNAS_MEMORIA;
    int ids[LINHAS_MEMORIA * COLUNAS_MEMORIA];

    for (int i = 0; i < total_cartas / 2; i++) {
        ids[2 * i] = i % 8;
        ids[2 * i + 1] = i % 8;
    }
    embaralhar_memoria(ids, total_cartas);

    Carta cartas[LINHAS_MEMORIA][COLUNAS_MEMORIA];
    int idx = 0;
    for (int i = 0; i < LINHAS_MEMORIA; i++) {
        for (int j = 0; j < COLUNAS_MEMORIA; j++) {
            cartas[i][j].id = ids[idx++];
            cartas[i][j].virada = 0;
            cartas[i][j].encontrada = 0;

            int img_index = cartas[i][j].id;
            cartas[i][j].imagem = al_load_bitmap(nomes_imagens[img_index]);
            if (!cartas[i][j].imagem) {
                fprintf(stderr, "Erro ao carregar imagem %s\n", nomes_imagens[img_index]);
                al_destroy_bitmap(background_memoria);
                al_destroy_font(font);
                al_destroy_timer(timer);
                al_destroy_event_queue(queue);
                return 0;
            }
        }
    }

    int abertas = 0;
    int esperando = 0;
    Carta* primeira = NULL;
    Carta* segunda = NULL;
    double tempo_virada = 0;
    int pares_encontrados = 0;
    int rodando = 1;

    bool tela_inicial_ativa = true;
    bool instrucoes_ativa = false;
    bool jogo_ativo = false;

    al_start_timer(timer);

    int area_x = (SCREEN_W - AREA_LARGURA_MEMORIA) / 2;
    int area_y = (SCREEN_H - AREA_ALTURA_MEMORIA) / 2;

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = 0;
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            if (jogo_ativo) {
                rodando = 0;
            }
            else if (instrucoes_ativa) {
                instrucoes_ativa = false;
                tela_inicial_ativa = true;
            }
            else if (tela_inicial_ativa) {
                rodando = 0;
            }
        }
        else if (tela_inicial_ativa) {
            if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                    ev.keyboard.keycode == ALLEGRO_KEY_E ||
                    ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                    tela_inicial_ativa = false;
                    instrucoes_ativa = true;
                }
            }
            else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                tela_inicial_ativa = false;
                instrucoes_ativa = true;
            }
        }
        else if (instrucoes_ativa) {
            if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                    ev.keyboard.keycode == ALLEGRO_KEY_E ||
                    ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                    instrucoes_ativa = false;
                    jogo_ativo = true;
                }
            }
            else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                instrucoes_ativa = false;
                jogo_ativo = true;
            }
        }
        else if (jogo_ativo && ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            if (!esperando) {
                int mx = ev.mouse.x;
                int my = ev.mouse.y;

                for (int i = 0; i < LINHAS_MEMORIA; i++) {
                    for (int j = 0; j < COLUNAS_MEMORIA; j++) {
                        int x = area_x + ESPACO_MEMORIA + j * (LARGURA_CARTA + ESPACO_MEMORIA);
                        int y = area_y + ESPACO_MEMORIA + i * (ALTURA_CARTA + ESPACO_MEMORIA);
                        if (mx > x && mx < x + LARGURA_CARTA && my > y && my < y + ALTURA_CARTA) {
                            if (!cartas[i][j].virada && !cartas[i][j].encontrada) {
                                cartas[i][j].virada = 1;
                                abertas++;
                                if (abertas == 1)
                                    primeira = &cartas[i][j];
                                else if (abertas == 2) {
                                    segunda = &cartas[i][j];
                                    tempo_virada = al_get_time();
                                    esperando = 1;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (esperando && al_get_time() - tempo_virada > TEMPO_ESPERA_MEMORIA) {
            if (primeira->id == segunda->id) {
                primeira->encontrada = 1;
                segunda->encontrada = 1;
                pares_encontrados++;
            }
            else {
                primeira->virada = 0;
                segunda->virada = 0;
            }
            abertas = 0;
            primeira = NULL;
            segunda = NULL;
            esperando = 0;
        }

        if (tela_inicial_ativa) {
            al_draw_scaled_bitmap(imagem_sete, 0, 0,
                al_get_bitmap_width(imagem_sete), al_get_bitmap_height(imagem_sete),
                0, 0, SCREEN_W, SCREEN_H, 0);

            al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H - 50,
                ALLEGRO_ALIGN_CENTER, "Pressione ENTER, E, ESPAÇO ou clique para continuar");
        }
        else if (instrucoes_ativa) {
            al_draw_scaled_bitmap(background_memoria, 0, 0,
                al_get_bitmap_width(background_memoria), al_get_bitmap_height(background_memoria),
                0, 0, SCREEN_W, SCREEN_H, 0);

            al_draw_filled_rectangle(SCREEN_W / 2 - 300, SCREEN_H / 2 - 150,
                SCREEN_W / 2 + 300, SCREEN_H / 2 + 150,
                al_map_rgba(0, 0, 0, 200));

            al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W / 2, SCREEN_H / 2 - 120,
                ALLEGRO_ALIGN_CENTER, "JOGO DA MEMÓRIA");

            al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 - 60,
                ALLEGRO_ALIGN_CENTER, "Encontre todos os pares de cartas!");
            al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 - 30,
                ALLEGRO_ALIGN_CENTER, "Clique nas cartas para virá-las.");
            al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2,
                ALLEGRO_ALIGN_CENTER, "Encontre 8 pares para ganhar a chave!");

            al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 80,
                ALLEGRO_ALIGN_CENTER, "Pressione ENTER, E, ESPAÇO ou clique para começar");
        }
        else if (jogo_ativo) {
            al_draw_scaled_bitmap(background_memoria, 0, 0,
                al_get_bitmap_width(background_memoria), al_get_bitmap_height(background_memoria),
                0, 0, SCREEN_W, SCREEN_H, 0);

            al_draw_filled_rounded_rectangle(area_x - 15, area_y - 15,
                area_x + AREA_LARGURA_MEMORIA + 15, area_y + AREA_ALTURA_MEMORIA + 15,
                20, 20, al_map_rgb(30, 30, 30));

            al_draw_rounded_rectangle(area_x - 15, area_y - 15,
                area_x + AREA_LARGURA_MEMORIA + 15, area_y + AREA_ALTURA_MEMORIA + 15,
                20, 20, al_map_rgb(255, 255, 255), 3);

            for (int i = 0; i < LINHAS_MEMORIA; i++) {
                for (int j = 0; j < COLUNAS_MEMORIA; j++) {
                    int x = area_x + ESPACO_MEMORIA + j * (LARGURA_CARTA + ESPACO_MEMORIA);
                    int y = area_y + ESPACO_MEMORIA + i * (ALTURA_CARTA + ESPACO_MEMORIA);

                    if (cartas[i][j].virada || cartas[i][j].encontrada) {
                        al_draw_scaled_bitmap(cartas[i][j].imagem,
                            0, 0, al_get_bitmap_width(cartas[i][j].imagem), al_get_bitmap_height(cartas[i][j].imagem),
                            x, y, LARGURA_CARTA, ALTURA_CARTA, 0);
                    }
                    else {
                        al_draw_filled_rectangle(x, y, x + LARGURA_CARTA, y + ALTURA_CARTA, al_map_rgb(80, 80, 150));
                    }

                    al_draw_rectangle(x, y, x + LARGURA_CARTA, y + ALTURA_CARTA, al_map_rgb(255, 255, 255), 2);
                }
            }

            al_draw_textf(font, al_map_rgb(255, 255, 255),
                SCREEN_W / 2, SCREEN_H - 60, ALLEGRO_ALIGN_CENTER,
                "Pares encontrados: %d / %d", pares_encontrados, total_cartas / 2);

            if (pares_encontrados == total_cartas / 2) {
                al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W / 2, SCREEN_H - 30,
                    ALLEGRO_ALIGN_CENTER, "🔑 Você ganhou uma CHAVE! 🔑");
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H - 10,
                    ALLEGRO_ALIGN_CENTER, "Voltando ao lobby...");

                static double tempo_vitoria = 0;
                if (tempo_vitoria == 0) {
                    tempo_vitoria = al_get_time();
                    chave_memoria = true;
                }
                if (al_get_time() - tempo_vitoria > 3.0) {
                    rodando = 0;
                }
            }
            else {
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 20,
                    ALLEGRO_ALIGN_CENTER, "Jogo da Memória - Pressione ESC para voltar");
            }
        }

        al_flip_display();
    }

    if (pares_encontrados == total_cartas / 2) {
        mostrar_quadrinho_oito(display);
    }

    if (imagem_sete) al_destroy_bitmap(imagem_sete);
    al_destroy_bitmap(background_memoria);
    for (int i = 0; i < LINHAS_MEMORIA; i++) {
        for (int j = 0; j < COLUNAS_MEMORIA; j++) {
            if (cartas[i][j].imagem) {
                al_destroy_bitmap(cartas[i][j].imagem);
            }
        }
    }

    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 1;
}

// ----------------------- Estruturas e funções do Jogo Colorir -----------------------
#define NUM_PLATAFORMAS_COLORIR 16
#define MAX_ESTADOS_COLORIR 6
#define MAX_SPRITES_PULO_COLORIR 8
#define MAX_SPRITES_CORRIDA_COLORIR 8
#define MAX_SPRITES_MORTE_COLORIR 5
#define MAX_FANTASMAS_COLORIR 3

typedef struct {
    float x, y;
    float largura, altura;
} PlataformaColorir;

typedef struct {
    float x, y;
    float vel_x, vel_y;
    int largura, altura;
    bool no_chao;
    int direcao;
    bool abaixado;
} JogadorColorir;

typedef struct {
    float x, y;
    float raio;
    float velocidade;
    float largura;
    float altura;
} InimigoColorir;

// ----------------------- Funções auxiliares do Colorir -----------------------
void atualizar_fisica_jogador_colorir(JogadorColorir* jogador, PlataformaColorir* plataformas) {
    if (!jogador->no_chao)
        jogador->vel_y += GRAVIDADE;

    jogador->y += jogador->vel_y;

    float chao = ALTURA_TELA - 100;
    if (jogador->y >= chao - jogador->altura) {
        jogador->y = chao - jogador->altura;
        jogador->vel_y = 0;
        jogador->no_chao = true;
    }
    else {
        jogador->no_chao = false;
    }

    for (int i = 0; i < NUM_PLATAFORMAS_COLORIR; i++) {
        PlataformaColorir p = plataformas[i];
        bool colide_horizontalmente = (jogador->x + jogador->largura > p.x) &&
            (jogador->x < p.x + p.largura);
        bool tocando_por_cima = (jogador->y + jogador->altura >= p.y) &&
            (jogador->y + jogador->altura <= p.y + p.altura + jogador->vel_y) &&
            (jogador->vel_y >= 0);
        if (jogador->abaixado) continue;
        if (colide_horizontalmente && tocando_por_cima) {
            jogador->y = p.y - jogador->altura;
            jogador->vel_y = 0;
            jogador->no_chao = true;
        }
    }
}

void processar_entrada_movimento_colorir(JogadorColorir* jogador, bool* jogador_travado, bool* jogador_morrendo,
    int* estado_atual, bool* balde_pego, int* num_fantasmas,
    InimigoColorir* fantasmas) {
    ALLEGRO_KEYBOARD_STATE kb;
    al_get_keyboard_state(&kb);

    if (*jogador_travado) {
        if (al_key_down(&kb, ALLEGRO_KEY_R)) {
            *jogador_travado = false;
            *jogador_morrendo = false;
            jogador->x = 200;
            jogador->y = ALTURA_TELA - 200;
            *estado_atual = 0;
            *balde_pego = false;
            *num_fantasmas = 1;
            for (int i = 0; i < MAX_FANTASMAS_COLORIR; i++) {
                fantasmas[i].x = LARGURA_TELA / 2;
                fantasmas[i].y = ALTURA_TELA / 2;
                fantasmas[i].velocidade = 2.5f;
                fantasmas[i].largura = 150;
                fantasmas[i].altura = 150;
                fantasmas[i].raio = 75;
            }
        }
        return;
    }

    jogador->vel_x = (al_key_down(&kb, ALLEGRO_KEY_D) - al_key_down(&kb, ALLEGRO_KEY_A)) * VELOCIDADE_MOVIMENTO;
    if (jogador->vel_x != 0)
        jogador->direcao = (jogador->vel_x > 0) ? 1 : -1;

    if (al_key_down(&kb, ALLEGRO_KEY_W) && jogador->no_chao) {
        jogador->vel_y = FORCA_PULO;
        jogador->no_chao = false;
    }

    jogador->abaixado = al_key_down(&kb, ALLEGRO_KEY_S);

    if (al_key_down(&kb, ALLEGRO_KEY_SPACE)) {
        float img_x = (LARGURA_TELA - 180) / 2;
        float img_y = ALTURA_TELA - 100 - 180;
        if (jogador->x + jogador->largura > img_x && jogador->x < img_x + 180 &&
            jogador->y + jogador->altura > img_y && jogador->y < img_y + 180) {
            if (*balde_pego && *estado_atual < 5) {
                (*estado_atual)++;
                *balde_pego = false;
                if (*estado_atual == 1) *num_fantasmas = 2;
                if (*estado_atual == 2) *num_fantasmas = 3;
                if (*estado_atual >= 5) *num_fantasmas = 0;
            }
        }
    }
}

void atualizar_inimigo_colorir(JogadorColorir* jogador, InimigoColorir* fantasmas, int num_fantasmas,
    bool* jogador_morrendo, bool jogador_travado, int estado_atual) {
    if (jogador_travado || estado_atual == 5 || num_fantasmas == 0) return;

    for (int i = 0; i < num_fantasmas; i++) {
        float dx = (jogador->x + jogador->largura / 2) - fantasmas[i].x;
        float dy = (jogador->y + jogador->altura / 2) - fantasmas[i].y;
        float distancia = sqrtf(dx * dx + dy * dy);

        if (distancia > 0) {
            fantasmas[i].x += (dx / distancia) * fantasmas[i].velocidade;
            fantasmas[i].y += (dy / distancia) * fantasmas[i].velocidade;
        }

        float dx_c = (jogador->x + jogador->largura / 2) - fantasmas[i].x;
        float dy_c = (jogador->y + jogador->altura / 2) - fantasmas[i].y;
        float distancia_c = sqrtf(dx_c * dx_c + dy_c * dy_c);

        if (distancia_c < fantasmas[i].raio + jogador->largura / 2 && !(*jogador_morrendo)) {
            *jogador_morrendo = true;
        }
    }
}


void mostrar_quadrinho_dez(ALLEGRO_DISPLAY* display) {
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    bool sair = false;
    bool redesenhar = true;

    if (!quadrinho_dez) {
        quadrinho_dez = al_load_bitmap("assets/10.jpg");
        if (!quadrinho_dez) {
            printf("ERRO: Não foi possível carregar assets/10.jpg\n");
        }
    }

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                evento.keyboard.keycode == ALLEGRO_KEY_E ||
                evento.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                sair = true;
            }
        }
        else if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            sair = true;
        }

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            if (quadrinho_dez) {
                al_draw_scaled_bitmap(quadrinho_dez, 0, 0,
                    al_get_bitmap_width(quadrinho_dez),
                    al_get_bitmap_height(quadrinho_dez),
                    0, 0, SCREEN_W, SCREEN_H, 0);
            }
            else {
                al_clear_to_color(al_map_rgb(0, 0, 0));
                ALLEGRO_FONT* font_temp = al_create_builtin_font();
                if (font_temp) {
                    al_draw_text(font_temp, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2,
                        ALLEGRO_ALIGN_CENTER, "Imagem 10.jpg não encontrada");
                    al_destroy_font(font_temp);
                }
            }

            ALLEGRO_FONT* font_temp = al_create_builtin_font();
            if (font_temp) {
                al_draw_text(font_temp, al_map_rgb(255, 255, 255),
                    SCREEN_W / 2, SCREEN_H - 50,
                    ALLEGRO_ALIGN_CENTER, "Pressione ENTER, E, ESPAÇO ou clique para continuar para o Boss Final");
                al_destroy_font(font_temp);
            }

            al_flip_display();
        }
    }

    al_destroy_event_queue(queue);
}

// ----------------------- Função Principal do Jogo Colorir -----------------------
int executar_jogo_colorir(ALLEGRO_DISPLAY* display_main) {
    ALLEGRO_DISPLAY* display = display_main;
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    JogadorColorir jogador_colorir;
    PlataformaColorir plataformas_colorir[NUM_PLATAFORMAS_COLORIR];
    InimigoColorir fantasmas_colorir[MAX_FANTASMAS_COLORIR];

    bool jogador_travado = false;
    bool jogador_morrendo = false;
    int num_fantasmas_colorir = 1;
    int estado_atual = 0;
    bool balde_pego = false;
    bool cena_instrucoes = true;

    ALLEGRO_BITMAP* imgs_estados[MAX_ESTADOS_COLORIR];
    ALLEGRO_BITMAP* img_balde = NULL;
    ALLEGRO_BITMAP* img_instrucoes = NULL;
    ALLEGRO_BITMAP* img_fantasma = NULL;
    ALLEGRO_BITMAP* sprite_parado = NULL;

    ALLEGRO_BITMAP* sprites_pulo[MAX_SPRITES_PULO_COLORIR];
    ALLEGRO_BITMAP* sprites_corrida[MAX_SPRITES_CORRIDA_COLORIR];
    ALLEGRO_BITMAP* sprites_morte[MAX_SPRITES_MORTE_COLORIR];

    int plataforma_proximo_balde[MAX_ESTADOS_COLORIR] = { 0, 1, 2, 3, 4, -1 };

    jogador_colorir.x = 200;
    jogador_colorir.y = ALTURA_TELA - 200;
    jogador_colorir.vel_x = 0;
    jogador_colorir.vel_y = 0;
    jogador_colorir.largura = 50;
    jogador_colorir.altura = 60;
    jogador_colorir.no_chao = true;
    jogador_colorir.direcao = 1;
    jogador_colorir.abaixado = false;

    plataformas_colorir[0] = (PlataformaColorir){ 70, 100, 100, 15 };
    plataformas_colorir[1] = (PlataformaColorir){ 1100, 100, 100, 15 };
    plataformas_colorir[2] = (PlataformaColorir){ 50, 460, 100, 15 };
    plataformas_colorir[3] = (PlataformaColorir){ 700, 250, 100, 15 };
    plataformas_colorir[4] = (PlataformaColorir){ 1200, 500, 100, 15 };
    plataformas_colorir[5] = (PlataformaColorir){ 360, 480, 100, 15 };
    plataformas_colorir[6] = (PlataformaColorir){ 260, 390, 100, 15 };
    plataformas_colorir[7] = (PlataformaColorir){ 180, 180, 100, 15 };
    plataformas_colorir[8] = (PlataformaColorir){ 900, 140, 100, 15 };
    plataformas_colorir[9] = (PlataformaColorir){ 490, 300, 100, 15 };
    plataformas_colorir[10] = (PlataformaColorir){ 970, 430, 100, 15 };
    plataformas_colorir[11] = (PlataformaColorir){ 920, 260, 100, 15 };
    plataformas_colorir[12] = (PlataformaColorir){ 800, 400, 100, 15 };
    plataformas_colorir[13] = (PlataformaColorir){ 1100, 300, 100, 15 };
    plataformas_colorir[14] = (PlataformaColorir){ 50, 320, 100, 15 };
    plataformas_colorir[15] = (PlataformaColorir){ 400, 200, 100, 15 };

    for (int i = 0; i < MAX_FANTASMAS_COLORIR; i++) {
        fantasmas_colorir[i].x = LARGURA_TELA / 2;
        fantasmas_colorir[i].y = ALTURA_TELA / 2;
        fantasmas_colorir[i].velocidade = 2.5f;
        fantasmas_colorir[i].largura = 150;
        fantasmas_colorir[i].altura = 150;
        fantasmas_colorir[i].raio = 75;
    }

    const char* arquivos_estados[MAX_ESTADOS_COLORIR] = {
        "assets/estado_zero.png",
        "assets/estado_um.png",
        "assets/estado_dois.png",
        "assets/estado_tres.png",
        "assets/estado_quatro.png",
        "assets/estado_cinco.png"
    };

    for (int i = 0; i < MAX_ESTADOS_COLORIR; i++) {
        imgs_estados[i] = al_load_bitmap(arquivos_estados[i]);
        if (!imgs_estados[i]) {
            printf("Erro: não foi possível carregar %s\n", arquivos_estados[i]);
            al_destroy_timer(timer);
            al_destroy_event_queue(queue);
            return 0;
        }
    }

    img_balde = al_load_bitmap("assets/balde.png");
    if (!img_balde) {
        printf("Erro: não foi possível carregar assets/balde.png\n");
        al_destroy_timer(timer);
        al_destroy_event_queue(queue);
        return 0;
    }

    img_instrucoes = al_load_bitmap("assets/instrucoes.jpg");
    if (!img_instrucoes) {
        printf("Erro: não foi possível carregar assets/instrucoes.jpg\n");
        al_destroy_timer(timer);
        al_destroy_event_queue(queue);
        return 0;
    }

    img_fantasma = al_load_bitmap("assets/fantasma.png");
    if (!img_fantasma) {
        printf("Erro: não foi possível carregar assets/fantasma.png\n");
        al_destroy_timer(timer);
        al_destroy_event_queue(queue);
        return 0;
    }

    sprite_parado = al_load_bitmap("assets/parada.png");
    if (!sprite_parado) {
        printf("Erro: não foi possível carregar assets/parada.png\n");
        al_destroy_timer(timer);
        al_destroy_event_queue(queue);
        return 0;
    }

    const char* arquivos_pulo[MAX_SPRITES_PULO_COLORIR] = {
        "assets/jump1.png","assets/jump2.png","assets/jump3.png",
        "assets/jump4.png","assets/jump5.png","assets/jump6.png",
        "assets/jump7.png","assets/jump8.png"
    };
    for (int i = 0; i < MAX_SPRITES_PULO_COLORIR; i++) {
        sprites_pulo[i] = al_load_bitmap(arquivos_pulo[i]);
        if (!sprites_pulo[i]) {
            printf("Erro: não foi possível carregar %s\n", arquivos_pulo[i]);
            al_destroy_timer(timer);
            al_destroy_event_queue(queue);
            return 0;
        }
    }

    const char* arquivos_corrida[MAX_SPRITES_CORRIDA_COLORIR] = {
        "assets/run1.png","assets/run2.png","assets/run3.png",
        "assets/run4.png","assets/run5.png","assets/run6.png",
        "assets/run7.png","assets/run8.png"
    };
    for (int i = 0; i < MAX_SPRITES_CORRIDA_COLORIR; i++) {
        sprites_corrida[i] = al_load_bitmap(arquivos_corrida[i]);
        if (!sprites_corrida[i]) {
            printf("Erro: não foi possível carregar %s\n", arquivos_corrida[i]);
            al_destroy_timer(timer);
            al_destroy_event_queue(queue);
            return 0;
        }
    }

    const char* arquivos_morte[MAX_SPRITES_MORTE_COLORIR] = {
        "assets/die1.png","assets/die2.png","assets/die3.png",
        "assets/die4.png","assets/die5.png"
    };
    for (int i = 0; i < MAX_SPRITES_MORTE_COLORIR; i++) {
        sprites_morte[i] = al_load_bitmap(arquivos_morte[i]);
        if (!sprites_morte[i]) {
            printf("Erro: não foi possível carregar %s\n", arquivos_morte[i]);
            al_destroy_timer(timer);
            al_destroy_event_queue(queue);
            return 0;
        }
    }

    al_start_timer(timer);

    bool sair = false;
    bool redesenhar = true;
    int resultado = 0;

    int frame_pulo = 0, frame_corrida = 0, frame_morte = 0;
    float tempo_frame_pulo = 0.0f, tempo_frame_corrida = 0.0f, tempo_frame_morte = 0.0f;
    float duracao_frame_pulo = 0.08f, duracao_frame_corrida = 0.08f, duracao_frame_morte = 0.1f;

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN && evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN && evento.keyboard.keycode == ALLEGRO_KEY_ENTER && cena_instrucoes) {
            cena_instrucoes = false;
        }
        else if (evento.type == ALLEGRO_EVENT_TIMER && !cena_instrucoes) {
            processar_entrada_movimento_colorir(&jogador_colorir, &jogador_travado, &jogador_morrendo,
                &estado_atual, &balde_pego, &num_fantasmas_colorir,
                fantasmas_colorir);

            if (!jogador_travado && !jogador_morrendo) {
                jogador_colorir.x += jogador_colorir.vel_x;
                if (jogador_colorir.x < 0) jogador_colorir.x = 0;
                if (jogador_colorir.x > LARGURA_TELA - jogador_colorir.largura)
                    jogador_colorir.x = LARGURA_TELA - jogador_colorir.largura;
                atualizar_fisica_jogador_colorir(&jogador_colorir, plataformas_colorir);
            }

            atualizar_inimigo_colorir(&jogador_colorir, fantasmas_colorir, num_fantasmas_colorir,
                &jogador_morrendo, jogador_travado, estado_atual);

            float delta = 1.0 / FPS;

            if (!jogador_colorir.no_chao && !jogador_morrendo) {
                tempo_frame_pulo += delta;
                if (tempo_frame_pulo >= duracao_frame_pulo) {
                    tempo_frame_pulo = 0;
                    frame_pulo++;
                    if (frame_pulo >= MAX_SPRITES_PULO_COLORIR)
                        frame_pulo = MAX_SPRITES_PULO_COLORIR - 1;
                }
            }
            else {
                frame_pulo = 0;
                tempo_frame_pulo = 0;
            }

            if (jogador_colorir.no_chao && jogador_colorir.vel_x != 0 && !jogador_morrendo) {
                tempo_frame_corrida += delta;
                if (tempo_frame_corrida >= duracao_frame_corrida) {
                    tempo_frame_corrida = 0;
                    frame_corrida++;
                    if (frame_corrida >= MAX_SPRITES_CORRIDA_COLORIR)
                        frame_corrida = 0;
                }
            }
            else {
                frame_corrida = 0;
                tempo_frame_corrida = 0;
            }

            if (jogador_morrendo) {
                tempo_frame_morte += delta;
                if (tempo_frame_morte >= duracao_frame_morte) {
                    tempo_frame_morte = 0;
                    frame_morte++;
                    if (frame_morte >= MAX_SPRITES_MORTE_COLORIR) {
                        frame_morte = MAX_SPRITES_MORTE_COLORIR - 1;
                        jogador_travado = true;
                    }
                }
            }

            if (!balde_pego && estado_atual < 5) {
                int plat_index = plataforma_proximo_balde[estado_atual];
                float balde_x = plataformas_colorir[plat_index].x + (plataformas_colorir[plat_index].largura - 50) / 2;
                float balde_y = plataformas_colorir[plat_index].y - 50;

                float dx = (jogador_colorir.x + jogador_colorir.largura / 2) - (balde_x + 25);
                float dy = (jogador_colorir.y + jogador_colorir.altura / 2) - (balde_y + 25);
                float distancia = sqrtf(dx * dx + dy * dy);
                if (distancia < (jogador_colorir.largura + 50) / 2) {
                    balde_pego = true;
                }
            }

            if (estado_atual == 5) {
                resultado = 1;
                sair = true;
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

                float img_x = (LARGURA_TELA - 180) / 2;
                float img_y = ALTURA_TELA - 100 - 180;
                al_draw_scaled_bitmap(
                    imgs_estados[estado_atual],
                    0, 0,
                    al_get_bitmap_width(imgs_estados[estado_atual]),
                    al_get_bitmap_height(imgs_estados[estado_atual]),
                    img_x, img_y,
                    180, 180,
                    0
                );

                al_draw_filled_rectangle(0, ALTURA_TELA - 100, LARGURA_TELA, ALTURA_TELA, al_map_rgb(101, 67, 33));

                for (int i = 0; i < NUM_PLATAFORMAS_COLORIR; i++) {
                    al_draw_filled_rectangle(
                        plataformas_colorir[i].x, plataformas_colorir[i].y,
                        plataformas_colorir[i].x + plataformas_colorir[i].largura, plataformas_colorir[i].y + plataformas_colorir[i].altura,
                        al_map_rgb(70, 70, 70)
                    );
                }

                if (!balde_pego && estado_atual < 5) {
                    int plat_index = plataforma_proximo_balde[estado_atual];
                    float balde_x = plataformas_colorir[plat_index].x + (plataformas_colorir[plat_index].largura - 50) / 2;
                    float balde_y = plataformas_colorir[plat_index].y - 50;
                    al_draw_scaled_bitmap(
                        img_balde,
                        0, 0,
                        al_get_bitmap_width(img_balde),
                        al_get_bitmap_height(img_balde),
                        balde_x, balde_y,
                        50, 50,
                        0
                    );
                }

                for (int i = 0; i < num_fantasmas_colorir; i++) {
                    al_draw_scaled_bitmap(
                        img_fantasma,
                        0, 0,
                        al_get_bitmap_width(img_fantasma),
                        al_get_bitmap_height(img_fantasma),
                        fantasmas_colorir[i].x - fantasmas_colorir[i].largura / 2,
                        fantasmas_colorir[i].y - fantasmas_colorir[i].altura / 2,
                        fantasmas_colorir[i].largura,
                        fantasmas_colorir[i].altura,
                        0
                    );
                }

                ALLEGRO_BITMAP* sprite_jogador = NULL;
                if (jogador_morrendo) {
                    sprite_jogador = sprites_morte[frame_morte];
                }
                else if (!jogador_colorir.no_chao) {
                    sprite_jogador = sprites_pulo[frame_pulo];
                }
                else if (jogador_colorir.vel_x != 0) {
                    sprite_jogador = sprites_corrida[frame_corrida];
                }
                else {
                    sprite_jogador = sprite_parado;
                }

                if (sprite_jogador) {
                    if (jogador_colorir.direcao == 1) {
                        al_draw_scaled_bitmap(sprite_jogador, 0, 0,
                            al_get_bitmap_width(sprite_jogador),
                            al_get_bitmap_height(sprite_jogador),
                            jogador_colorir.x, jogador_colorir.y,
                            jogador_colorir.largura, jogador_colorir.altura, 0);
                    }
                    else {
                        al_draw_scaled_bitmap(sprite_jogador, 0, 0,
                            al_get_bitmap_width(sprite_jogador),
                            al_get_bitmap_height(sprite_jogador),
                            jogador_colorir.x + jogador_colorir.largura, jogador_colorir.y,
                            -jogador_colorir.largura, jogador_colorir.altura, 0);
                    }
                }

                al_flip_display();
            }
        }
    }

    if (resultado == 1) {
        mostrar_quadrinho_dez(display_main);
    }

    for (int i = 0; i < MAX_ESTADOS_COLORIR; i++) {
        if (imgs_estados[i]) al_destroy_bitmap(imgs_estados[i]);
    }
    if (img_balde) al_destroy_bitmap(img_balde);
    if (img_instrucoes) al_destroy_bitmap(img_instrucoes);
    if (img_fantasma) al_destroy_bitmap(img_fantasma);
    if (sprite_parado) al_destroy_bitmap(sprite_parado);

    for (int i = 0; i < MAX_SPRITES_PULO_COLORIR; i++) {
        if (sprites_pulo[i]) al_destroy_bitmap(sprites_pulo[i]);
    }
    for (int i = 0; i < MAX_SPRITES_CORRIDA_COLORIR; i++) {
        if (sprites_corrida[i]) al_destroy_bitmap(sprites_corrida[i]);
    }
    for (int i = 0; i < MAX_SPRITES_MORTE_COLORIR; i++) {
        if (sprites_morte[i]) al_destroy_bitmap(sprites_morte[i]);
    }

    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return resultado;
}

// ----------------------- Estruturas e funções do Boss Final -----------------------
#define LARGURA_TELA_BOSS SCREEN_W
#define ALTURA_TELA_BOSS SCREEN_H
#define FPS_BOSS 60
#define GRAVIDADE_BOSS 0.4f
#define FORCA_PULO_BOSS -12.5f
#define VELOCIDADE_MOVIMENTO_BOSS 5.0f
#define MAX_PLATAFORMAS_BOSS 8
#define TAMANHO_BLOCO_PLATAFORMA_BOSS 32
#define MAX_TIROS_JOGADOR_BOSS 50
#define MAX_TIROS_BOSS_FINAL 50
#define MAX_CAPANGAS_BOSS 4
#define MAX_TIROS_CAPANGA_BOSS 20
#define VIDA_CAPANGA_BOSS 50
#define VELOCIDADE_CAPANGA_BOSS 1.1f
#define MAX_SPRITES_PULO_BOSS 8
#define MAX_SPRITES_CORRIDA_BOSS 8
#define MAX_SPRITES_MORTE_BOSS 5
#define VELOCIDADE_TIRO_BOSS_FINAL 3.0f
#define VELOCIDADE_TIRO_CAPANGA 2.5f

typedef struct {
    float x, y, vel_x, vel_y;
    bool ativo;
} TiroBoss;

typedef struct {
    float x, y;
    int largura, altura;
} PlataformaBoss;

typedef struct {
    float x, y, vel_x, vel_y;
    int largura, altura;
    bool no_chao, descendo_plataforma;
    int direcao, vida;
} JogadorBoss;

typedef struct {
    float x, y;
    int vida, vida_maxima;
    bool ativo, segunda_fase;
    int tempo_tiro, largura, altura;
} BossFinal;

typedef struct {
    float x, y, vel_x, vel_y;
    int largura, altura, vida, tempo_tiro;
    bool ativo, no_chao;
    int direcao;
} CapangaBoss;

typedef struct {
    int vida_jogador;
    int boss_vida_atual;
    int boss_vida_max;
    bool boss_ativo;
    bool jogador_morto;
} DadosHUD;

void atualizar_fisica_entidade_boss(float* x, float* y, float* vel_y, int largura, int altura,
    bool* no_chao, bool ignora_plataformas, PlataformaBoss* plataformas) {
    if (!*no_chao) *vel_y += GRAVIDADE_BOSS;
    *y += *vel_y;

    float chao_y = ALTURA_TELA_BOSS - 50;
    bool colidiu_chao = (*y >= chao_y - altura);

    if (colidiu_chao) {
        *y = chao_y - altura;
        *vel_y = 0;
        *no_chao = true;
        return;
    }

    if (*vel_y >= 0 && !ignora_plataformas) {
        for (int i = 0; i < MAX_PLATAFORMAS_BOSS; i++) {
            bool colide_x = (*x + largura > plataformas[i].x) && (*x < plataformas[i].x + plataformas[i].largura);
            bool colide_y = (*y + altura >= plataformas[i].y) && (*y + altura <= plataformas[i].y + 10);

            if (colide_x && colide_y) {
                *y = plataformas[i].y - altura;
                *vel_y = 0;
                *no_chao = true;
                return;
            }
        }
    }

    *no_chao = false;
}

void criar_tiro_boss(TiroBoss tiros[], int max_tiros, float x, float y, float vel_x, float vel_y) {
    for (int i = 0; i < max_tiros; i++) {
        if (!tiros[i].ativo) {
            tiros[i].x = x;
            tiros[i].y = y;
            tiros[i].vel_x = vel_x;
            tiros[i].vel_y = vel_y;
            tiros[i].ativo = true;
            break;
        }
    }
}

void criar_tiros_direcionados_boss(TiroBoss tiros[], int max_tiros, float start_x, float start_y,
    float target_x, float target_y, float speed) {
    float dx = target_x - start_x;
    float dy = target_y - start_y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist > 0) {
        criar_tiro_boss(tiros, max_tiros, start_x, start_y, (dx / dist) * speed, (dy / dist) * speed);
    }
}

bool colidiu_boss(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void desenhar_hud(ALLEGRO_FONT* font, DadosHUD dados) {
    char buffer[100];
    ALLEGRO_COLOR cor_branca = al_map_rgb(255, 255, 255);

    snprintf(buffer, sizeof(buffer), "VIDA: %d", dados.vida_jogador);
    al_draw_text(font, cor_branca, 20, 20, 0, buffer);

    if (dados.boss_ativo) {
        snprintf(buffer, sizeof(buffer), "BOSS: %d/%d", dados.boss_vida_atual, dados.boss_vida_max);
        al_draw_text(font, cor_branca, LARGURA_TELA_BOSS - 150, 20,
            ALLEGRO_ALIGN_RIGHT, buffer);
    }
    else {
        al_draw_text(font, al_map_rgb(255, 255, 0), LARGURA_TELA_BOSS / 2,
            ALTURA_TELA_BOSS / 2, ALLEGRO_ALIGN_CENTER, "VILAO DERROTADO!");
    }

    if (dados.jogador_morto) {
        al_draw_filled_rectangle(0, 0, LARGURA_TELA_BOSS, ALTURA_TELA_BOSS,
            al_map_rgba(0, 0, 0, 180));

        al_draw_text(font, al_map_rgb(255, 50, 50), LARGURA_TELA_BOSS / 2,
            ALTURA_TELA_BOSS / 2 - 40, ALLEGRO_ALIGN_CENTER,
            "VOCE MORREU, TE FALTA ODIO");

        al_draw_text(font, cor_branca, LARGURA_TELA_BOSS / 2,
            ALTURA_TELA_BOSS / 2 + 20, ALLEGRO_ALIGN_CENTER,
            "TECLE R PARA REINICIAR");
    }
}

int executar_boss_final(ALLEGRO_DISPLAY* display_main) {
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS_BOSS);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_FONT* font = al_create_builtin_font();

    ALLEGRO_SAMPLE* musica_fundo = al_load_sample("assets/paranoid.wav");
    ALLEGRO_SAMPLE_INSTANCE* instancia_musica = NULL;

    if (musica_fundo) {
        instancia_musica = al_create_sample_instance(musica_fundo);
        al_set_sample_instance_playmode(instancia_musica, ALLEGRO_PLAYMODE_LOOP);
        al_attach_sample_instance_to_mixer(instancia_musica, al_get_default_mixer());
        al_set_sample_instance_gain(instancia_musica, 1.0);
        al_play_sample_instance(instancia_musica);
    }

    ALLEGRO_BITMAP* background_image = al_load_bitmap("assets/inferno.jpg");
    ALLEGRO_BITMAP* platform_block_image = al_load_bitmap("assets/floor.png");
    ALLEGRO_BITMAP* boss_image = al_load_bitmap("assets/hitler.png");
    ALLEGRO_BITMAP* capanga_image = al_load_bitmap("assets/capanga.png");
    ALLEGRO_BITMAP* sprite_parado = al_load_bitmap("assets/parada.png");
    ALLEGRO_BITMAP* sprites_pulo[MAX_SPRITES_PULO_BOSS];
    ALLEGRO_BITMAP* sprites_corrida[MAX_SPRITES_CORRIDA_BOSS];
    ALLEGRO_BITMAP* sprites_morte[MAX_SPRITES_MORTE_BOSS];

    const char* arquivos_pulo[] = {
        "assets/jump1.png", "assets/jump2.png", "assets/jump3.png", "assets/jump4.png",
        "assets/jump5.png", "assets/jump6.png", "assets/jump7.png", "assets/jump8.png"
    };
    const char* arquivos_corrida[] = {
        "assets/run1.png", "assets/run2.png", "assets/run3.png", "assets/run4.png",
        "assets/run5.png", "assets/run6.png", "assets/run7.png", "assets/run8.png"
    };
    const char* arquivos_morte[] = {
        "assets/die1.png", "assets/die2.png", "assets/die3.png", "assets/die4.png", "assets/die5.png"
    };

    for (int i = 0; i < MAX_SPRITES_PULO_BOSS; i++)
        sprites_pulo[i] = al_load_bitmap(arquivos_pulo[i]);
    for (int i = 0; i < MAX_SPRITES_CORRIDA_BOSS; i++)
        sprites_corrida[i] = al_load_bitmap(arquivos_corrida[i]);
    for (int i = 0; i < MAX_SPRITES_MORTE_BOSS; i++)
        sprites_morte[i] = al_load_bitmap(arquivos_morte[i]);

    JogadorBoss jogador = {
        .x = 100,
        .y = ALTURA_TELA_BOSS - 125,
        .vel_x = 0,
        .vel_y = 0,
        .largura = 40,
        .altura = 60,
        .no_chao = true,
        .descendo_plataforma = false,
        .direcao = 1,
        .vida = 100
    };

    BossFinal boss = {
        .x = LARGURA_TELA_BOSS - 300,
        .y = ALTURA_TELA_BOSS - 550,
        .vida = 500,
        .vida_maxima = 500,
        .ativo = true,
        .segunda_fase = false,
        .tempo_tiro = 0,
        .largura = 300,
        .altura = 500
    };

    PlataformaBoss plataformas[MAX_PLATAFORMAS_BOSS] = {
        {50, 270, 350, 10}, {550, 545, 400, 10}, {100, 120, 400, 10}, {150, 420, 400, 10},
        {50, 545, 400, 10}, {500, 270, 400, 10}, {600, 120, 400, 10}, {650, 420, 400, 10}
    };

    TiroBoss tiros_jogador[MAX_TIROS_JOGADOR_BOSS];
    TiroBoss tiros_boss[MAX_TIROS_BOSS_FINAL];
    TiroBoss tiros_capangas[MAX_TIROS_CAPANGA_BOSS];

    for (int i = 0; i < MAX_TIROS_JOGADOR_BOSS; i++)
        tiros_jogador[i].ativo = false;
    for (int i = 0; i < MAX_TIROS_BOSS_FINAL; i++)
        tiros_boss[i].ativo = false;
    for (int i = 0; i < MAX_TIROS_CAPANGA_BOSS; i++)
        tiros_capangas[i].ativo = false;

    CapangaBoss capangas[MAX_CAPANGAS_BOSS];
    for (int i = 0; i < MAX_CAPANGAS_BOSS; i++)
        capangas[i].ativo = false;

    int frame_pulo = 0, frame_corrida = 0, frame_morte = 0;
    float tempo_frame_pulo = 0, tempo_frame_corrida = 0, tempo_frame_morte = 0;
    bool jogador_morrendo = false;
    bool jogador_morto = false;

    int tempo_spawn_capanga = 0;
    bool jogo_rodando = true;
    bool sair = false;
    bool redesenhar = true;
    bool reiniciar = false;

    al_register_event_source(queue, al_get_display_event_source(display_main));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_start_timer(timer);

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN && evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN && evento.keyboard.keycode == ALLEGRO_KEY_R && jogador_morto) {
            reiniciar = true;
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_TIMER && jogo_rodando) {
            float delta = 1.0f / FPS_BOSS;

            jogador.x += jogador.vel_x;
            if (jogador.x < 0) jogador.x = 0;
            if (jogador.x > LARGURA_TELA_BOSS - jogador.largura)
                jogador.x = LARGURA_TELA_BOSS - jogador.largura;

            atualizar_fisica_entidade_boss(&jogador.x, &jogador.y, &jogador.vel_y,
                jogador.largura, jogador.altura, &jogador.no_chao,
                jogador.descendo_plataforma, plataformas);

            if (!jogador.no_chao && !jogador_morrendo) {
                tempo_frame_pulo += delta;
                if (tempo_frame_pulo >= 0.08f) {
                    tempo_frame_pulo = 0;
                    if (frame_pulo < MAX_SPRITES_PULO_BOSS - 1) frame_pulo++;
                }
            }
            else {
                frame_pulo = 0;
                tempo_frame_pulo = 0;
            }

            if (jogador.no_chao && jogador.vel_x != 0 && !jogador_morrendo) {
                tempo_frame_corrida += delta;
                if (tempo_frame_corrida >= 0.08f) {
                    tempo_frame_corrida = 0;
                    frame_corrida = (frame_corrida + 1) % MAX_SPRITES_CORRIDA_BOSS;
                }
            }
            else {
                frame_corrida = 0;
                tempo_frame_corrida = 0;
            }

            if (jogador_morrendo) {
                tempo_frame_morte += delta;
                if (tempo_frame_morte >= 0.1f) {
                    tempo_frame_morte = 0;
                    if (frame_morte < MAX_SPRITES_MORTE_BOSS - 1) {
                        frame_morte++;
                    }
                    else {
                        jogador_morto = true;
                        jogo_rodando = false;
                    }
                }
            }

            if (boss.ativo) {
                if (boss.vida <= boss.vida_maxima / 2)
                    boss.segunda_fase = true;

                boss.tempo_tiro++;
                int intervalo = boss.segunda_fase ? 20 : 30;

                if (boss.tempo_tiro >= intervalo) {
                    float boss_centro_x = boss.x + boss.largura / 2;
                    float boss_centro_y = boss.y + boss.altura / 2;
                    float jogador_centro_x = jogador.x + jogador.largura / 2;
                    float jogador_centro_y = jogador.y + jogador.largura / 2;

                    criar_tiros_direcionados_boss(tiros_boss, MAX_TIROS_BOSS_FINAL,
                        boss_centro_x, boss_centro_y,
                        jogador_centro_x, jogador_centro_y, VELOCIDADE_TIRO_BOSS_FINAL);

                    boss.tempo_tiro = 0;
                }
            }

            if (boss.ativo && boss.vida <= boss.vida_maxima * 0.75) {
                tempo_spawn_capanga++;
                if (tempo_spawn_capanga > FPS_BOSS * 3.5) {
                    for (int i = 0; i < MAX_CAPANGAS_BOSS; i++) {
                        if (!capangas[i].ativo) {
                            capangas[i].x = (rand() % 2 == 0) ? 25.0f : LARGURA_TELA_BOSS - 50.0f;
                            capangas[i].y = 25;
                            capangas[i].largura = 50;
                            capangas[i].altura = 70;
                            capangas[i].vida = VIDA_CAPANGA_BOSS;
                            capangas[i].ativo = true;
                            capangas[i].tempo_tiro = 0;
                            capangas[i].vel_x = 0;
                            capangas[i].vel_y = 0;
                            capangas[i].no_chao = false;
                            capangas[i].direcao = 1;
                            break;
                        }
                    }
                    tempo_spawn_capanga = 0;
                }
            }

            for (int i = 0; i < MAX_CAPANGAS_BOSS; i++) {
                if (!capangas[i].ativo) continue;

                capangas[i].vel_x = (jogador.x > capangas[i].x) ? VELOCIDADE_CAPANGA_BOSS : -VELOCIDADE_CAPANGA_BOSS;

                if (capangas[i].no_chao && (rand() % (FPS_BOSS * 4) == 0)) {
                    capangas[i].vel_y = -12.0f;
                    capangas[i].no_chao = false;
                }

                capangas[i].x += capangas[i].vel_x;

                if (capangas[i].vel_x > 0) {
                    capangas[i].direcao = 1;
                }
                else if (capangas[i].vel_x < 0) {
                    capangas[i].direcao = -1;
                }

                atualizar_fisica_entidade_boss(&capangas[i].x, &capangas[i].y, &capangas[i].vel_y,
                    capangas[i].largura, capangas[i].altura,
                    &capangas[i].no_chao, false, plataformas);

                capangas[i].tempo_tiro++;
                if (capangas[i].tempo_tiro > FPS_BOSS * 0.8) {
                    criar_tiros_direcionados_boss(tiros_capangas, MAX_TIROS_CAPANGA_BOSS,
                        capangas[i].x + capangas[i].largura / 2,
                        capangas[i].y + capangas[i].altura / 2,
                        jogador.x + jogador.largura / 2,
                        jogador.y + jogador.altura / 2, VELOCIDADE_TIRO_CAPANGA);
                    capangas[i].tempo_tiro = 0;
                }
            }

            for (int i = 0; i < MAX_TIROS_JOGADOR_BOSS; i++) {
                if (!tiros_jogador[i].ativo) continue;
                tiros_jogador[i].x += tiros_jogador[i].vel_x;
                tiros_jogador[i].y += tiros_jogador[i].vel_y;
                if (tiros_jogador[i].x < 0 || tiros_jogador[i].x > LARGURA_TELA_BOSS ||
                    tiros_jogador[i].y < 0 || tiros_jogador[i].y > ALTURA_TELA_BOSS) {
                    tiros_jogador[i].ativo = false;
                }
            }

            for (int i = 0; i < MAX_TIROS_BOSS_FINAL; i++) {
                if (!tiros_boss[i].ativo) continue;
                tiros_boss[i].x += tiros_boss[i].vel_x;
                tiros_boss[i].y += tiros_boss[i].vel_y;
                if (tiros_boss[i].x < 0 || tiros_boss[i].x > LARGURA_TELA_BOSS ||
                    tiros_boss[i].y < 0 || tiros_boss[i].y > ALTURA_TELA_BOSS) {
                    tiros_boss[i].ativo = false;
                }
            }

            for (int i = 0; i < MAX_TIROS_CAPANGA_BOSS; i++) {
                if (!tiros_capangas[i].ativo) continue;
                tiros_capangas[i].x += tiros_capangas[i].vel_x;
                tiros_capangas[i].y += tiros_capangas[i].vel_y;
                if (tiros_capangas[i].x < 0 || tiros_capangas[i].x > LARGURA_TELA_BOSS ||
                    tiros_capangas[i].y < 0 || tiros_capangas[i].y > ALTURA_TELA_BOSS) {
                    tiros_capangas[i].ativo = false;
                }
            }

            for (int i = 0; i < MAX_TIROS_JOGADOR_BOSS; i++) {
                if (!tiros_jogador[i].ativo) continue;

                if (boss.ativo && colidiu_boss(tiros_jogador[i].x, tiros_jogador[i].y, 4, 2,
                    boss.x, boss.y, boss.largura, boss.altura)) {
                    tiros_jogador[i].ativo = false;
                    boss.vida -= 2;
                    if (boss.vida <= 0) {
                        boss.ativo = false;
                    }
                }

                for (int j = 0; j < MAX_CAPANGAS_BOSS; j++) {
                    if (capangas[j].ativo && colidiu_boss(tiros_jogador[i].x, tiros_jogador[i].y, 4, 2,
                        capangas[j].x, capangas[j].y,
                        capangas[j].largura, capangas[j].altura)) {
                        tiros_jogador[i].ativo = false;
                        capangas[j].vida -= 20;
                        if (capangas[j].vida <= 0) {
                            capangas[j].ativo = false;
                        }
                        break;
                    }
                }
            }

            for (int i = 0; i < MAX_TIROS_BOSS_FINAL; i++) {
                if (tiros_boss[i].ativo && colidiu_boss(tiros_boss[i].x, tiros_boss[i].y, 6, 6,
                    jogador.x, jogador.y, jogador.largura, jogador.altura)) {
                    tiros_boss[i].ativo = false;
                    jogador.vida -= 15;
                    if (jogador.vida <= 0 && !jogador_morrendo) {
                        jogador_morrendo = true;
                        frame_morte = 0;
                        tempo_frame_morte = 0;
                        if (instancia_musica) al_stop_sample_instance(instancia_musica);
                    }
                }
            }

            for (int i = 0; i < MAX_TIROS_CAPANGA_BOSS; i++) {
                if (tiros_capangas[i].ativo && colidiu_boss(tiros_capangas[i].x, tiros_capangas[i].y, 5, 5,
                    jogador.x, jogador.y, jogador.largura, jogador.altura)) {
                    tiros_capangas[i].ativo = false;
                    jogador.vida -= 5;
                    if (jogador.vida <= 0 && !jogador_morrendo) {
                        jogador_morrendo = true;
                        frame_morte = 0;
                        tempo_frame_morte = 0;
                        if (instancia_musica) al_stop_sample_instance(instancia_musica);
                    }
                }
            }

            if (boss.ativo && colidiu_boss(jogador.x, jogador.y, jogador.largura, jogador.altura,
                boss.x, boss.y, boss.largura, boss.altura)) {
                if (!jogador_morrendo) {
                    jogador.vida = 0;
                    jogador_morrendo = true;
                    frame_morte = 0;
                    tempo_frame_morte = 0;
                    if (instancia_musica) al_stop_sample_instance(instancia_musica);
                }
            }

            if (!boss.ativo) jogo_rodando = false;

            redesenhar = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (evento.keyboard.keycode == ALLEGRO_KEY_SPACE && !jogador_morrendo && jogo_rodando) {
                criar_tiro_boss(tiros_jogador, MAX_TIROS_JOGADOR_BOSS,
                    jogador.x + (jogador.direcao == 1 ? jogador.largura : 0),
                    jogador.y + jogador.altura / 2,
                    jogador.direcao * 6.0f, 0);
            }
        }

        if (!jogador_morrendo && jogo_rodando) {
            ALLEGRO_KEYBOARD_STATE kb;
            al_get_keyboard_state(&kb);

            jogador.vel_x = (al_key_down(&kb, ALLEGRO_KEY_D) - al_key_down(&kb, ALLEGRO_KEY_A)) * VELOCIDADE_MOVIMENTO_BOSS;
            if (jogador.vel_x != 0)
                jogador.direcao = (jogador.vel_x > 0) ? 1 : -1;

            if (al_key_down(&kb, ALLEGRO_KEY_W) && jogador.no_chao) {
                jogador.vel_y = FORCA_PULO_BOSS;
                jogador.no_chao = false;
            }

            jogador.descendo_plataforma = al_key_down(&kb, ALLEGRO_KEY_S);
        }

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            if (background_image) {
                al_draw_scaled_bitmap(background_image, 0, 0,
                    al_get_bitmap_width(background_image),
                    al_get_bitmap_height(background_image),
                    0, 0, LARGURA_TELA_BOSS, ALTURA_TELA_BOSS, 0);
            }
            else {
                al_clear_to_color(al_map_rgb(20, 20, 40));
            }

            float chao_y = ALTURA_TELA_BOSS - 50;
            if (platform_block_image) {
                int num_blocos = (LARGURA_TELA_BOSS / TAMANHO_BLOCO_PLATAFORMA_BOSS) + 1;
                for (int i = 0; i < num_blocos; i++) {
                    al_draw_scaled_bitmap(platform_block_image, 0, 0,
                        al_get_bitmap_width(platform_block_image),
                        al_get_bitmap_height(platform_block_image),
                        i * TAMANHO_BLOCO_PLATAFORMA_BOSS, chao_y,
                        TAMANHO_BLOCO_PLATAFORMA_BOSS, 50, 0);
                }
            }
            else {
                al_draw_filled_rectangle(0, chao_y, LARGURA_TELA_BOSS, ALTURA_TELA_BOSS, al_map_rgb(80, 60, 30));
            }

            for (int i = 0; i < MAX_PLATAFORMAS_BOSS; i++) {
                if (platform_block_image) {
                    int num_blocos = (plataformas[i].largura / TAMANHO_BLOCO_PLATAFORMA_BOSS) + 1;
                    for (int j = 0; j < num_blocos; j++) {
                        al_draw_scaled_bitmap(platform_block_image, 0, 0,
                            al_get_bitmap_width(platform_block_image),
                            al_get_bitmap_height(platform_block_image),
                            plataformas[i].x + (j * TAMANHO_BLOCO_PLATAFORMA_BOSS),
                            plataformas[i].y,
                            TAMANHO_BLOCO_PLATAFORMA_BOSS, plataformas[i].altura, 0);
                    }
                }
                else {
                    al_draw_filled_rectangle(plataformas[i].x, plataformas[i].y,
                        plataformas[i].x + plataformas[i].largura,
                        plataformas[i].y + plataformas[i].altura,
                        al_map_rgb(100, 100, 100));
                }
            }

            ALLEGRO_BITMAP* sprite_atual = NULL;

            if (jogador_morrendo && sprites_morte[0]) {
                sprite_atual = sprites_morte[frame_morte];
            }
            else if (!jogador.no_chao && sprites_pulo[0]) {
                sprite_atual = sprites_pulo[frame_pulo];
            }
            else if (jogador.vel_x != 0 && sprites_corrida[0]) {
                sprite_atual = sprites_corrida[frame_corrida];
            }
            else if (sprite_parado) {
                sprite_atual = sprite_parado;
            }

            if (sprite_atual) {
                if (jogador.direcao == 1) {
                    al_draw_scaled_bitmap(sprite_atual, 0, 0,
                        al_get_bitmap_width(sprite_atual),
                        al_get_bitmap_height(sprite_atual),
                        jogador.x, jogador.y, jogador.largura, jogador.altura, 0);
                }
                else {
                    al_draw_scaled_bitmap(sprite_atual, 0, 0,
                        al_get_bitmap_width(sprite_atual),
                        al_get_bitmap_height(sprite_atual),
                        jogador.x + jogador.largura, jogador.y,
                        -jogador.largura, jogador.altura, 0);
                }
            }

            if (boss.ativo) {
                if (boss_image) {
                    al_draw_scaled_bitmap(boss_image, 0, 0,
                        al_get_bitmap_width(boss_image),
                        al_get_bitmap_height(boss_image),
                        boss.x, boss.y, boss.largura, boss.altura, 0);
                }

                float vida_percentual = (float)boss.vida / boss.vida_maxima;
                al_draw_filled_rectangle(boss.x, boss.y - 15, boss.x + boss.largura,
                    boss.y - 5, al_map_rgb(100, 0, 0));
                al_draw_filled_rectangle(boss.x, boss.y - 15, boss.x + boss.largura * vida_percentual,
                    boss.y - 5, al_map_rgb(255, 0, 0));
            }

            for (int i = 0; i < MAX_CAPANGAS_BOSS; i++) {
                if (capangas[i].ativo) {
                    if (capanga_image) {
                        int flags = 0;

                        if (capangas[i].direcao == 1) {
                            flags = ALLEGRO_FLIP_HORIZONTAL;
                        }

                        al_draw_scaled_bitmap(capanga_image, 0, 0,
                            al_get_bitmap_width(capanga_image),
                            al_get_bitmap_height(capanga_image),
                            capangas[i].x, capangas[i].y,
                            capangas[i].largura, capangas[i].altura, flags);
                    }
                }
            }

            for (int i = 0; i < MAX_TIROS_JOGADOR_BOSS; i++) {
                if (tiros_jogador[i].ativo) {
                    al_draw_filled_circle(tiros_jogador[i].x, tiros_jogador[i].y, 2,
                        al_map_rgb(255, 255, 0));
                }
            }

            for (int i = 0; i < MAX_TIROS_BOSS_FINAL; i++) {
                if (tiros_boss[i].ativo) {
                    al_draw_filled_circle(tiros_boss[i].x, tiros_boss[i].y, 3,
                        al_map_rgb(255, 100, 100));
                }
            }

            for (int i = 0; i < MAX_TIROS_CAPANGA_BOSS; i++) {
                if (tiros_capangas[i].ativo) {
                    al_draw_filled_circle(tiros_capangas[i].x, tiros_capangas[i].y, 3,
                        al_map_rgb(150, 255, 150));
                }
            }

            DadosHUD dados_atuais = {
                .vida_jogador = jogador.vida,
                .boss_vida_atual = boss.vida,
                .boss_vida_max = boss.vida_maxima,
                .boss_ativo = boss.ativo,
                .jogador_morto = jogador_morto
            };
            desenhar_hud(font, dados_atuais);

            al_flip_display();
        }
    }

    if (instancia_musica) {
        al_stop_sample_instance(instancia_musica);
        al_destroy_sample_instance(instancia_musica);
    }
    if (musica_fundo) {
        al_destroy_sample(musica_fundo);
    }
    if (background_image) al_destroy_bitmap(background_image);
    if (platform_block_image) al_destroy_bitmap(platform_block_image);
    if (sprite_parado) al_destroy_bitmap(sprite_parado);
    if (boss_image) al_destroy_bitmap(boss_image);
    if (capanga_image) al_destroy_bitmap(capanga_image);

    for (int i = 0; i < MAX_SPRITES_PULO_BOSS; i++) {
        if (sprites_pulo[i]) al_destroy_bitmap(sprites_pulo[i]);
    }
    for (int i = 0; i < MAX_SPRITES_CORRIDA_BOSS; i++) {
        if (sprites_corrida[i]) al_destroy_bitmap(sprites_corrida[i]);
    }
    for (int i = 0; i < MAX_SPRITES_MORTE_BOSS; i++) {
        if (sprites_morte[i]) al_destroy_bitmap(sprites_morte[i]);
    }

    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    if (reiniciar) {
        return executar_boss_final(display_main);
    }

    return (boss.ativo == false) ? 1 : 0;
}

// ----------------------- Tela de Vitória -----------------------
void mostrar_tela_vitoria(ALLEGRO_DISPLAY* display) {
    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());

    bool sair = false;
    double tempo_inicio = al_get_time();

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE ||
            (evento.type == ALLEGRO_EVENT_KEY_DOWN && al_get_time() - tempo_inicio > 2.0)) {
            sair = true;
        }

        al_clear_to_color(al_map_rgb(0, 0, 0));

        double tempo = al_get_time();
        int alpha = (int)((sin(tempo * 3) + 1) * 127) + 128;

        al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W / 2, SCREEN_H / 2 - 80,
            ALLEGRO_ALIGN_CENTER, "🎉 PARABÉNS! 🎉");
        al_draw_text(font, al_map_rgba(255, 255, 255, alpha), SCREEN_W / 2, SCREEN_H / 2 - 20,
            ALLEGRO_ALIGN_CENTER, "Você concluiu o jogo!");
        al_draw_text(font, al_map_rgb(100, 255, 100), SCREEN_W / 2, SCREEN_H / 2 + 20,
            ALLEGRO_ALIGN_CENTER, "Obrigado por jogar!");

        if (al_get_time() - tempo_inicio > 2.0) {
            al_draw_text(font, al_map_rgb(200, 200, 200), SCREEN_W / 2, SCREEN_H / 2 + 80,
                ALLEGRO_ALIGN_CENTER, "Pressione qualquer tecla para sair");
        }

        al_flip_display();
    }

    al_destroy_font(font);
    al_destroy_event_queue(queue);
}

// ----------------------- Função para resetar o lobby -----------------------
void resetar_lobby() {
    jogador.x = 200;
    jogador.y = ALTURA_TELA - 250;
    jogador.vel_x = 0;
    jogador.vel_y = 0;
    jogador.no_chao = true;
    jogador.direcao = 1;

    jogador.sprite_corrida = 0;
    jogador.contador_frames_corrida = 0;
    jogador.sprite_pulo = 0;
    jogador.contador_frames_pulo = 0;

    for (int i = 0; i < numero_portas; i++) {
        portas[i].jogador_proximo = false;
    }
}

void mostrar_quadrinho_nove(ALLEGRO_DISPLAY* display) {
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    bool sair = false;
    bool redesenhar = true;

    if (!quadrinho_nove) {
        quadrinho_nove = al_load_bitmap("assets/9.jpg");
        if (!quadrinho_nove) {
            printf("ERRO: Não foi possível carregar assets/9.jpg\n");
        }
    }

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                evento.keyboard.keycode == ALLEGRO_KEY_E ||
                evento.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                sair = true;
            }
        }
        else if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            sair = true;
        }

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            if (quadrinho_nove) {
                al_draw_scaled_bitmap(quadrinho_nove, 0, 0,
                    al_get_bitmap_width(quadrinho_nove),
                    al_get_bitmap_height(quadrinho_nove),
                    0, 0, SCREEN_W, SCREEN_H, 0);
            }
            else {
                al_clear_to_color(al_map_rgb(0, 0, 0));
                ALLEGRO_FONT* font_temp = al_create_builtin_font();
                if (font_temp) {
                    al_draw_text(font_temp, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2,
                        ALLEGRO_ALIGN_CENTER, "Imagem 9.jpg não encontrada");
                    al_destroy_font(font_temp);
                }
            }

            ALLEGRO_FONT* font_temp = al_create_builtin_font();
            if (font_temp) {
                al_draw_text(font_temp, al_map_rgb(255, 255, 255),
                    SCREEN_W / 2, SCREEN_H - 50,
                    ALLEGRO_ALIGN_CENTER, "Pressione ENTER, E, ESPAÇO ou clique para continuar");
                al_destroy_font(font_temp);
            }

            al_flip_display();
        }
    }

    al_destroy_event_queue(queue);
}

// ----------------------- Funções do Lobby -----------------------
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

void verificar_proximidade_portas() {
    float area_interacao = 100.0f;

    for (int i = 0; i < numero_portas; i++) {
        float centro_jogador_x = jogador.x + jogador.largura / 2;
        float centro_jogador_y = jogador.y + jogador.altura / 2;
        float centro_porta_x = portas[i].x + portas[i].largura / 2;
        float centro_porta_y = portas[i].y + portas[i].altura / 2;

        float distancia_x = centro_jogador_x - centro_porta_x;
        float distancia_y = centro_jogador_y - centro_porta_y;
        float distancia = sqrt(distancia_x * distancia_x + distancia_y * distancia_y);

        portas[i].jogador_proximo = (distancia <= area_interacao);
    }
}

void mostrar_quadrinho_cinco(ALLEGRO_DISPLAY* display) {
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    bool sair = false;
    bool redesenhar = true;

    if (!quadrinho_cinco) {
        quadrinho_cinco = al_load_bitmap("assets/5.jpg");
        if (!quadrinho_cinco) {
            printf("ERRO: Não foi possível carregar assets/5.jpg\n");
        }
    }

    while (!sair) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE ||
                evento.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                sair = true;
            }
        }
        else if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            sair = true;
        }

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            if (quadrinho_cinco) {
                al_draw_scaled_bitmap(quadrinho_cinco, 0, 0,
                    al_get_bitmap_width(quadrinho_cinco),
                    al_get_bitmap_height(quadrinho_cinco),
                    0, 0, SCREEN_W, SCREEN_H, 0);
            }
            else {
                al_clear_to_color(al_map_rgb(0, 0, 0));
                ALLEGRO_FONT* font_temp = al_create_builtin_font();
                if (font_temp) {
                    al_draw_text(font_temp, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2,
                        ALLEGRO_ALIGN_CENTER, "Imagem 5.jpg não encontrada");
                    al_destroy_font(font_temp);
                }
            }

            ALLEGRO_FONT* font_temp = al_create_builtin_font();
            if (font_temp) {
                al_draw_text(font_temp, al_map_rgb(255, 255, 255),
                    SCREEN_W / 2, SCREEN_H - 50,
                    ALLEGRO_ALIGN_CENTER, "Pressione ENTER, ESPAÇO ou clique para continuar");
                al_destroy_font(font_temp);
            }

            al_flip_display();
        }
    }

    al_destroy_event_queue(queue);
}

void processar_interacao_portas(ALLEGRO_DISPLAY* display) {
    ALLEGRO_KEYBOARD_STATE kb;
    al_get_keyboard_state(&kb);

    static bool e_pressionada_anteriormente = false;
    bool e_pressionada = al_key_down(&kb, ALLEGRO_KEY_E);

    if (e_pressionada && !e_pressionada_anteriormente) {
        for (int i = 0; i < numero_portas; i++) {
            if (portas[i].jogador_proximo) {
                printf("Interagindo com a porta: %s\n", portas[i].nome);

                if (i == 0) {
                    printf("Iniciando quebra-cabeça...\n");

                    mostrar_quadrinho_cinco(display);

                    int resultado = executar_quebra_cabeca(display);

                    if (resultado == 1) {
                        mostrar_quadrinho_seis(display);
                    }

                    printf("Retornando ao lobby...\n");
                    resetar_lobby();
                }
                else if (i == 1) {
                    if (!chave_quebra_cabeca || !chave_memoria) {
                        printf("Porta trancada! Faltam chaves.\n");
                        ALLEGRO_FONT* font_temp = al_create_builtin_font();
                        if (font_temp) {
                            al_clear_to_color(al_map_rgb(0, 0, 0));
                            al_draw_text(font_temp, al_map_rgb(255, 0, 0), SCREEN_W / 2, SCREEN_H / 2 - 30,
                                ALLEGRO_ALIGN_CENTER, "PORTA TRANCADA!");
                            al_draw_text(font_temp, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 10,
                                ALLEGRO_ALIGN_CENTER, "Você precisa das chaves do Quebra-Cabeça e da Memória");
                            al_draw_text(font_temp, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 40,
                                ALLEGRO_ALIGN_CENTER, "Pressione qualquer tecla para voltar");
                            al_flip_display();
                            al_rest(3.0);
                            al_destroy_font(font_temp);
                        }
                    }
                    else {
                        printf("Iniciando sequência do jogo Colorir...\n");

                        mostrar_quadrinho_nove(display);

                        ALLEGRO_BITMAP* img_instrucoes_colorir = al_load_bitmap("assets/instrucoes.jpg");
                        if (img_instrucoes_colorir) {
                            ALLEGRO_EVENT_QUEUE* queue_instrucoes = al_create_event_queue();
                            al_register_event_source(queue_instrucoes, al_get_display_event_source(display));
                            al_register_event_source(queue_instrucoes, al_get_keyboard_event_source());
                            al_register_event_source(queue_instrucoes, al_get_mouse_event_source());

                            bool sair_instrucoes = false;
                            while (!sair_instrucoes) {
                                ALLEGRO_EVENT evento_instrucoes;
                                al_wait_for_event(queue_instrucoes, &evento_instrucoes);

                                if (evento_instrucoes.type == ALLEGRO_EVENT_DISPLAY_CLOSE ||
                                    evento_instrucoes.type == ALLEGRO_EVENT_KEY_DOWN ||
                                    evento_instrucoes.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                                    sair_instrucoes = true;
                                }

                                al_draw_scaled_bitmap(img_instrucoes_colorir, 0, 0,
                                    al_get_bitmap_width(img_instrucoes_colorir),
                                    al_get_bitmap_height(img_instrucoes_colorir),
                                    0, 0, SCREEN_W, SCREEN_H, 0);

                                ALLEGRO_FONT* font_temp = al_create_builtin_font();
                                if (font_temp) {
                                    al_draw_text(font_temp, al_map_rgb(255, 255, 255),
                                        SCREEN_W / 2, SCREEN_H - 50,
                                        ALLEGRO_ALIGN_CENTER, "Pressione ENTER, E, ESPAÇO ou clique para começar o jogo");
                                    al_destroy_font(font_temp);
                                }

                                al_flip_display();
                            }

                            al_destroy_event_queue(queue_instrucoes);
                            al_destroy_bitmap(img_instrucoes_colorir);
                        }

                        
                        int resultado_colorir = executar_jogo_colorir(display);

                        if (resultado_colorir == 1) {
                            printf("Colorir concluído! Mostrando imagem 10.jpg...\n");

                            printf("Iniciando Boss Final...\n");
                            int resultado_boss = executar_boss_final(display);
                            if (resultado_boss == 1) {
                                printf("BOSS DERROTADO! JOGO CONCLUÍDO!\n");
                                mostrar_tela_vitoria(display);
                            }
                            else {
                                printf("Retornando ao lobby...\n");
                                resetar_lobby();
                            }
                        }
                        else {
                            printf("Retornando ao lobby...\n");
                            resetar_lobby();
                        }
                    }
                }
                else if (i == 2) {
                    printf("Iniciando jogo da memória...\n");
                    int resultado = executar_jogo_memoria(display);
                    printf("Retornando ao lobby...\n");
                    resetar_lobby();
                }
            }
        }
    }
    e_pressionada_anteriormente = e_pressionada;
}

void inicializar_portas() {
    portas[0].x = 150;
    portas[0].y = ALTURA_TELA - 250;
    portas[0].largura = 80;
    portas[0].altura = 150;
    portas[0].jogador_proximo = false;
    snprintf(portas[0].nome, sizeof(portas[0].nome), "Quebra-Cabeça");

    portas[1].x = LARGURA_TELA / 2 - 40;
    portas[1].y = ALTURA_TELA - 250;
    portas[1].largura = 80;
    portas[1].altura = 150;
    portas[1].jogador_proximo = false;
    snprintf(portas[1].nome, sizeof(portas[1].nome), "Colorir");

    portas[2].x = LARGURA_TELA - 230;
    portas[2].y = ALTURA_TELA - 250;
    portas[2].largura = 80;
    portas[2].altura = 150;
    portas[2].jogador_proximo = false;
    snprintf(portas[2].nome, sizeof(portas[2].nome), "Jogo da Memória");
}

bool carregar_sprites_lobby() {
    fundo = al_load_bitmap("assets/lobby.jpg");
    if (!fundo) {
        al_show_native_message_box(NULL, "Erro", "Falha ao carregar fundo",
            "Verifique se 'assets/lobby.jpg' existe", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return false;
    }

    sprite_parado = al_load_bitmap("assets/parada.png");
    if (!sprite_parado) {
        al_show_native_message_box(NULL, "Erro", "Falha ao carregar sprite parado",
            "Verifique se 'assets/parada.png' existe", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return false;
    }

    char caminho[100];
    for (int i = 0; i < NUM_SPRITES; i++) {
        snprintf(caminho, sizeof(caminho), "assets/run%d.png", i + 1);
        sprites_corrida[i] = al_load_bitmap(caminho);
        if (!sprites_corrida[i]) {
            al_show_native_message_box(NULL, "Erro", "Falha ao carregar sprite de corrida",
                caminho, NULL, ALLEGRO_MESSAGEBOX_ERROR);
            return false;
        }
    }

    for (int i = 0; i < NUM_SPRITES; i++) {
        snprintf(caminho, sizeof(caminho), "assets/jump%d.png", i + 1);
        sprites_pulo[i] = al_load_bitmap(caminho);
        if (!sprites_pulo[i]) {
            al_show_native_message_box(NULL, "Erro", "Falha ao carregar sprite de pulo",
                caminho, NULL, ALLEGRO_MESSAGEBOX_ERROR);
            return false;
        }
    }

    font = al_create_builtin_font();

    return true;
}

// ----------------------- Função principal -----------------------
int main(void) {
    if (!al_init()) {
        fprintf(stderr, "Falha ao inicializar Allegro!\n");
        return -1;
    }
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_mouse();
    al_init_native_dialog_addon();

    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_mouse();
    al_init_native_dialog_addon();

    if (!al_install_audio()) {
        fprintf(stderr, "Falha ao inicializar áudio!\n");
        return -1;
    }
    if (!al_init_acodec_addon()) {
        fprintf(stderr, "Falha ao inicializar acodec!\n");
        return -1;
    }
    if (!al_reserve_samples(16)) {
        fprintf(stderr, "Falha ao reservar samples!\n");
        return -1;
    }

    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    if (!display) {
        fprintf(stderr, "Falha ao criar display!\n");
        return -1;
    }

    ALLEGRO_BITMAP* background = al_load_bitmap("assets/background.jpg");
    ALLEGRO_BITMAP* instrucoes = al_load_bitmap("assets/instrucoesmdj.jpg");
    if (!background || !instrucoes) {
        al_show_native_message_box(display, "Erro", "Falha ao carregar imagem.",
            "Verifique se os arquivos 'background.jpg' e 'instrucoesmdj.jpg' existem na pasta assets.",
            NULL, ALLEGRO_MESSAGEBOX_ERROR);
        al_destroy_display(display);
        return -1;
    }

    quadrinho_um = al_load_bitmap("assets/1.jpg");
    quadrinho_dois = al_load_bitmap("assets/2.jpg");
    quadrinho_tres = al_load_bitmap("assets/3.jpg");
    quadrinho_quatro = al_load_bitmap("assets/4.jpg");
    quadrinho_cinco = al_load_bitmap("assets/5.jpg");
    quadrinho_seis = al_load_bitmap("assets/6.jpg");
    quadrinho_oito = al_load_bitmap("assets/8.jpg");
    quadrinho_nove = al_load_bitmap("assets/9.jpg");
    quadrinho_dez = al_load_bitmap("assets/10.jpg");

    if (!quadrinho_um) printf("AVISO: Não foi possível carregar assets/1.jpg\n");
    if (!quadrinho_dois) printf("AVISO: Não foi possível carregar assets/2.jpg\n");
    if (!quadrinho_tres) printf("AVISO: Não foi possível carregar assets/3.jpg\n");
    if (!quadrinho_quatro) printf("AVISO: Não foi possível carregar assets/4.jpg\n");
    if (!quadrinho_cinco) printf("AVISO: Não foi possível carregar assets/5.jpg\n");
    if (!quadrinho_seis) printf("AVISO: Não foi possível carregar assets/6.jpg\n");
    if (!quadrinho_oito) printf("AVISO: Não foi possível carregar assets/8.jpg\n");
    if (!quadrinho_nove) printf("AVISO: Não foi possível carregar assets/9.jpg\n");
    if (!quadrinho_dez) printf("AVISO: Não foi possível carregar assets/10.jpg\n");

    ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_mouse_event_source());

    al_start_timer(timer);

    bool running = true;
    bool redraw = true;

    float rect_w = 354;
    float rect_h = 100;
    float spacing = 23;
    float pos_x = 150;
    float pos_y = 279;
    ALLEGRO_COLOR red = al_map_rgba(0, 0, 0, 0);

    int scene = 0;
    bool lobby_carregado = false;

    jogador.x = 200; jogador.y = ALTURA_TELA - 250;
    jogador.vel_x = 0; jogador.vel_y = 0;
    jogador.largura = 120; jogador.altura = 160;
    jogador.no_chao = true; jogador.direcao = 1;
    jogador.sprite_corrida = 0; jogador.contador_frames_corrida = 0;
    jogador.sprite_pulo = 0; jogador.contador_frames_pulo = 0;

    inicializar_portas();

    bool mostrando_quadrinhos = false;

    while (running) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);

        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            running = false;
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            running = false;
        }
        else if (event.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;
        }
        else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            float mouse_x = event.mouse.x;
            float mouse_y = event.mouse.y;

            if (scene == 0 && !mostrando_quadrinhos) {
                float first_top = pos_y;
                float first_bottom = pos_y + rect_h;
                if (mouse_x >= pos_x && mouse_x <= pos_x + rect_w &&
                    mouse_y >= first_top && mouse_y <= first_bottom) {
                    mostrando_quadrinhos = true;
                    cena_quadrinho = 1;
                }

                float middle_top = pos_y + rect_h + spacing;
                float middle_bottom = middle_top + rect_h;
                if (mouse_x >= pos_x && mouse_x <= pos_x + rect_w &&
                    mouse_y >= middle_top && mouse_y <= middle_bottom) {
                    scene = 1;
                }

                float last_top = pos_y + 2 * (rect_h + spacing);
                float last_bottom = last_top + rect_h;
                if (mouse_x >= pos_x && mouse_x <= pos_x + rect_w &&
                    mouse_y >= last_top && mouse_y <= last_bottom) {
                    running = false;
                }
            }
            else if (scene == 1) {
                scene = 0;
            }
            else if (mostrando_quadrinhos) {
                if (cena_quadrinho > 0) {
                    cena_quadrinho++;
                    if (cena_quadrinho > 4) {
                        cena_quadrinho = 0;
                        mostrando_quadrinhos = false;
                        scene = 2;
                        if (!lobby_carregado) {
                            lobby_carregado = carregar_sprites_lobby();
                        }
                    }
                }
            }
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (mostrando_quadrinhos && cena_quadrinho > 0) {
                if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                    cena_quadrinho++;
                    if (cena_quadrinho > 4) {
                        cena_quadrinho = 0;
                        mostrando_quadrinhos = false;
                        scene = 2;
                        if (!lobby_carregado) {
                            lobby_carregado = carregar_sprites_lobby();
                        }
                    }
                }
            }
        }

        if (scene == 2 && lobby_carregado) {
            processar_entrada_movimento();
            atualizar_jogador_movimento();
            verificar_proximidade_portas();
            processar_interacao_portas(display);
        }

        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;

            if (mostrando_quadrinhos && cena_quadrinho > 0) {
                ALLEGRO_BITMAP* quadrinho_atual = NULL;
                switch (cena_quadrinho) {
                case 1: quadrinho_atual = quadrinho_um; break;
                case 2: quadrinho_atual = quadrinho_dois; break;
                case 3: quadrinho_atual = quadrinho_tres; break;
                case 4: quadrinho_atual = quadrinho_quatro; break;
                }

                if (quadrinho_atual) {
                    al_draw_scaled_bitmap(quadrinho_atual, 0, 0,
                        al_get_bitmap_width(quadrinho_atual),
                        al_get_bitmap_height(quadrinho_atual),
                        0, 0, SCREEN_W, SCREEN_H, 0);

                    if (font) {
                        al_draw_text(font, al_map_rgb(255, 255, 255),
                            SCREEN_W / 2, SCREEN_H - 50,
                            ALLEGRO_ALIGN_CENTER, "Pressione ENTER ou clique para continuar");
                    }
                }
                else {
                    al_clear_to_color(al_map_rgb(0, 0, 0));
                    ALLEGRO_FONT* font_temp = al_create_builtin_font();
                    if (font_temp) {
                        al_draw_text(font_temp, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2,
                            ALLEGRO_ALIGN_CENTER, "Quadrinho não encontrado");
                        al_draw_text(font_temp, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2 + 30,
                            ALLEGRO_ALIGN_CENTER, "Pressione ENTER ou clique para continuar");
                        al_destroy_font(font_temp);
                    }
                }
            }
            else if (scene == 0) {
                al_draw_bitmap(background, 0, 0, 0);
                for (int i = 0; i < 3; i++) {
                    float y = pos_y + i * (rect_h + spacing);
                    al_draw_filled_rectangle(pos_x, y, pos_x + rect_w, y + rect_h, red);
                }
            }
            else if (scene == 1) {
                al_draw_bitmap(instrucoes, 0, 0, 0);
            }
            else if (scene == 2 && lobby_carregado) {
                al_draw_scaled_bitmap(fundo, 0, 0,
                    al_get_bitmap_width(fundo),
                    al_get_bitmap_height(fundo),
                    0, 0,
                    LARGURA_TELA, ALTURA_TELA, 0);

                for (int i = 0; i < numero_portas; i++) {
                    if (portas[i].jogador_proximo && font) {
                        al_draw_text(font, al_map_rgb(255, 255, 255),
                            portas[i].x + portas[i].largura / 2,
                            portas[i].y - 30,
                            ALLEGRO_ALIGN_CENTER, "Pressione E");
                    }
                }

                if (font) {
                    char buffer[100];
                    int y_pos = 20;

                    al_draw_text(font, al_map_rgb(255, 255, 255), 20, y_pos, 0, "CHAVES COLETADAS:");
                    y_pos += 30;

                    if (chave_quebra_cabeca) {
                        al_draw_text(font, al_map_rgb(255, 215, 0), 20, y_pos, 0, "🔑 Quebra-Cabeça");
                    }
                    else {
                        al_draw_text(font, al_map_rgb(100, 100, 100), 20, y_pos, 0, "🔒 Quebra-Cabeça");
                    }
                    y_pos += 30;

                    if (chave_memoria) {
                        al_draw_text(font, al_map_rgb(255, 215, 0), 20, y_pos, 0, "🔑 Memória");
                    }
                    else {
                        al_draw_text(font, al_map_rgb(100, 100, 100), 20, y_pos, 0, "🔒 Memória");
                    }
                }

                for (int i = 0; i < numero_portas; i++) {
                    if (i == 1 && portas[i].jogador_proximo && font) {
                        if (!chave_quebra_cabeca || !chave_memoria) {
                            al_draw_filled_rectangle(
                                portas[i].x - 100, portas[i].y - 80,
                                portas[i].x + portas[i].largura + 100, portas[i].y - 40,
                                al_map_rgba(0, 0, 0, 180)
                            );
                            al_draw_text(font, al_map_rgb(255, 50, 50),
                                portas[i].x + portas[i].largura / 2,
                                portas[i].y - 65,
                                ALLEGRO_ALIGN_CENTER, "🔒 PORTA TRANCADA!");

                            char msg[100];
                            snprintf(msg, sizeof(msg), "Faltam %d chave(s)",
                                2 - (chave_quebra_cabeca ? 1 : 0) - (chave_memoria ? 1 : 0));
                            al_draw_text(font, al_map_rgb(255, 255, 255),
                                portas[i].x + portas[i].largura / 2,
                                portas[i].y - 45,
                                ALLEGRO_ALIGN_CENTER, msg);
                        }
                        else {
                            al_draw_text(font, al_map_rgb(0, 255, 0),
                                portas[i].x + portas[i].largura / 2,
                                portas[i].y - 50,
                                ALLEGRO_ALIGN_CENTER, "✓ DESBLOQUEADA");
                        }
                    }
                }

                ALLEGRO_BITMAP* sprite = sprite_parado;

                if (!jogador.no_chao && sprites_pulo[jogador.sprite_pulo]) {
                    sprite = sprites_pulo[jogador.sprite_pulo];
                }
                else if (jogador.vel_x != 0 && sprites_corrida[jogador.sprite_corrida]) {
                    sprite = sprites_corrida[jogador.sprite_corrida];
                }
                else if (sprite_parado) {
                    sprite = sprite_parado;
                }

                if (sprite) {
                    if (jogador.direcao == 1) {
                        al_draw_scaled_bitmap(sprite, 0, 0,
                            al_get_bitmap_width(sprite),
                            al_get_bitmap_height(sprite),
                            jogador.x, jogador.y,
                            jogador.largura, jogador.altura, 0);
                    }
                    else {
                        al_draw_scaled_bitmap(sprite,
                            al_get_bitmap_width(sprite), 0,
                            -al_get_bitmap_width(sprite),
                            al_get_bitmap_height(sprite),
                            jogador.x, jogador.y,
                            jogador.largura, jogador.altura, 0);
                    }
                }
            }

            al_flip_display();
        }
    }

    if (quadrinho_um) al_destroy_bitmap(quadrinho_um);
    if (quadrinho_dois) al_destroy_bitmap(quadrinho_dois);
    if (quadrinho_tres) al_destroy_bitmap(quadrinho_tres);
    if (quadrinho_quatro) al_destroy_bitmap(quadrinho_quatro);
    if (quadrinho_cinco) al_destroy_bitmap(quadrinho_cinco);
    if (quadrinho_seis) al_destroy_bitmap(quadrinho_seis);
    if (quadrinho_oito) al_destroy_bitmap(quadrinho_oito);
    if (quadrinho_nove) al_destroy_bitmap(quadrinho_nove);
    if (quadrinho_dez) al_destroy_bitmap(quadrinho_dez);

    al_destroy_bitmap(background);
    al_destroy_bitmap(instrucoes);

    if (sprite_parado) al_destroy_bitmap(sprite_parado);
    if (fundo) al_destroy_bitmap(fundo);
    for (int i = 0; i < NUM_SPRITES; i++) {
        if (sprites_corrida[i]) al_destroy_bitmap(sprites_corrida[i]);
        if (sprites_pulo[i]) al_destroy_bitmap(sprites_pulo[i]);
    }
    if (font) al_destroy_font(font);

    al_destroy_display(display);
    al_destroy_event_queue(event_queue);
    al_destroy_timer(timer);

    return 0;
}