#include "simon_game.h"
#include "lvgl.h"
#include <mbed.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <cstdio>

// Définir le PIN du Buzzer
#define BUZZER_PIN D11

// Initialiser le PwmOut pour le Buzzer
static PwmOut buzzer(BUZZER_PIN);

// Frequences pour les différentes tonalités du buzzer
static const float tones[] = {2000.0, 3000.0, 3500.0, 4000.0};

// couleurs des LEDs
static lv_color_t led_color1 = lv_color_hex(0xFF0000); // Red
static lv_color_t led_color2 = lv_color_hex(0x00FF00); // Green
static lv_color_t led_color3 = lv_color_hex(0x0000FF); // Blue
static lv_color_t led_color4 = lv_color_hex(0xFFFF00); // Yellow

// Variables du jeu
static std::vector<int> sequence;
static int user_input[100]; // Stock la saisie du joueur
static int sequence_length = 0;
static int user_input_length = 0;
static int score = 0; // Score du joueur
static bool is_user_turn = false;
static lv_timer_t *sequence_timer = nullptr;
static lv_timer_t *blink_timer = nullptr;
static lv_obj_t *led1;
static lv_obj_t *led2;
static lv_obj_t *led3;
static lv_obj_t *led4;
static lv_obj_t *start_btn;
static lv_obj_t *title_label;
static lv_obj_t *score_label;

// Index de la séquence de couleurs actuelle
static int current_sequence_index = 0;
static int current_led_id = -1;

// Fonction pour régler la tonalité du buzzer
static void set_buzzer_tone(float frequency) {
    if (frequency > 0) {
        buzzer.period(1.0 / frequency);
        buzzer.write(0.5); // 50% du duty cycle pour la tonalité
    } else {
        buzzer.write(0.0); // Eteint le buzzer
    }
}

// Fonction pour reset le jeu
static void reset_game(void) {
    sequence.clear();
    user_input_length = 0;
    sequence_length = 0;
    score = 0;
    is_user_turn = false;
    if (sequence_timer) {
        lv_timer_del(sequence_timer);
        sequence_timer = nullptr;
    }
    if (blink_timer) {
        lv_timer_del(blink_timer);
        blink_timer = nullptr;
    }
}

// Fonction pour générer une nouvelle séquence
static void generate_sequence(void) {
    sequence.push_back(rand() % 4);
    sequence_length = sequence.size();
    printf("Generated sequence: ");
    for (int i : sequence) {
        printf("%d ", i);
    }
    printf("\n");
}

// Fonction pour reset la couleur d'une LED
static void reset_led_color(int led_id) {
    switch (led_id) {
        case 0:
            lv_obj_set_style_bg_color(led1, led_color1, 0);
            break;
        case 1:
            lv_obj_set_style_bg_color(led2, led_color2, 0);
            break;
        case 2:
            lv_obj_set_style_bg_color(led3, led_color3, 0);
            break;
        case 3:
            lv_obj_set_style_bg_color(led4, led_color4, 0);
            break;
    }
    set_buzzer_tone(0.0);
}

// Callback du timer pour éteindre la LED
static void turn_off_led(lv_timer_t *timer) {
    int led_id = (int)(intptr_t)timer->user_data;
    reset_led_color(led_id);
    lv_timer_del(timer);
}

// Fonction pour allumer une LED et lire la tonalité correspondante
static void light_led_and_play_sound(int led_id) {
    printf("Lighting LED %d\n", led_id);

    switch (led_id) {
        case 0:
            lv_obj_set_style_bg_color(led1, lv_color_lighten(led_color1, LV_OPA_70), 0);
            set_buzzer_tone(tones[0]);
            break;
        case 1:
            lv_obj_set_style_bg_color(led2, lv_color_lighten(led_color2, LV_OPA_70), 0);
            set_buzzer_tone(tones[1]);
            break;
        case 2:
            lv_obj_set_style_bg_color(led3, lv_color_lighten(led_color3, LV_OPA_70), 0);
            set_buzzer_tone(tones[2]);
            break;
        case 3:
            lv_obj_set_style_bg_color(led4, lv_color_lighten(led_color4, LV_OPA_70), 0);
            set_buzzer_tone(tones[3]);
            break;
    }

    // Créé un timer pour éteindre la LED après 500ms
    blink_timer = lv_timer_create(turn_off_led, 500, (void*)(intptr_t)led_id);
}

