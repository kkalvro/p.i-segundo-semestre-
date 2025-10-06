// Quebra-cabeça Allegro C - Corrigido 100% (debug error resolvido)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <direct.h>
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
    ALLEGRO_BITMAP* bmp;
    float x, y;
    float target_x, target_y;
    int placed;
    int w, h;
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
    if (_getcwd(cwd, sizeof(cwd))) printf("Diretório atual: %s\n", cwd);

    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);
    if (!display) { fprintf(stderr, "Falha ao criar display\n"); return -1; }

    const char* image_path = "assets/exemplo_arte_1.jpg";
    ALLEGRO_BITMAP* source = al_load_bitmap(image_path);
    if (!source) {
        al_show_native_message_box(display, "Erro", "Falha ao carregar imagem. Verifique a pasta assets.", NULL, NULL, 0);
        al_destroy_display(display);
        return -1;
    }

    const int rows = 5;
    const int cols = 5;
    const int total = rows * cols;

    int src_w = al_get_bitmap_width(source);
    int src_h = al_get_bitmap_height(source);

    float maxGridW = SCREEN_W * 0.65f;
    float maxGridH = SCREEN_H * 0.65f;
    float scaleX = maxGridW / (float)src_w;
    float scaleY = maxGridH / (float)src_h;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    if (scale > 1.0f) scale = 1.0f;

    int piece_w = src_w / cols;
    int piece_h = src_h / rows;
    int piece_w_s = (int)(piece_w * scale + 0.5f);
    int piece_h_s = (int)(piece_h * scale + 0.5f);

    int grid_total_w = cols * piece_w_s + (cols - 1) * 2;
    int grid_total_h = rows * piece_h_s + (rows - 1) * 2;
    int grid_x = (SCREEN_W - grid_total_w) / 2;
    int grid_y = (SCREEN_H - grid_total_h) / 2 - 20;

    const int gap = 2;
    const float SNAP_DIST_NORMAL = 36.0f;
    const float SNAP_DIST_FINAL = 4.0f;

    Piece* pieces = (Piece*)malloc(sizeof(Piece) * total);
    if (!pieces) {
        al_show_native_message_box(display, "Erro", "Memória insuficiente", NULL, NULL, 0);
        al_destroy_bitmap(source);
        al_destroy_display(display);
        return -1;
    }

    int idx = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            ALLEGRO_BITMAP* sub = al_create_sub_bitmap(source, c * piece_w, r * piece_h, piece_w, piece_h);
            pieces[idx].bmp = sub;
            pieces[idx].w = piece_w;
            pieces[idx].h = piece_h;
            pieces[idx].target_x = grid_x + c * (piece_w_s + gap);
            pieces[idx].target_y = grid_y + r * (piece_h_s + gap);
            pieces[idx].placed = 0;
            idx++;
        }
    }

    srand((unsigned)time(NULL));
    for (int i = 0; i < total; i++) {
        pieces[i].x = rand() % (SCREEN_W - piece_w_s);
        pieces[i].y = (SCREEN_H - piece_h_s - 60) + rand() % 40;
    }

    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* font = al_load_ttf_font("assets/arial.ttf", 20, 0);
    if (!font) font = al_create_builtin_font();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);

    int running = 1, grabbed = -1;
    float grab_off_x = 0, grab_off_y = 0;
    int snap_mode = 1;
    int victory_flag = 0;
    int message_shown = 0; // nova flag para evitar multiple message box

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) running = 0;

        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            al_clear_to_color(al_map_rgb(20, 20, 20));

            for (int i = 0; i < total; i++)
                al_draw_rectangle(pieces[i].target_x - 1, pieces[i].target_y - 1,
                    pieces[i].target_x + piece_w_s + 1, pieces[i].target_y + piece_h_s + 1,
                    al_map_rgb(80, 80, 80), 1);

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

            if (font) {
                if (snap_mode == 1)
                    al_draw_text(font, al_map_rgb(255, 255, 255), 12, 8, 0, "Modo: NORMAL (F para modo difícil)");
                else
                    al_draw_text(font, al_map_rgb(255, 255, 255), 12, 8, 0, "Modo: DIFÍCIL (F para voltar ao modo normal)");
                    al_draw_text(font, al_map_rgb(255, 255, 255), 12, 56, 0, "R: Reembaralhar");
                    al_draw_text(font, al_map_rgb(255, 255, 255), 12, 32, 0, "ESC: Sair");
                
            }

            // Verifica vitória
            if (!victory_flag) {
                int win = 1;
                for (int i = 0; i < total; i++)
                    if (!pieces[i].placed) { win = 0; break; }
                if (win) victory_flag = 1;
            }

            // Desenha mensagem de vitória
            if (victory_flag) {
                al_draw_filled_rectangle(SCREEN_W / 2 - 260, SCREEN_H / 2 - 40,
                    SCREEN_W / 2 + 260, SCREEN_H / 2 + 40, al_map_rgba(0, 0, 0, 200));
                al_draw_text(font, al_map_rgb(255, 215, 0), SCREEN_W / 2, SCREEN_H / 2 - 12,
                    ALLEGRO_ALIGN_CENTRE, "Parabéns! Quebra-cabeça montado!");
            }

            al_flip_display();
        }

        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            float mx = ev.mouse.x, my = ev.mouse.y;
            for (int i = total - 1; i >= 0; i--) {
                if (pieces[i].placed) continue;
                if (mx >= pieces[i].x && mx <= pieces[i].x + piece_w_s &&
                    my >= pieces[i].y && my <= pieces[i].y + piece_h_s) {
                    grabbed = i;
                    grab_off_x = mx - pieces[i].x;
                    grab_off_y = my - pieces[i].y;
                    break;
                }
            }
        }

        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
            if (grabbed >= 0) {
                float mx = ev.mouse.x, my = ev.mouse.y;
                pieces[grabbed].x = mx - grab_off_x;
                pieces[grabbed].y = my - grab_off_y;

                float dx = pieces[grabbed].x - pieces[grabbed].target_x;
                float dy = pieces[grabbed].y - pieces[grabbed].target_y;
                float dist = sqrtf(dx * dx + dy * dy);

                float snap_dist = (snap_mode == 1) ? SNAP_DIST_NORMAL : SNAP_DIST_FINAL;
                if (dist <= snap_dist) {
                    pieces[grabbed].placed = 1;
                    pieces[grabbed].x = pieces[grabbed].target_x;
                    pieces[grabbed].y = pieces[grabbed].target_y;
                }

                grabbed = -1;

                // Mostrar mensagem fora do render
                if (victory_flag && !message_shown) {
                    al_show_native_message_box(display, "VITÓRIA", "Parabéns! Quebra-cabeça montado!", NULL, NULL, 0);
                    message_shown = 1;
                }
            }
        }

        else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES && grabbed >= 0) {
            pieces[grabbed].x = ev.mouse.x - grab_off_x;
            pieces[grabbed].y = ev.mouse.y - grab_off_y;
        }

        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) running = 0;
            else if (ev.keyboard.keycode == ALLEGRO_KEY_F) snap_mode = (snap_mode == 1) ? 2 : 1;
            else if (ev.keyboard.keycode == ALLEGRO_KEY_R) {
                for (int i = 0; i < total; i++) {
                    pieces[i].placed = 0;
                    pieces[i].x = rand() % (SCREEN_W - piece_w_s);
                    pieces[i].y = (SCREEN_H - piece_h_s - 60) + rand() % 40;
                }
                victory_flag = 0;
                message_shown = 0;
            }
        }
    }

    for (int i = 0; i < total; i++) if (pieces[i].bmp) al_destroy_bitmap(pieces[i].bmp);
    free(pieces);
    al_destroy_bitmap(source);
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    return 0;
}
