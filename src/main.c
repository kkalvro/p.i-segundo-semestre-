#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_image.h>

int main() {
    if (!al_init()) {
        printf("Erro na inicializacao do programa");
        return -1;
    }

    ALLEGRO_DISPLAY *janela;
    janela = al_create_display(1920, 1080);
    al_set_window_title(janela, "Aline no País das Artes");
    if (!janela) {
        printf("Erro na criacao da janela");
        return -1;
    }

    al_clear_to_color(al_map_rgb(200, 230, 130));
    al_flip_display();

    al_rest(2.0);
    al_destroy_display(janela);
    return 0;
}