// Callback du timer pour lire la séquence de LEDs
static void play_sequence_step(lv_timer_t *timer) {
    if (current_sequence_index < sequence_length) {
        int led_id = sequence[current_sequence_index];
        light_led_and_play_sound(led_id);
        current_sequence_index++;
    } else {
        lv_timer_del(timer);
        sequence_timer = nullptr;
        is_user_turn = true;
        user_input_length = 0;
        printf("User's turn to play\n");
    }
}

// Fonction pour démarrer la séquence
static void play_sequence(void) {
    current_sequence_index = 0;
    printf("Starting sequence\n");
    sequence_timer = lv_timer_create(play_sequence_step, 1000, NULL); // Create the timer without storing it
}

// Fonction pour vérifier la saisie du joueur
static void check_user_input(void) {
    if (user_input_length == sequence_length) {
        is_user_turn = false;
        score++; // Incrémente le score
        char score_text[16];
        sprintf(score_text, "Score: %d", score);
        lv_label_set_text(score_label, score_text); // Met à jour le label du score

        ThisThread::sleep_for(500ms);
        generate_sequence();
        play_sequence();
    }
}

// Gestion d'évènements pour les LEDs
static void led_event_handler(lv_event_t *e) {
    if (!is_user_turn) return;

    lv_event_code_t code = lv_event_get_code(e);
    int led_id = (int)(intptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        printf("LED %d clicked\n", led_id);
        light_led_and_play_sound(led_id);

        user_input[user_input_length++] = led_id;
        check_user_input();
    }
}

// Gestion d'évènements pour le bouton Start
static void start_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        // Enlève le start button and le title label
        lv_obj_del(title_label);
        lv_obj_del(start_btn);

        // Lance le jeu
        start_game();
    }
}

// Fonction pour initialiser le menu d'accueil
static void create_start_screen(void) {
    // Créé le label title
    title_label = lv_label_create(lv_scr_act());
    lv_label_set_text(title_label, "Simon");
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, -50);

    // Créé le label start
    start_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(start_btn, 100, 50);
    lv_obj_align(start_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_t *label = lv_label_create(start_btn);
    lv_label_set_text(label, "Start");
    lv_obj_center(label);
    lv_obj_add_event_cb(start_btn, start_button_event_handler, LV_EVENT_CLICKED, NULL);
}

// Fonction pour démarrer le jeu
void start_game(void) {
    // Créé le label score
    score_label = lv_label_create(lv_scr_act());
    lv_label_set_text(score_label, "Score: 0");
    lv_obj_align(score_label, LV_ALIGN_TOP_LEFT, 10, 10);

    // Créé les 4 LEDs
    led1 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(led1, 100, 100);
    lv_obj_align(led1, LV_ALIGN_CENTER, -75, -75);
    lv_obj_set_style_bg_color(led1, led_color1, 0);
    lv_obj_add_event_cb(led1, led_event_handler, LV_EVENT_CLICKED, (void*)(intptr_t)0);

    led2 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(led2, 100, 100);
    lv_obj_align(led2, LV_ALIGN_CENTER, 75, -75);
    lv_obj_set_style_bg_color(led2, led_color2, 0);
    lv_obj_add_event_cb(led2, led_event_handler, LV_EVENT_CLICKED, (void*)(intptr_t)1);

    led3 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(led3, 100, 100);
    lv_obj_align(led3, LV_ALIGN_CENTER, -75, 75);
    lv_obj_set_style_bg_color(led3, led_color3, 0);
    lv_obj_add_event_cb(led3, led_event_handler, LV_EVENT_CLICKED, (void*)(intptr_t)2);

    led4 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(led4, 100, 100);
    lv_obj_align(led4, LV_ALIGN_CENTER, 75, 75);
    lv_obj_set_style_bg_color(led4, led_color4, 0);
    lv_obj_add_event_cb(led4, led_event_handler, LV_EVENT_CLICKED, (void*)(intptr_t)3);

    reset_game();
    srand(time(NULL)); // Génère un nombre aléatoire
    generate_sequence();
    play_sequence();
}

// Fonction pour lancer l'application
void app_main(void) {
    // Initialise le menu
    create_start_screen();
}
