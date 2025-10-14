#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <allegro5/allegro.h>
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
#define FORCA_PULO -30.0f
#define VELOCIDADE_MOVIMENTO 6.0f
#define ALTURA_TELA 720
#define LARGURA_TELA 1280
#define NUM_SPRITES 8
#define TEMPO_ANIMACAO 5

// Estrutura para representar uma porta
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

// ----------------------- Função do Quebra-Cabeça (CORRIGIDA) -----------------------
int executar_quebra_cabeca(ALLEGRO_DISPLAY* display_main) {
    srand((unsigned)time(NULL));

    // USA O DISPLAY PRINCIPAL em vez de criar um novo
    ALLEGRO_DISPLAY* display = display_main;

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);

    // Carrega assets do quebra-cabeça
    ALLEGRO_BITMAP* instrucoes_img = al_load_bitmap("assets/instrucoesqdc.jpg");
    ALLEGRO_BITMAP* fundo_puzzle = al_load_bitmap("assets/fundo.jpg");

    if (!instrucoes_img) {
        al_show_native_message_box(display, "Erro", "Não foi possível carregar assets/instrucoes.jpg", NULL, NULL, 0);
        al_destroy_display(display);
        return 0;
    }
    if (!fundo_puzzle) {
        al_show_native_message_box(display, "Erro", "Não foi possível carregar assets/fundo.jpg", NULL, NULL, 0);
        al_destroy_display(display);
        return 0;
    }

    // Tela inicial do quebra-cabeça
    bool started = false;
    while (!started) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            al_destroy_display(display);
            al_destroy_timer(timer);
            al_destroy_event_queue(queue);
            al_destroy_bitmap(instrucoes_img);
            al_destroy_bitmap(fundo_puzzle);
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

    // Definição das fases do quebra-cabeça
    PhaseDef phases[3] = {
        { "assets/exemplo_arte_1.jpg", 120 },
        { "assets/exemplo_arte_2.jpg", 90 },
        { "assets/exemplo_arte_3.jpg", 60 }
    };

    bool todas_fases_concluidas = false;

    // Loop pelas fases
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
                al_destroy_display(display);
                return 0;
            }

            // Configurações do grid 5x5
            const int rows = 5, cols = 5;
            const int total = rows * cols;
            int src_w = al_get_bitmap_width(source);
            int src_h = al_get_bitmap_height(source);
            int piece_w = src_w / cols;
            int piece_h = src_h / rows;

            // Foto de referência
            const int ref_width = 320;
            float ref_scale = (float)ref_width / (float)src_w;
            int ref_height = (int)(src_h * ref_scale + 0.5f);

            // Calcula escala do grid
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

            // Cria as peças
            Piece* pieces = (Piece*)malloc(sizeof(Piece) * total);
            if (!pieces) {
                al_show_native_message_box(display, "Erro", "Memória insuficiente", NULL, NULL, 0);
                al_destroy_bitmap(source);
                al_destroy_display(display);
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

            // Embaralha posições
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

                    // Desenha cena do quebra-cabeça
                    al_draw_scaled_bitmap(fundo_puzzle, 0, 0,
                        al_get_bitmap_width(fundo_puzzle), al_get_bitmap_height(fundo_puzzle),
                        0, 0, SCREEN_W, SCREEN_H, 0);

                    // Desenha grid
                    for (int i = 0; i < total; i++)
                        al_draw_rectangle(pieces[i].target_x - 1, pieces[i].target_y - 1,
                            pieces[i].target_x + piece_w_s + 1, pieces[i].target_y + piece_h_s + 1,
                            al_map_rgb(90, 90, 90), 1);

                    // Desenha peças colocadas
                    for (int i = 0; i < total; i++)
                        if (pieces[i].placed)
                            al_draw_scaled_bitmap(pieces[i].bmp, 0, 0, pieces[i].w, pieces[i].h,
                                pieces[i].target_x, pieces[i].target_y, piece_w_s, piece_h_s, 0);

                    // Desenha peças soltas
                    for (int i = 0; i < total; i++)
                        if (!pieces[i].placed && i != grabbed)
                            al_draw_scaled_bitmap(pieces[i].bmp, 0, 0, pieces[i].w, pieces[i].h,
                                pieces[i].x, pieces[i].y, piece_w_s, piece_h_s, 0);

                    // Desenha peça sendo arrastada
                    if (grabbed >= 0)
                        al_draw_scaled_bitmap(pieces[grabbed].bmp, 0, 0, pieces[grabbed].w, pieces[grabbed].h,
                            pieces[grabbed].x, pieces[grabbed].y, piece_w_s, piece_h_s, 0);

                    // Desenha imagem de referência
                    float ref_x = (float)(grid_x + grid_total_w + 20);
                    float ref_y = (float)grid_y;
                    al_draw_scaled_bitmap(source, 0, 0, src_w, src_h, (int)ref_x, (int)ref_y, ref_width, ref_height, 0);

                    // Desenha timer
                    char timer_text[64];
                    int remaining = (int)ceilf(TIME_LIMIT - elapsed_time);
                    if (remaining < 0) remaining = 0;
                    snprintf(timer_text, sizeof(timer_text), "Tempo: %02d:%02d", remaining / 60, remaining % 60);
                    al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W - 300, 8, 0, timer_text);

                    // Verifica vitória
                    if (!victory_flag) {
                        int all_placed = 1;
                        for (int i = 0; i < total; i++)
                            if (!pieces[i].placed) { all_placed = 0; break; }
                        if (all_placed) victory_flag = 1;
                    }

                    // Trata fim de fase
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

            // Limpeza da fase
            for (int i = 0; i < total; i++) if (pieces[i].bmp) al_destroy_bitmap(pieces[i].bmp);
            free(pieces);
            al_destroy_bitmap(source);

            if (!phase_running && !retry_phase && !fase_concluida) {
                break; // Usuário saiu do quebra-cabeça
            }
        }

        if (phase_idx < 2 && fase_concluida) {
            // Prepara próxima fase
            al_clear_to_color(al_map_rgb(12, 12, 12));
            char tx[128]; snprintf(tx, sizeof(tx), "Preparando fase %d...", phase_idx + 2);
            al_draw_text(font, al_map_rgb(255, 215, 0), SCREEN_W / 2, SCREEN_H / 2, ALLEGRO_ALIGN_CENTRE, tx);
            al_flip_display();
            al_rest(1.5);
        }
    }

    if (todas_fases_concluidas) {
        al_clear_to_color(al_map_rgb(12, 12, 12));
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2, ALLEGRO_ALIGN_CENTRE, "Parabéns! Você concluiu todas as fases!");
        al_flip_display();
        al_rest(3.0);
    }

    // Limpeza final do quebra-cabeça
    al_destroy_bitmap(instrucoes_img);
    al_destroy_bitmap(fundo_puzzle);
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 1; // Retorna 1 se o quebra-cabeça foi concluído com sucesso
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

