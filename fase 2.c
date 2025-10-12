#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LINHAS 4
#define COLUNAS 4
#define LARGURA_CARTA 100
#define ALTURA_CARTA 100
#define ESPACO 10
#define AREA_LARGURA (COLUNAS * (LARGURA_CARTA + ESPACO) + ESPACO)
#define AREA_ALTURA (LINHAS * (ALTURA_CARTA + ESPACO) + ESPACO)
#define TELA_LARGURA 1280
#define TELA_ALTURA 720
#define TEMPO_ESPERA 1.0

typedef struct {
    int id;
    int virada;
    int encontrada;
    ALLEGRO_BITMAP* imagem;
} Carta;

void embaralhar(int* v, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = v[i];
        v[i] = v[j];
        v[j] = temp;
    }
}

int main() {
    if (!al_init()) {
        fprintf(stderr, "Erro ao iniciar Allegro.\n");
        return -1;
    }

    al_init_primitives_addon();
    al_install_mouse();
    al_init_font_addon();
    al_init_image_addon();

    ALLEGRO_DISPLAY* janela = al_create_display(TELA_LARGURA, TELA_ALTURA);
    if (!janela) {
        fprintf(stderr, "Erro ao criar janela.\n");
        return -1;
    }

    ALLEGRO_FONT* fonte = al_create_builtin_font();
    if (!fonte) {
        fprintf(stderr, "Erro ao criar fonte.\n");
        return -1;
    }

    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60);

    al_register_event_source(fila, al_get_display_event_source(janela));
    al_register_event_source(fila, al_get_mouse_event_source());
    al_register_event_source(fila, al_get_timer_event_source(timer));

    srand(time(NULL));

    ALLEGRO_BITMAP* background = al_load_bitmap("assets/fundo.jpg");
    if (!background) {
        fprintf(stderr, "Erro ao carregar imagem de fundo\n");
        return -1;
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

    int total_cartas = LINHAS * COLUNAS;
    int ids[LINHAS * COLUNAS];

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
    embaralhar(ids, total_cartas);

    Carta cartas[LINHAS][COLUNAS];
    int idx = 0;
    for (int i = 0; i < LINHAS; i++) {
        for (int j = 0; j < COLUNAS; j++) {
            cartas[i][j].id = ids[idx++];
            cartas[i][j].virada = 0;
            cartas[i][j].encontrada = 0;

            int img_index = cartas[i][j].id;
            if (img_index > 6) img_index = 6;
            cartas[i][j].imagem = al_load_bitmap(nomes_imagens[img_index]);
            if (!cartas[i][j].imagem) {
                fprintf(stderr, "Erro ao carregar imagem %s\n", nomes_imagens[img_index]);
                return -1;
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
    int area_x = (TELA_LARGURA - AREA_LARGURA) / 2;
    int area_y = (TELA_ALTURA - AREA_ALTURA) / 2;

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            rodando = 0;

        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            if (!esperando) {
                int mx = ev.mouse.x;
                int my = ev.mouse.y;

                for (int i = 0; i < LINHAS; i++) {
                    for (int j = 0; j < COLUNAS; j++) {
                        int x = area_x + ESPACO + j * (LARGURA_CARTA + ESPACO);
                        int y = area_y + ESPACO + i * (ALTURA_CARTA + ESPACO);
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

        if (esperando && al_get_time() - tempo_virada > TEMPO_ESPERA) {
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
        al_draw_scaled_bitmap(background, 0, 0,
            al_get_bitmap_width(background), al_get_bitmap_height(background),
            0, 0, TELA_LARGURA, TELA_ALTURA, 0);

        // Moldura do tabuleiro
        al_draw_filled_rounded_rectangle(area_x - 15, area_y - 15,
            area_x + AREA_LARGURA + 15, area_y + AREA_ALTURA + 15,
            20, 20, al_map_rgb(30, 30, 30));

        al_draw_rounded_rectangle(area_x - 15, area_y - 15,
            area_x + AREA_LARGURA + 15, area_y + AREA_ALTURA + 15,
            20, 20, al_map_rgb(255, 255, 255), 3);

        for (int i = 0; i < LINHAS; i++) {
            for (int j = 0; j < COLUNAS; j++) {
                int x = area_x + ESPACO + j * (LARGURA_CARTA + ESPACO);
                int y = area_y + ESPACO + i * (ALTURA_CARTA + ESPACO);

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

        al_draw_textf(fonte, al_map_rgb(255, 255, 255),
            TELA_LARGURA / 2, TELA_ALTURA - 60, ALLEGRO_ALIGN_CENTER,
            "Pares encontrados: %d / %d", pares_encontrados, total_cartas / 2);

        if (pares_encontrados == total_cartas / 2)
            al_draw_text(fonte, al_map_rgb(255, 255, 0), TELA_LARGURA / 2, TELA_ALTURA - 30,
                ALLEGRO_ALIGN_CENTER, "🎉 Você venceu!");

        al_flip_display();
    }

    // Destruir bitmaps
    al_destroy_bitmap(background);
    for (int i = 0; i < LINHAS; i++)
        for (int j = 0; j < COLUNAS; j++)
            al_destroy_bitmap(cartas[i][j].imagem);

    al_destroy_font(fonte);
    al_destroy_display(janela);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);

    return 0;
}