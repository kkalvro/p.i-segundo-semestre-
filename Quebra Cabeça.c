// Programa.c
// Quebra-cabeça melhorado:
// - grid centralizado e escalado
// - tecla F para "fase final" (snap desativado)
// - mensagem de conclusão visível (texto + caixa nativa)

// Coloque a imagem em: <pasta-do-projeto>/assets/exemplo_arte_1.jpg

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <direct.h> // _getcwd no Windows
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_native_dialog.h>

#define SCREEN_W 1024
#define SCREEN_H 700
#define FPS 60

typedef struct {
    ALLEGRO_BITMAP* bmp;   // bitmap original da peça (sem escala)
    float x, y;            // posição atual (em tela, já com escala aplicada)
    float target_x, target_y; // posição alvo (em tela, já com escala)
    int row, col;
    int placed;
    int w, h;              // largura/altura originais da peça (px)
} Piece;

int main(void) {
    if (!al_init()) { fprintf(stderr, "Falha ao inicializar Allegro\n"); return -1; }
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_mouse();
    al_install_keyboard();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_native_dialog_addon();

    char cwd[1024];
    if (_getcwd(cwd, sizeof(cwd))) printf("Working directory: %s\n", cwd);

    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    if (!display) { fprintf(stderr, "Falha ao criar display\n"); return -1; }

    const char* image_path = "assets/exemplo_arte_1.jpg";
    ALLEGRO_BITMAP* source = al_load_bitmap(image_path);
    if (!source) {
        char msg[1024];
        snprintf(msg, sizeof(msg),
            "Falha ao carregar imagem: %s\nVerifique se a pasta assets contem o arquivo e se foi copiado para Debug/Release",
            image_path);
        al_show_native_message_box(display, "Erro: imagem não encontrada", msg, NULL, NULL, 0);
        al_destroy_display(display);
        return -1;
    }

    /* configurações do puzzle */
    const int rows = 3;
    const int cols = 3;
    const int total = rows * cols;

    int src_w = al_get_bitmap_width(source);
    int src_h = al_get_bitmap_height(source);
    if (src_w <= 0 || src_h <= 0) {
        al_show_native_message_box(display, "Erro: imagem inválida", "A imagem carregada tem largura/altura inválida.", NULL, NULL, 0);
        al_destroy_bitmap(source);
        al_destroy_display(display);
        return -1;
    }

    /* ESCALA E POSIÇÃO: queremos o grid maior e centralizado.
       Calculamos uma escala para que a imagem ocupe até 60% da largura/altura da tela. */
    float maxGridW = SCREEN_W * 0.60f;
    float maxGridH = SCREEN_H * 0.60f;
    float scaleX = maxGridW / (float)src_w;
    float scaleY = maxGridH / (float)src_h;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    if (scale > 1.0f) scale = 1.0f; // não aumentar além do original

    /* dimensão da peça original e escalada */
    int piece_w = src_w / cols;
    int piece_h = src_h / rows;
    int piece_w_s = (int)(piece_w * scale + 0.5f);
    int piece_h_s = (int)(piece_h * scale + 0.5f);

    if (piece_w <= 0 || piece_h <= 0 || piece_w_s <= 0 || piece_h_s <= 0) {
        al_show_native_message_box(display, "Erro", "Imagem muito pequena ou rows/cols muito grandes.", NULL, NULL, 0);
        al_destroy_bitmap(source);
        al_destroy_display(display);
        return -1;
    }

    /* calcula grid total e posiciona centralizado */
    int grid_total_w = cols * piece_w_s + (cols - 1) * 2;
    int grid_total_h = rows * piece_h_s + (rows - 1) * 2;
    int grid_x = (SCREEN_W - grid_total_w) / 2;
    int grid_y = (SCREEN_H - grid_total_h) / 2 - 20; // um pouco para cima

    const int gap = 2;
    const float SNAP_DIST_NORMAL = 36.0f; /* distância de snap no modo normal */
    const float SNAP_DIST_FINAL = 4.0f;   /* quase exigência exata no modo final */

    /* aloca peças */
    Piece* pieces = (Piece*)malloc(sizeof(Piece) * total);
    if (!pieces) {
        al_show_native_message_box(display, "Erro", "Memória insuficiente", NULL, NULL, 0);
        al_destroy_bitmap(source); al_destroy_display(display); return -1;
    }

    /* cria sub-bitmaps (ou fallback) e define target positions (em coords de tela scaled) */
    int idx = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int sx = c * piece_w;
            int sy = r * piece_h;
            ALLEGRO_BITMAP* sub = al_create_sub_bitmap(source, sx, sy, piece_w, piece_h);
            if (!sub) { /* fallback: criar bitmap e copiar região */
                sub = al_create_bitmap(piece_w, piece_h);
                if (sub) {
                    ALLEGRO_BITMAP* old = al_get_target_bitmap();
                    al_set_target_bitmap(sub);
                    al_draw_bitmap_region(source, sx, sy, piece_w, piece_h, 0, 0, 0);
                    al_set_target_bitmap(old);
                }
                else {
                    /* erro crítico */
                    for (int j = 0; j < idx; j++) if (pieces[j].bmp) al_destroy_bitmap(pieces[j].bmp);
                    free(pieces); al_destroy_bitmap(source); al_destroy_display(display);
                    al_show_native_message_box(display, "Erro", "Falha ao criar bitmaps das peças", NULL, NULL, 0);
                    return -1;
                }
            }
            pieces[idx].bmp = sub;
            pieces[idx].row = r; pieces[idx].col = c;
            pieces[idx].w = piece_w; pieces[idx].h = piece_h;
            /* target já em coordenadas de tela (escaladas) */
            pieces[idx].target_x = grid_x + c * (piece_w_s + gap);
            pieces[idx].target_y = grid_y + r * (piece_h_s + gap);
            pieces[idx].placed = 0;
            idx++;
        }
    }

    /* Embaralhar: posicionar fora do grid (acima/abaixo/esquerda/direita).
       Colocamos à volta para ficar visualmente organizado. */
    srand((unsigned)time(NULL));
    int areaX = 20;
    int areaY = SCREEN_H - piece_h_s - 40; // linha inferior para embaralhar
    for (int i = 0; i < total; i++) {
        int maxX = SCREEN_W - areaX - piece_w_s;
        if (maxX < 0) maxX = 0;
        pieces[i].x = areaX + (rand() % (maxX + 1));
        pieces[i].y = areaY + (rand() % 20); // leve variação vertical
    }

    /* eventos, timer, fonte */
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* font = al_load_ttf_font("assets/arial.ttf", 20, 0);
    if (!font) {
        /* tenta criar fonte built-in como fallback */
        font = al_create_builtin_font();
    }

    if (!queue || !timer) {
        al_show_native_message_box(display, "Erro", "Falha ao inicializar eventos/timer", NULL, NULL, 0);
        goto cleanup;
    }

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);

    /* variáveis para drag */
    int running = 1;
    int grabbed = -1;
    float grab_offset_x = 0.0f, grab_offset_y = 0.0f;

    /* modo de snap: 1 = normal (snap ativo), 2 = fase final (snap fraco/quase desativado) */
    int snap_mode = 1;
    int victory_shown = 0;

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            running = 0;
        }
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            /* desenhar cena */
            al_clear_to_color(al_map_rgb(20, 20, 20));

            /* desenhar guias do grid (retângulos vazios) */
            for (int i = 0; i < total; i++) {
                al_draw_rectangle(pieces[i].target_x - 1, pieces[i].target_y - 1,
                    pieces[i].target_x + piece_w_s + 1, pieces[i].target_y + piece_h_s + 1,
                    al_map_rgb(80, 80, 80), 1);
            }

            /* desenhar peças encaixadas (fixas no target) */
            for (int i = 0; i < total; i++) {
                if (pieces[i].placed) {
                    al_draw_scaled_bitmap(pieces[i].bmp, 0, 0, pieces[i].w, pieces[i].h,
                        pieces[i].target_x, pieces[i].target_y,
                        piece_w_s, piece_h_s, 0);
                }
            }

            /* desenhar peças não encaixadas (exceto a arrastada) */
            for (int i = 0; i < total; i++) {
                if (!pieces[i].placed && i != grabbed) {
                    al_draw_scaled_bitmap(pieces[i].bmp, 0, 0, pieces[i].w, pieces[i].h,
                        pieces[i].x, pieces[i].y, piece_w_s, piece_h_s, 0);
                }
            }

            /* desenhar peça sendo arrastada por cima */
            if (grabbed >= 0) {
                al_draw_scaled_bitmap(pieces[grabbed].bmp, 0, 0, pieces[grabbed].w, pieces[grabbed].h,
                    pieces[grabbed].x, pieces[grabbed].y, piece_w_s, piece_h_s, 0);
            }

            /* indicador de modo (normal / final) */
            if (font) {
                if (snap_mode == 1) {
                    al_draw_text(font, al_map_rgb(200, 200, 200), 12, 8, 0, "Modo: NORMAL (F para modo final)");
                }
                else {
                    al_draw_text(font, al_map_rgb(200, 200, 200), 12, 8, 0, "Modo: FASE FINAL - SNAP DESATIVADO (F alterna)");
                }
                al_draw_textf(font, al_map_rgb(200, 200, 200), 12, 30, 0, "Teclas: R=Reembaralhar  F=AlternaModo  ESC=Sair");
            }

            /* verificar vitória */
            int win = 1;
            for (int i = 0; i < total; i++) if (!pieces[i].placed) { win = 0; break; }

            if (win) {
                /* mostrar mensagem de vitória no centro da tela */
                const char* msg = "Parabéns! Quebra-cabeça montado!";
                if (font) {
                    al_draw_filled_rectangle(SCREEN_W / 2 - 260, SCREEN_H / 2 - 40, SCREEN_W / 2 + 260, SCREEN_H / 2 + 40, al_map_rgba(0, 0, 0, 200));
                    al_draw_rectangle(SCREEN_W / 2 - 260, SCREEN_H / 2 - 40, SCREEN_W / 2 + 260, SCREEN_H / 2 + 40, al_map_rgb(255, 215, 0), 2);
                    al_draw_text(font, al_map_rgb(255, 215, 0), SCREEN_W / 2, SCREEN_H / 2 - 12, ALLEGRO_ALIGN_CENTRE, msg);
                }
                if (!victory_shown) {
                    al_show_native_message_box(display, "VITÓRIA", "Você montou o quebra-cabeça!", NULL, NULL, 0);
                    victory_shown = 1;
                }
            }

            al_flip_display();

        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            ALLEGRO_MOUSE_STATE ms; al_get_mouse_state(&ms);
            float mx = (float)ms.x, my = (float)ms.y;

            /* iterar do topo pra baixo */
            for (int i = total - 1; i >= 0; i--) {
                if (pieces[i].placed) continue;
                if (mx >= pieces[i].x && mx <= pieces[i].x + piece_w_s &&
                    my >= pieces[i].y && my <= pieces[i].y + piece_h_s) {
                    grabbed = i;
                    grab_offset_x = mx - pieces[i].x;
                    grab_offset_y = my - pieces[i].y;
                    /* trazer peça para topo do array */
                    Piece tmp = pieces[i];
                    for (int j = i; j < total - 1; j++) pieces[j] = pieces[j + 1];
                    pieces[total - 1] = tmp;
                    grabbed = total - 1;
                    break;
                }
            }

        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
            if (grabbed >= 0) {
                ALLEGRO_MOUSE_STATE ms; al_get_mouse_state(&ms);
                float mx = (float)ms.x, my = (float)ms.y;
                /* atualiza posição para onde soltou */
                pieces[grabbed].x = mx - grab_offset_x;
                pieces[grabbed].y = my - grab_offset_y;

                /* calcular distância do centro do target */
                float dx = pieces[grabbed].x - pieces[grabbed].target_x;
                float dy = pieces[grabbed].y - pieces[grabbed].target_y;
                float dist = sqrtf(dx * dx + dy * dy);

                /* decidir snap dependendo do modo */
                float snap_dist = (snap_mode == 1) ? SNAP_DIST_NORMAL : SNAP_DIST_FINAL;
                if (dist <= snap_dist) {
                    /* encaixa */
                    pieces[grabbed].placed = 1;
                    pieces[grabbed].x = pieces[grabbed].target_x;
                    pieces[grabbed].y = pieces[grabbed].target_y;
                }
                else {
                    /* NÃO forçar volta — a peça fica onde soltar (comportamento pedido) */
                }
                grabbed = -1;
            }

        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
            if (grabbed >= 0) {
                pieces[grabbed].x = ev.mouse.x - grab_offset_x;
                pieces[grabbed].y = ev.mouse.y - grab_offset_y;
            }

        }
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                running = 0;
            }
            else if (ev.keyboard.keycode == ALLEGRO_KEY_R) {
                /* reembaralhar e resetar placed */
                for (int i = 0; i < total; i++) {
                    pieces[i].placed = 0;
                    int maxX = SCREEN_W - 20 - piece_w_s;
                    if (maxX < 0) maxX = 0;
                    pieces[i].x = 20 + (rand() % (maxX + 1));
                    pieces[i].y = SCREEN_H - piece_h_s - 40 + (rand() % 20);
                }
                victory_shown = 0;
            }
            else if (ev.keyboard.keycode == ALLEGRO_KEY_F) {
                /* alterna modo final / normal */
                snap_mode = (snap_mode == 1) ? 2 : 1;
            }
        }
    }

cleanup:
    if (pieces) {
        for (int i = 0; i < total; i++) if (pieces[i].bmp) al_destroy_bitmap(pieces[i].bmp);
        free(pieces);
    }
    if (source) al_destroy_bitmap(source);
    if (font) al_destroy_font(font);
    if (timer) al_destroy_timer(timer);
    if (queue) al_destroy_event_queue(queue);
    if (display) al_destroy_display(display);

    return 0;
}