// ----------------------- Função do Jogo da Memória (CORRIGIDA) -----------------------
int executar_jogo_memoria(ALLEGRO_DISPLAY* display_main) {
    srand((unsigned)time(NULL));

    // USA O DISPLAY PRINCIPAL
    ALLEGRO_DISPLAY* display = display_main;

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    // Carrega assets do jogo da memória
    ALLEGRO_BITMAP* background_memoria = al_load_bitmap("assets/backgroundjdm.jpg");
    if (!background_memoria) {
        fprintf(stderr, "Erro ao carregar imagem de fundo do jogo da memória\n");
        al_show_native_message_box(display, "Erro", "Arquivo não encontrado",
            "Verifique se assets/background.jpg existe", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        al_destroy_display(display);
        return 0;
    }

    const char* nomes_imagens[7] = {
        "assets/img1.jpg",
        "assets/img2.jpg",
        "assets/img3.jpg",
        "assets/jc.jpg",
        "assets/pintinho.jpg",
        "assets/img5.jpg",
        "assets/img6.jpg"
    };

    // Verifica se as imagens das cartas existem
    for (int i = 0; i < 7; i++) {
        ALLEGRO_BITMAP* test = al_load_bitmap(nomes_imagens[i]);
        if (!test) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg),
                "Arquivo não encontrado: %s\n\nVerifique se o arquivo existe na pasta assets.",
                nomes_imagens[i]);
            al_show_native_message_box(display, "Erro", "Asset não encontrado", err_msg, NULL, 0);
            al_destroy_display(display);
            return 0;
        }
        al_destroy_bitmap(test);
    }

    int total_cartas = LINHAS_MEMORIA * COLUNAS_MEMORIA;
    int ids[LINHAS_MEMORIA * COLUNAS_MEMORIA];

    // Preenche os IDs dos pares
    for (int i = 0; i < total_cartas / 2; i++) {
        if (i < 7)
            ids[2 * i] = i;
        else
            ids[2 * i] = 6;
        if (i < 7)
            ids[2 * i + 1] = i;
        else
            ids[2 * i + 1] = 6;
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
            if (img_index > 6) img_index = 6;
            cartas[i][j].imagem = al_load_bitmap(nomes_imagens[img_index]);
            if (!cartas[i][j].imagem) {
                fprintf(stderr, "Erro ao carregar imagem %s\n", nomes_imagens[img_index]);
                al_destroy_display(display);
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

    al_start_timer(timer);

    // Centralizar tabuleiro
    int area_x = (SCREEN_W - AREA_LARGURA_MEMORIA) / 2;
    int area_y = (SCREEN_H - AREA_ALTURA_MEMORIA) / 2;

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = 0;
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            rodando = 0;
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
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

        // Desenhar fundo
        al_draw_scaled_bitmap(background_memoria, 0, 0,
            al_get_bitmap_width(background_memoria), al_get_bitmap_height(background_memoria),
            0, 0, SCREEN_W, SCREEN_H, 0);

        // Moldura do tabuleiro
        al_draw_filled_rounded_rectangle(area_x - 15, area_y - 15,
            area_x + AREA_LARGURA_MEMORIA + 15, area_y + AREA_ALTURA_MEMORIA + 15,
            20, 20, al_map_rgb(30, 30, 30));

        al_draw_rounded_rectangle(area_x - 15, area_y - 15,
            area_x + AREA_LARGURA_MEMORIA + 15, area_y + AREA_ALTURA_MEMORIA + 15,
            20, 20, al_map_rgb(255, 255, 255), 3);

        // Desenha cartas
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

        // Desenha informações
        al_draw_textf(font, al_map_rgb(255, 255, 255),
            SCREEN_W / 2, SCREEN_H - 60, ALLEGRO_ALIGN_CENTER,
            "Pares encontrados: %d / %d", pares_encontrados, total_cartas / 2);

        // No jogo da memória, substitua toda a parte de vitória por isso:
        if (pares_encontrados == total_cartas / 2) {
            al_draw_text(font, al_map_rgb(255, 255, 0), SCREEN_W / 2, SCREEN_H - 30,
                ALLEGRO_ALIGN_CENTER, "🎉 Você venceu! Voltando ao lobby...");

            // Espera 3 segundos e volta automaticamente
            static double tempo_vitoria = 0;
            if (tempo_vitoria == 0) {
                tempo_vitoria = al_get_time();
            }
            if (al_get_time() - tempo_vitoria > 3.0) {
                rodando = 0; // Volta automaticamente após 3 segundos
            }
        }
        else {
            al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, 20,
                ALLEGRO_ALIGN_CENTER, "Jogo da Memória - Pressione ESC para voltar");
        }

        // E mantenha apenas a verificação do ESC para sair durante o jogo:
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            rodando = 0;
        }

        al_flip_display();
    }

    // Limpeza
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

    if (al_key_down(&kb, ALLEGRO_KEY_E)) {
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

// ----------------------- Função Principal do Jogo Colorir -----------------------
int executar_jogo_colorir(ALLEGRO_DISPLAY* display_main) {
    ALLEGRO_DISPLAY* display = display_main;
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    // ---------- VARIÁVEIS DO COLORIR ----------
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

    // ---------- INICIALIZAÇÃO DO COLORIR ----------
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

    // ---------- CARREGAR IMAGENS DO COLORIR ----------
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

    // Variáveis de animação
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
        else if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && cena_instrucoes) {
            cena_instrucoes = false;
        }
        else if (evento.type == ALLEGRO_EVENT_TIMER && !cena_instrucoes) {
            // PROCESSAMENTO DO JOGO
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

            // ANIMAÇÕES
            float delta = 1.0 / FPS;

            // Animação de pulo
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

            // Animação de corrida
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

            // Animação de morte
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

            // VERIFICA SE PEGOU O BALDE
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

            // VERIFICA SE CONCLUIU O JOGO
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

                // Desenha imagem do estado atual
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

                // Desenha chão
                al_draw_filled_rectangle(0, ALTURA_TELA - 100, LARGURA_TELA, ALTURA_TELA, al_map_rgb(101, 67, 33));

                // Desenha plataformas
                for (int i = 0; i < NUM_PLATAFORMAS_COLORIR; i++) {
                    al_draw_filled_rectangle(
                        plataformas_colorir[i].x, plataformas_colorir[i].y,
                        plataformas_colorir[i].x + plataformas_colorir[i].largura, plataformas_colorir[i].y + plataformas_colorir[i].altura,
                        al_map_rgb(70, 70, 70)
                    );
                }

                // Desenha balde se não foi pego
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

                // Desenha fantasmas
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

                // Desenha jogador
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

    // ---------- LIMPEZA ----------
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
#define LARGURA_TELA_BOSS 1280
#define ALTURA_TELA_BOSS 720
#define FPS_BOSS 60
#define GRAVIDADE_BOSS 0.4f
#define FORCA_PULO_BOSS -12.5f
#define VELOCIDADE_MOVIMENTO_BOSS 3.0f
#define MAX_PLATAFORMAS_BOSS 8
#define TAMANHO_BLOCO_PLATAFORMA_BOSS 32
#define MAX_TIROS_JOGADOR_BOSS 50
#define MAX_TIROS_BOSS_FINAL 40
#define MAX_CAPANGAS_BOSS 4
#define MAX_TIROS_CAPANGA_BOSS 20
#define VIDA_CAPANGA_BOSS 50
#define VELOCIDADE_CAPANGA_BOSS 1.25f

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
    bool no_chao, abaixado;
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
} CapangaBoss;

// ----------------------- Funções auxiliares do Boss Final -----------------------
void atualizar_fisica_entidade_boss(float* x, float* y, float* vel_y, int largura, int altura, bool* no_chao, bool ignora_plataformas, PlataformaBoss* plataformas) {
    if (!*no_chao) *vel_y += GRAVIDADE_BOSS;
    *y += *vel_y;

    float chao_y_pos = ALTURA_TELA_BOSS - 50;
    bool em_contato = false;

    if (*y >= chao_y_pos - altura) {
        *y = chao_y_pos - altura;
        em_contato = true;
    }
    else if (*vel_y >= 0 && !ignora_plataformas) {
        for (int i = 0; i < MAX_PLATAFORMAS_BOSS; i++) {
            if (*x + largura > plataformas[i].x && *x < plataformas[i].x + plataformas[i].largura &&
                *y + altura >= plataformas[i].y && *y + altura <= plataformas[i].y + 10) {
                *y = plataformas[i].y - altura;
                em_contato = true;
                break;
            }
        }
    }
    if (em_contato) *vel_y = 0;
    *no_chao = em_contato;
}

void criar_tiro_boss(TiroBoss tiros[], int max_tiros, float x, float y, float vel_x, float vel_y) {
    for (int i = 0; i < max_tiros; i++) {
        if (!tiros[i].ativo) {
            tiros[i] = (TiroBoss){ .x = x, .y = y, .vel_x = vel_x, .vel_y = vel_y, .ativo = true };
            break;
        }
    }
}

void criar_tiros_direcionados_boss(TiroBoss tiros[], int max_tiros, float start_x, float start_y, float target_x, float target_y, float speed) {
    float dx = target_x - start_x;
    float dy = target_y - start_y;
    float dist = sqrt(dx * dx + dy * dy);
    if (dist > 0)
        criar_tiro_boss(tiros, max_tiros, start_x, start_y, (dx / dist) * speed, (dy / dist) * speed);
}

bool colidiu_boss(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

// ----------------------- Função do Boss Final (VERSÃO CORRIGIDA) -----------------------
int executar_boss_final(ALLEGRO_DISPLAY* display_main) {
    ALLEGRO_DISPLAY* display = display_main;
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS_BOSS);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_FONT* font = al_create_builtin_font();

    // Carrega assets
    ALLEGRO_BITMAP* background_image = al_load_bitmap("assets/inferno.jpg");
    ALLEGRO_BITMAP* platform_block_image = al_load_bitmap("assets/floor.png");

    if (!background_image) {
        printf("Erro ao carregar assets/inferno.jpg\n");
        // Continua mesmo sem imagem
    }
    if (!platform_block_image) {
        printf("Erro ao carregar assets/floor.png\n");
        // Continua mesmo sem imagem
    }

    // INICIALIZAÇÃO CORRETA - ZERANDO TUDO
    JogadorBoss jogador;
    BossFinal boss;
    CapangaBoss capangas[MAX_CAPANGAS_BOSS];
    TiroBoss tiros_jogador[MAX_TIROS_JOGADOR_BOSS];
    TiroBoss tiros_boss[MAX_TIROS_BOSS_FINAL];
    TiroBoss tiros_capangas[MAX_TIROS_CAPANGA_BOSS];

    // Zera todas as variáveis
    memset(&jogador, 0, sizeof(jogador));
    memset(&boss, 0, sizeof(boss));
    memset(capangas, 0, sizeof(capangas));
    memset(tiros_jogador, 0, sizeof(tiros_jogador));
    memset(tiros_boss, 0, sizeof(tiros_boss));
    memset(tiros_capangas, 0, sizeof(tiros_capangas));

    // Inicializa com valores específicos
    jogador.x = 100;
    jogador.y = ALTURA_TELA_BOSS - 125;
    jogador.largura = 40;
    jogador.altura = 60;
    jogador.no_chao = true;
    jogador.abaixado = false;
    jogador.direcao = 1;
    jogador.vida = 100;

    boss.x = LARGURA_TELA_BOSS - 300;
    boss.y = ALTURA_TELA_BOSS - 550;
    boss.vida = 500;
    boss.vida_maxima = 500;
    boss.ativo = true;
    boss.segunda_fase = false;
    boss.tempo_tiro = 0;
    boss.largura = 300;
    boss.altura = 500;

    PlataformaBoss plataformas[MAX_PLATAFORMAS_BOSS] = {
        {50, 270, 350, 10}, {550, 545, 400, 10},
        {100, 120, 400, 10}, {150, 420, 400, 10},
        {50, 545, 400, 10},  {500, 270, 400, 10},
        {600, 120, 400, 10}, {650, 420, 400, 10}
    };

    int pontuacao = 0;
    int tempo_spawn_capanga = 0;
    bool jogo_rodando = true;

    // Cores
    ALLEGRO_COLOR cor_jogador = al_map_rgb(0, 0, 0);
    ALLEGRO_COLOR cor_tiro_jogador = al_map_rgb(255, 255, 0);
    ALLEGRO_COLOR cor_boss = al_map_rgb(150, 0, 150);
    ALLEGRO_COLOR cor_tiro_boss = al_map_rgb(255, 100, 100);
    ALLEGRO_COLOR cor_capanga = al_map_rgb(50, 150, 50);
    ALLEGRO_COLOR cor_tiro_capanga = al_map_rgb(150, 255, 150);

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());

    al_start_timer(timer);

    bool sair = false;
    bool redesenhar = true;

    while (!sair && jogo_rodando) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(queue, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN && evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            sair = true;
        }
        else if (evento.type == ALLEGRO_EVENT_TIMER) {
            // ATUALIZAÇÃO DO JOGADOR
            jogador.x += jogador.vel_x;
            if (jogador.x < 0) jogador.x = 0;
            if (jogador.x > LARGURA_TELA_BOSS - jogador.largura) jogador.x = LARGURA_TELA_BOSS - jogador.largura;
            atualizar_fisica_entidade_boss(&jogador.x, &jogador.y, &jogador.vel_y, jogador.largura, jogador.altura, &jogador.no_chao, jogador.abaixado, plataformas);

            // ATUALIZAÇÃO DO BOSS
            if (boss.ativo) {
                if (boss.vida <= boss.vida_maxima / 2 && !boss.segunda_fase) {
                    boss.segunda_fase = true;
                }

                boss.tempo_tiro++;
                if (boss.tempo_tiro >= (boss.segunda_fase ? 30 : 60)) {
                    float centro_boss_x = boss.x + boss.largura / 2.0f;
                    float centro_boss_y = boss.y + boss.altura / 2.0f;
                    float centro_jogador_x = jogador.x + jogador.largura / 2.0f;
                    float centro_jogador_y = jogador.y + jogador.altura / 2.0f;

                    criar_tiros_direcionados_boss(tiros_boss, MAX_TIROS_BOSS_FINAL, centro_boss_x, centro_boss_y, centro_jogador_x, centro_jogador_y, 4.0f);
                    if (boss.segunda_fase) {
                        criar_tiros_direcionados_boss(tiros_boss, MAX_TIROS_BOSS_FINAL, centro_boss_x, centro_boss_y, centro_jogador_x, centro_jogador_y, 4.0f);
                    }
                    boss.tempo_tiro = 0;
                }
            }

            // ATUALIZAÇÃO DOS CAPANGAS
            if (boss.ativo && boss.vida <= boss.vida_maxima / 2) {
                tempo_spawn_capanga++;
                if (tempo_spawn_capanga > FPS_BOSS * 4) {
                    for (int i = 0; i < MAX_CAPANGAS_BOSS; i++) {
                        if (!capangas[i].ativo) {
                            if ((rand() % 2 == 0)) {
                                capangas[i].x = 25.0f;
                                capangas[i].y = 25;
                                capangas[i].largura = 25;
                                capangas[i].altura = 35;
                                capangas[i].vida = VIDA_CAPANGA_BOSS;
                                capangas[i].ativo = true;
                                capangas[i].tempo_tiro = rand() % 100;
                                capangas[i].vel_x = 0;
                                capangas[i].vel_y = 0;
                                capangas[i].no_chao = false;
                            }
                            else {
                                capangas[i].x = LARGURA_TELA_BOSS - 50.0f;
                                capangas[i].y = 25;
                                capangas[i].largura = 25;
                                capangas[i].altura = 35;
                                capangas[i].vida = VIDA_CAPANGA_BOSS;
                                capangas[i].ativo = true;
                                capangas[i].tempo_tiro = rand() % 100;
                                capangas[i].vel_x = 0;
                                capangas[i].vel_y = 0;
                                capangas[i].no_chao = false;
                            }
                            break;
                        }
                    }
                    tempo_spawn_capanga = 0;
                }
            }

            for (int i = 0; i < MAX_CAPANGAS_BOSS; i++) {
                if (!capangas[i].ativo) continue;

                capangas[i].vel_x = (jogador.x > capangas[i].x) ? VELOCIDADE_CAPANGA_BOSS : -VELOCIDADE_CAPANGA_BOSS;
                capangas[i].x += capangas[i].vel_x;
                atualizar_fisica_entidade_boss(&capangas[i].x, &capangas[i].y, &capangas[i].vel_y, capangas[i].largura, capangas[i].altura, &capangas[i].no_chao, false, plataformas);

                capangas[i].tempo_tiro++;
                if (capangas[i].tempo_tiro > FPS_BOSS * 2) {
                    criar_tiros_direcionados_boss(tiros_capangas, MAX_TIROS_CAPANGA_BOSS,
                        capangas[i].x + capangas[i].largura / 2.0f,
                        capangas[i].y + capangas[i].altura / 2.0f,
                        jogador.x + jogador.largura / 2.0f,
                        jogador.y + jogador.altura / 2.0f, 3.0f);
                    capangas[i].tempo_tiro = 0;
                }
            }

            // ATUALIZAÇÃO DOS TIROS
            for (int i = 0; i < MAX_TIROS_JOGADOR_BOSS; i++) {
                if (!tiros_jogador[i].ativo) continue;
                tiros_jogador[i].x += tiros_jogador[i].vel_x;
                tiros_jogador[i].y += tiros_jogador[i].vel_y;
                if (tiros_jogador[i].x < 0 || tiros_jogador[i].x > LARGURA_TELA_BOSS || tiros_jogador[i].y < 0 || tiros_jogador[i].y > ALTURA_TELA_BOSS) {
                    tiros_jogador[i].ativo = false;
                }
            }

            for (int i = 0; i < MAX_TIROS_BOSS_FINAL; i++) {
                if (!tiros_boss[i].ativo) continue;
                tiros_boss[i].x += tiros_boss[i].vel_x;
                tiros_boss[i].y += tiros_boss[i].vel_y;
                if (tiros_boss[i].x < 0 || tiros_boss[i].x > LARGURA_TELA_BOSS || tiros_boss[i].y < 0 || tiros_boss[i].y > ALTURA_TELA_BOSS) {
                    tiros_boss[i].ativo = false;
                }
            }

            for (int i = 0; i < MAX_TIROS_CAPANGA_BOSS; i++) {
                if (!tiros_capangas[i].ativo) continue;
                tiros_capangas[i].x += tiros_capangas[i].vel_x;
                tiros_capangas[i].y += tiros_capangas[i].vel_y;
                if (tiros_capangas[i].x < 0 || tiros_capangas[i].x > LARGURA_TELA_BOSS || tiros_capangas[i].y < 0 || tiros_capangas[i].y > ALTURA_TELA_BOSS) {
                    tiros_capangas[i].ativo = false;
                }
            }

            // VERIFICAÇÃO DE COLISÕES
            for (int i = 0; i < MAX_TIROS_JOGADOR_BOSS; i++) {
                if (!tiros_jogador[i].ativo) continue;

                // Colisão com boss
                if (boss.ativo && colidiu_boss(tiros_jogador[i].x, tiros_jogador[i].y, 4, 2, boss.x, boss.y, boss.largura, boss.altura)) {
                    tiros_jogador[i].ativo = false;
                    boss.vida -= 2;
                    pontuacao += 10;
                    if (boss.vida <= 0) {
                        boss.ativo = false;
                        pontuacao += 1000;
                    }
                }

                // Colisão com capangas
                for (int j = 0; j < MAX_CAPANGAS_BOSS; j++) {
                    if (capangas[j].ativo && colidiu_boss(tiros_jogador[i].x, tiros_jogador[i].y, 4, 2, capangas[j].x, capangas[j].y, capangas[j].largura, capangas[j].altura)) {
                        tiros_jogador[i].ativo = false;
                        capangas[j].vida -= 20;
                        pontuacao += 5;
                        if (capangas[j].vida <= 0) {
                            capangas[j].ativo = false;
                            pontuacao += 50;
                        }
                        break;
                    }
                }
            }

            // Colisão tiros do boss com jogador
            for (int i = 0; i < MAX_TIROS_BOSS_FINAL; i++) {
                if (tiros_boss[i].ativo && colidiu_boss(tiros_boss[i].x, tiros_boss[i].y, 6, 6, jogador.x, jogador.y, jogador.largura, jogador.altura)) {
                    tiros_boss[i].ativo = false;
                    jogador.vida -= 15;
                    if (jogador.vida <= 0) {
                        // Reset do jogo se jogador morrer
                        jogador.x = 100;
                        jogador.y = ALTURA_TELA_BOSS - 125;
                        jogador.largura = 40;
                        jogador.altura = 60;
                        jogador.no_chao = true;
                        jogador.abaixado = false;
                        jogador.direcao = 1;
                        jogador.vida = 100;
                        jogador.vel_x = 0;
                        jogador.vel_y = 0;
                    }
                }
            }

            // Colisão tiros dos capangas com jogador
            for (int i = 0; i < MAX_TIROS_CAPANGA_BOSS; i++) {
                if (tiros_capangas[i].ativo && colidiu_boss(tiros_capangas[i].x, tiros_capangas[i].y, 5, 5, jogador.x, jogador.y, jogador.largura, jogador.altura)) {
                    tiros_capangas[i].ativo = false;
                    jogador.vida -= 5;
                    if (jogador.vida <= 0) {
                        jogador.x = 100;
                        jogador.y = ALTURA_TELA_BOSS - 125;
                        jogador.largura = 40;
                        jogador.altura = 60;
                        jogador.no_chao = true;
                        jogador.abaixado = false;
                        jogador.direcao = 1;
                        jogador.vida = 100;
                        jogador.vel_x = 0;
                        jogador.vel_y = 0;
                    }
                }
            }

            // Colisão jogador com boss
            if (boss.ativo && colidiu_boss(jogador.x, jogador.y, jogador.largura, jogador.altura, boss.x, boss.y, boss.largura, boss.altura)) {
                jogador.vida = 0;
                jogador.x = 100;
                jogador.y = ALTURA_TELA_BOSS - 125;
                jogador.largura = 40;
                jogador.altura = 60;
                jogador.no_chao = true;
                jogador.abaixado = false;
                jogador.direcao = 1;
                jogador.vida = 100;
                jogador.vel_x = 0;
                jogador.vel_y = 0;
            }

            // VERIFICA SE O BOSS FOI DERROTADO
            if (!boss.ativo) {
                jogo_rodando = false;
            }

            redesenhar = true;
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            // Controles do jogador
            if (evento.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                criar_tiro_boss(tiros_jogador, MAX_TIROS_JOGADOR_BOSS,
                    jogador.x + (jogador.direcao == 1 ? jogador.largura : 0),
                    jogador.y + jogador.altura / 2,
                    jogador.direcao * 6.0f, 0);
            }
        }

        // Processa movimento contínuo
        ALLEGRO_KEYBOARD_STATE kb;
        al_get_keyboard_state(&kb);
        jogador.vel_x = (al_key_down(&kb, ALLEGRO_KEY_D) - al_key_down(&kb, ALLEGRO_KEY_A)) * VELOCIDADE_MOVIMENTO_BOSS;
        if (jogador.vel_x != 0) jogador.direcao = (jogador.vel_x > 0) ? 1 : -1;
        if (al_key_down(&kb, ALLEGRO_KEY_W) && jogador.no_chao) {
            jogador.vel_y = FORCA_PULO_BOSS;
            jogador.no_chao = false;
        }
        jogador.abaixado = al_key_down(&kb, ALLEGRO_KEY_S);

        if (redesenhar && al_is_event_queue_empty(queue)) {
            redesenhar = false;

            // DESENHO DO JOGO
            if (background_image) {
                al_draw_scaled_bitmap(background_image,
                    0, 0, al_get_bitmap_width(background_image), al_get_bitmap_height(background_image),
                    0, 0, LARGURA_TELA_BOSS, ALTURA_TELA_BOSS, 0);
            }
            else {
                // Fundo alternativo se não carregar a imagem
                al_clear_to_color(al_map_rgb(20, 20, 40));
            }

            // Desenha chão principal
            if (platform_block_image) {
                float chao_y_pos = ALTURA_TELA_BOSS - 50;
                int num_blocos_chao = LARGURA_TELA_BOSS / TAMANHO_BLOCO_PLATAFORMA_BOSS;
                if (LARGURA_TELA_BOSS % TAMANHO_BLOCO_PLATAFORMA_BOSS != 0) num_blocos_chao++;

                for (int i = 0; i < num_blocos_chao; i++) {
                    al_draw_scaled_bitmap(platform_block_image,
                        0, 0, al_get_bitmap_width(platform_block_image), al_get_bitmap_height(platform_block_image),
                        i * TAMANHO_BLOCO_PLATAFORMA_BOSS, chao_y_pos,
                        TAMANHO_BLOCO_PLATAFORMA_BOSS, 50, 0);
                }
            }
            else {
                // Chão alternativo
                al_draw_filled_rectangle(0, ALTURA_TELA_BOSS - 50, LARGURA_TELA_BOSS, ALTURA_TELA_BOSS, al_map_rgb(80, 60, 30));
            }

            // Desenha plataformas
            if (platform_block_image) {
                for (int i = 0; i < MAX_PLATAFORMAS_BOSS; i++) {
                    int num_blocos_plataforma = plataformas[i].largura / TAMANHO_BLOCO_PLATAFORMA_BOSS;
                    if (plataformas[i].largura % TAMANHO_BLOCO_PLATAFORMA_BOSS != 0) num_blocos_plataforma++;

                    for (int j = 0; j < num_blocos_plataforma; j++) {
                        al_draw_scaled_bitmap(platform_block_image,
                            0, 0, al_get_bitmap_width(platform_block_image), al_get_bitmap_height(platform_block_image),
                            plataformas[i].x + (j * TAMANHO_BLOCO_PLATAFORMA_BOSS), plataformas[i].y,
                            TAMANHO_BLOCO_PLATAFORMA_BOSS, plataformas[i].altura, 0);
                    }
                }
            }
            else {
                // Plataformas alternativas
                for (int i = 0; i < MAX_PLATAFORMAS_BOSS; i++) {
                    al_draw_filled_rectangle(plataformas[i].x, plataformas[i].y,
                        plataformas[i].x + plataformas[i].largura,
                        plataformas[i].y + plataformas[i].altura,
                        al_map_rgb(100, 100, 100));
                }
            }

            // Desenha jogador
            float altura_desenhada = jogador.abaixado ? jogador.altura / 2.0f : jogador.altura;
            al_draw_filled_rectangle(jogador.x, jogador.y + (jogador.abaixado ? altura_desenhada : 0),
                jogador.x + jogador.largura, jogador.y + jogador.altura, cor_jogador);

            // Desenha boss
            if (boss.ativo) {
                al_draw_filled_rectangle(boss.x, boss.y, boss.x + boss.largura, boss.y + boss.altura, cor_boss);
                float vida_pc = (float)boss.vida / boss.vida_maxima;
                al_draw_filled_rectangle(boss.x, boss.y - 15, boss.x + boss.largura, boss.y - 5, al_map_rgb(100, 0, 0));
                al_draw_filled_rectangle(boss.x, boss.y - 15, boss.x + boss.largura * vida_pc, boss.y - 5, al_map_rgb(255, 0, 0));
            }

            // Desenha capangas
            for (int i = 0; i < MAX_CAPANGAS_BOSS; i++) {
                if (capangas[i].ativo) {
                    al_draw_filled_rectangle(capangas[i].x, capangas[i].y,
                        capangas[i].x + capangas[i].largura,
                        capangas[i].y + capangas[i].altura, cor_capanga);
                }
            }

            // Desenha tiros
            for (int i = 0; i < MAX_TIROS_JOGADOR_BOSS; i++) {
                if (tiros_jogador[i].ativo) {
                    al_draw_filled_circle(tiros_jogador[i].x, tiros_jogador[i].y, 2, cor_tiro_jogador);
                }
            }
            for (int i = 0; i < MAX_TIROS_BOSS_FINAL; i++) {
                if (tiros_boss[i].ativo) {
                    al_draw_filled_circle(tiros_boss[i].x, tiros_boss[i].y, 3, cor_tiro_boss);
                }
            }
            for (int i = 0; i < MAX_TIROS_CAPANGA_BOSS; i++) {
                if (tiros_capangas[i].ativo) {
                    al_draw_filled_circle(tiros_capangas[i].x, tiros_capangas[i].y, 3, cor_tiro_capanga);
                }
            }

            // Desenha HUD - CORRIGIDO SEM SPRINTF_S
            char buffer[100];
            printf(buffer, "VIDA: %d", jogador.vida);
            al_draw_text(font, al_map_rgb(255, 255, 255), 20, 20, 0, buffer);
            printf(buffer, "PONTOS: %d", pontuacao);
            al_draw_text(font, al_map_rgb(255, 255, 255), 20, 50, 0, buffer);

            if (boss.ativo) {
                printf(buffer, "BOSS: %d/%d", boss.vida, boss.vida_maxima);
                al_draw_text(font, al_map_rgb(255, 255, 255), LARGURA_TELA_BOSS - 150, 20, ALLEGRO_ALIGN_RIGHT, buffer);
            }
            else {
                al_draw_text(font, al_map_rgb(255, 255, 0), LARGURA_TELA_BOSS / 2, ALTURA_TELA_BOSS / 2,
                    ALLEGRO_ALIGN_CENTER, "VILÃO DERROTADO!");
            }

            al_flip_display();
        }
    }

    // LIMPEZA
    if (background_image) al_destroy_bitmap(background_image);
    if (platform_block_image) al_destroy_bitmap(platform_block_image);
    if (font) al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    // Retorna 1 se o boss foi derrotado, 0 se o jogador saiu
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

        // Efeito de texto piscante
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
    // Reposiciona o jogador no centro
    jogador.x = 200;
    jogador.y = ALTURA_TELA - 250;
    jogador.vel_x = 0;
    jogador.vel_y = 0;
    jogador.no_chao = true;
    jogador.direcao = 1;

    // Reseta animações
    jogador.sprite_corrida = 0;
    jogador.contador_frames_corrida = 0;
    jogador.sprite_pulo = 0;
    jogador.contador_frames_pulo = 0;

    // Reseta estado das portas
    for (int i = 0; i < numero_portas; i++) {
        portas[i].jogador_proximo = false;
    }
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

