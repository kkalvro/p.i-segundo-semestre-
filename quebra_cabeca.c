//
// Created by Juliano Duarte on 12/10/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

// --- Ajuste para compatibilidade entre Windows e macOS/Linux ---
#ifdef _WIN32
    #include <direct.h>     // Windows: _getcwd, _mkdir
    #define GETCWD _getcwd
#else
    #include <unistd.h>     // macOS/Linux: getcwd
    #include <sys/stat.h>   // Para mkdir() se precisar
    #define GETCWD getcwd
#endif
// ---------------------------------------------------------------

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>

#define SCREEN_W 1280
#define SCREEN_H 720
#define FPS 60

typedef struct {
    ALLEGRO_BITMAP* bmp;
    float x, y;                 // posição atual (em tela)
    float target_x, target_y;   // posição alvo (grade)
    int placed;
    int w, h;                   // largura/altura originais da peça
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

int main(void) {
    srand((unsigned)time(NULL));

    if (!al_init()) { fprintf(stderr, "Falha ao inicializar Allegro\n"); return -1; }
    al_init_image_addon();
    al_init_primitives_addon();
    al_init_font_addon();
    al_install_mouse();
    al_install_keyboard();
    al_init_native_dialog_addon();

    char cwd[1024];
    if (GETCWD(cwd, sizeof(cwd))) printf("Working dir: %s\n", cwd);

    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    if (!display) { fprintf(stderr, "Falha ao criar display\n"); return -1; }

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* font = al_create_builtin_font();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);

    // --- CARREGA AS IMAGENS NOVAS ---
    ALLEGRO_BITMAP* instrucoes_img = al_load_bitmap("assets/instrucoes.jpg");
    ALLEGRO_BITMAP* fundo = al_load_bitmap("assets/fundo.jpg");

    if (!instrucoes_img) {
        al_show_native_message_box(display, "Erro", "Não foi possível carregar assets/instrucoes.jpg", NULL, NULL, 0);
        return -1;
    }
    if (!fundo) {
        al_show_native_message_box(display, "Erro", "Não foi possível carregar assets/fundo.jpg", NULL, NULL, 0);
        return -1;
    }

    // Definição das fases
    PhaseDef phases[3] = {
        { "assets/exemplo_arte_1.jpg", 120 },
        { "assets/exemplo_arte_2.jpg", 90  },
        { "assets/exemplo_arte_3.jpg", 60  }
    };

    // TELA INICIAL — começa quando usuário aperta ENTER ou dá um clique no mouse
    bool started = false;
    while (!started) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            al_destroy_display(display);
            al_destroy_timer(timer);
            al_destroy_event_queue(queue);
            return 0;
        }
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER || ev.keyboard.keycode == ALLEGRO_KEY_PAD_ENTER) {
                started = true;
                break;
            }
        }
        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            started = true;
            break;
        }

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            al_clear_to_color(al_map_rgb(0, 0, 0));
            al_draw_scaled_bitmap(instrucoes_img, 0, 0,
                al_get_bitmap_width(instrucoes_img), al_get_bitmap_height(instrucoes_img),
                0, 0, SCREEN_W, SCREEN_H, 0);
            al_flip_display();
        }
    }

    al_clear_to_color(al_map_rgb(12, 12, 12));
    al_draw_text(font, al_map_rgb(255, 215, 0), SCREEN_W / 2, SCREEN_H / 2 - 10, ALLEGRO_ALIGN_CENTRE, "Fase 1 começando automaticamente...");
    al_flip_display();
    al_rest(1.5);

    // loop pelas fases se o usuário não conseguir completar a fase
    for (int phase_idx = 0; phase_idx < 3; ++phase_idx) {
        bool retry_phase = true;

        while (retry_phase) {
            retry_phase = false;

            const char* image_path = phases[phase_idx].filename;
            int TIME_LIMIT = phases[phase_idx].time_limit_sec;

            ALLEGRO_BITMAP* source = al_load_bitmap(image_path);
            if (!source) {
                char err[256];
                snprintf(err, sizeof(err), "Falha ao carregar: %s", image_path);
                al_show_native_message_box(display, "Erro", err, NULL, NULL, 0);
                return -1;
            }

            // foto de referência
            const int ref_width = 320;
            int src_w = al_get_bitmap_width(source);
            int src_h = al_get_bitmap_height(source);
            float ref_scale = (float)ref_width / (float)src_w;
            int ref_height = (int)(src_h * ref_scale + 0.5f);

            // grid 5x5
            const int rows = 5, cols = 5;
            const int total = rows * cols;
            int piece_w = src_w / cols;
            int piece_h = src_h / rows;

            // grid ocupa metade da largura para deixar espaço para foto de referência
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
            const float SNAP_DIST_NORMAL = 36.0f;
            const float SNAP_DIST_FINAL = 4.0f;

            // aloca e cria as peças do jogo
            Piece* pieces = (Piece*)malloc(sizeof(Piece) * total);
            if (!pieces) {
                al_show_native_message_box(display, "Erro", "Memória insuficiente", NULL, NULL, 0);
                return -1;
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

            // embaralha posições iniciais
            srand((unsigned)time(NULL) + phase_idx * 11);
            shuffle_positions_outside(pieces, total, piece_w_s, grid_x, grid_y, grid_total_h);

            int grabbed = -1;
            float grab_off_x = 0.0f, grab_off_y = 0.0f;
            int victory_flag = 0;
            float elapsed_time = 0.0f;
            bool phase_running = true;

            while (phase_running) {
                ALLEGRO_EVENT ev;
                al_wait_for_event(queue, &ev);

                if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) return 0;

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

                    al_draw_scaled_bitmap(fundo, 0, 0,
                        al_get_bitmap_width(fundo), al_get_bitmap_height(fundo),
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
        }

        if (phase_idx <= 1) {
            al_clear_to_color(al_map_rgb(12, 12, 12));
            char tx[128]; snprintf(tx, sizeof(tx), "Preparando fase %d...", phase_idx + 2);
            al_draw_text(font, al_map_rgb(255, 215, 0), SCREEN_W / 2, SCREEN_H / 2, ALLEGRO_ALIGN_CENTRE, tx);
            al_flip_display();
            al_rest(1.5);
        }
        else {
            al_clear_to_color(al_map_rgb(12, 12, 12));
            al_draw_text(font, al_map_rgb(255, 255, 255), SCREEN_W / 2, SCREEN_H / 2, ALLEGRO_ALIGN_CENTRE, "Parabéns! Você concluiu todas as fases!");
            al_flip_display();
            al_rest(3.0);
        }
    }

    bool waiting_close = true;
    while (waiting_close) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE || ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
            waiting_close = false;
    }

    al_destroy_bitmap(instrucoes_img);
    al_destroy_bitmap(fundo);
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    return 0;
}