void processar_interacao_portas(ALLEGRO_DISPLAY* display) {
    ALLEGRO_KEYBOARD_STATE kb;
    al_get_keyboard_state(&kb);

    static bool e_pressionada_anteriormente = false;
    bool e_pressionada = al_key_down(&kb, ALLEGRO_KEY_E);

    if (e_pressionada && !e_pressionada_anteriormente) {
        for (int i = 0; i < numero_portas; i++) {
            if (portas[i].jogador_proximo) {
                printf("Interagindo com a porta: %s\n", portas[i].nome);

                if (i == 0) { // Quebra-cabeça
                    printf("Iniciando quebra-cabeça...\n");
                    executar_quebra_cabeca(display);
                    printf("Retornando ao lobby...\n");
                    resetar_lobby();
                }
                else if (i == 1) { // Colorir
                    printf("Iniciando jogo Colorir...\n");
                    int resultado = executar_jogo_colorir(display);
                    if (resultado == 1) {
                        printf("Colorir concluído! Indo para Boss Final...\n");
                        int resultado_boss = executar_boss_final(display);
                        if (resultado_boss == 1) {
                            printf("BOSS DERROTADO! JOGO CONCLUÍDO!\n");
                            // AQUI VAMOS MOSTRAR A TELA FINAL
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
                else if (i == 2) { // Jogo da Memória
                    printf("Iniciando jogo da memória...\n");
                    executar_jogo_memoria(display);
                    printf("Retornando ao lobby...\n");
                    resetar_lobby();
                }
            }
        }
    }
    e_pressionada_anteriormente = e_pressionada;
}

void inicializar_portas() {
    // Porta 1 - Esquerda (Quebra-cabeça)
    portas[0].x = 150;
    portas[0].y = ALTURA_TELA - 250;
    snprintf(portas[0].nome, sizeof(portas[0].nome), "Quebra-Cabeça");

    // Porta 2 - Centro (Colorir)
    portas[1].x = LARGURA_TELA / 2 - 40;
    portas[1].y = ALTURA_TELA - 250;
    portas[1].largura = 80;
    portas[1].altura = 150;
    portas[1].jogador_proximo = false;
    snprintf(portas[1].nome, sizeof(portas[1].nome), "Colorir");

    // Porta 3 - Direita (Jogo da Memória)
    portas[2].x = LARGURA_TELA - 230;
    portas[2].y = ALTURA_TELA - 250;
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

    al_init_font_addon();
    al_init_ttf_addon();
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

    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    if (!display) {
        fprintf(stderr, "Falha ao criar display!\n");
        return -1;
    }

    ALLEGRO_BITMAP* background = al_load_bitmap("assets/background.jpg");
    ALLEGRO_BITMAP* instrucoes = al_load_bitmap("assets/instrucoesmdj.jpg");
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
                float top = pos_y;
                float bottom = top + rect_h;
                if (mouse_x >= pos_x && mouse_x <= pos_x + rect_w &&
                    mouse_y >= top && mouse_y <= bottom) {
                    scene = 2;
                    if (!lobby_carregado) {
                        lobby_carregado = carregar_sprites_lobby();
                        if (!lobby_carregado) {
                            running = false;
                        }
                    }
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
        }

        if (scene == 2 && lobby_carregado) {
            processar_entrada_movimento();
            atualizar_jogador_movimento();
            verificar_proximidade_portas();
            processar_interacao_portas(display);
        }

        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;

            if (scene == 0) {
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

                // Desenha texto de interação apenas quando jogador está próximo
                for (int i = 0; i < numero_portas; i++) {
                    if (portas[i].jogador_proximo && font) {
                        al_draw_text(font, al_map_rgb(255, 255, 255),
                            portas[i].x + portas[i].largura / 2,
                            portas[i].y - 30,
                            ALLEGRO_ALIGN_CENTER, "Pressione E");
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

    al_destroy_bitmap(background);
    al_destroy_bitmap(instrucoes);

    // Libera sprites do lobby
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