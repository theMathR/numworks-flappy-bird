#include "eadk.h"
#include "storage.h"
#include <math.h>
#include <string.h>
#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Flappy Bird";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

const eadk_color_t blue_sky = 0x4df9;
const eadk_color_t white_clouds = 0xeffb;
const eadk_color_t green_plants = 0x5f0e;
const eadk_color_t transparent_color = 0x1234;

INCBIN(eadk_color_t, spr_ground, "spritesbin/groundsegment.bin");
const int spr_ground_width = 24;
const int spr_ground_height = 30;

INCBIN(eadk_color_t, spr_parallax_bg_0 , "spritesbin/clouds.bin");
INCBIN(eadk_color_t, spr_parallax_bg_1 , "spritesbin/city_back.bin");
INCBIN(eadk_color_t, spr_parallax_bg_2 , "spritesbin/city.bin");
INCBIN(eadk_color_t, spr_parallax_bg_3 , "spritesbin/trees.bin");
typedef struct {
    const eadk_color_t* sprite;
    const int width;
    const int height;
    const int y;
} parallax_bg_t;
#define bg_layer_count 4
const parallax_bg_t parallax_bgs[bg_layer_count] = {
    {spr_parallax_bg_0_data, 284, 22, 130},
    {spr_parallax_bg_1_data, 60, 45, 150},
    {spr_parallax_bg_2_data, 68, 45, 150},
    {spr_parallax_bg_3_data, 284, 18, 180},
};

INCBIN(eadk_color_t, spr_bird_a, "spritesbin/yellowbird-upflap.bin");
INCBIN(eadk_color_t, spr_bird_b, "spritesbin/yellowbird-midflap.bin");
INCBIN(eadk_color_t, spr_bird_c, "spritesbin/yellowbird-downflap.bin");
const eadk_color_t* spr_bird_frames[] = {spr_bird_a_data, spr_bird_b_data, spr_bird_c_data, spr_bird_b_data};
const int spr_bird_width = 34;
const int spr_bird_height = 24;
const int bird_draw_size = 40;

INCBIN(eadk_color_t, spr_pipe_head, "spritesbin/pipe_head.bin");
INCBIN(eadk_color_t, spr_pipe_segment, "spritesbin/pipe_segment.bin");
const int spr_pipe_head_height = 26;
const int spr_pipe_width = 52;

INCBIN(eadk_color_t, spr_digit_0, "spritesbin/0.bin");
INCBIN(eadk_color_t, spr_digit_1, "spritesbin/1.bin");
INCBIN(eadk_color_t, spr_digit_2, "spritesbin/2.bin");
INCBIN(eadk_color_t, spr_digit_3, "spritesbin/3.bin");
INCBIN(eadk_color_t, spr_digit_4, "spritesbin/4.bin");
INCBIN(eadk_color_t, spr_digit_5, "spritesbin/5.bin");
INCBIN(eadk_color_t, spr_digit_6, "spritesbin/6.bin");
INCBIN(eadk_color_t, spr_digit_7, "spritesbin/7.bin");
INCBIN(eadk_color_t, spr_digit_8, "spritesbin/8.bin");
INCBIN(eadk_color_t, spr_digit_9, "spritesbin/9.bin");
const eadk_color_t* spr_digits[] = {
    spr_digit_0_data, spr_digit_1_data, spr_digit_2_data, spr_digit_3_data, spr_digit_4_data,
    spr_digit_5_data, spr_digit_6_data, spr_digit_7_data, spr_digit_8_data, spr_digit_9_data,
};
const int spr_digit_width = 24;
const int spr_digit_height = 36;
const int spr_digit_1_width = 16;
const int digit_y = 30;

INCBIN(eadk_color_t, spr_title, "spritesbin/title.bin");
const int spr_title_width = 178;
const int spr_title_height = 48;

INCBIN(eadk_color_t, spr_ready, "spritesbin/ready.bin");
const int spr_ready_width = 184;
const int spr_ready_height = 50;
const int ready_y = 130;

int blink = 0;

double scroll = 0;
int score = 0;

typedef struct {
    int high_score;
    double high_score_speed;
    double scroll_speed;
} save_t;
save_t save = {0, 0., 1.5};

const char* save_name = "flappy.score";
size_t save_size = sizeof(save);
union save_reader_t {
    char raw_data[sizeof(save)];
    save_t save;
};

const int bird_x = 50;
int bird_y;
double bird_dy;
double bird_angle = 0;
double sin_a = 0;
double cos_a = 1;
const int bird_collision_margin_x = 10;
const int bird_collision_margin_y = 10;

const eadk_color_t* spr_bird_data = spr_bird_b_data;
int bird_anim = 0;

bool press_registered = true; // True because you press OK to launch the app

const int pipe_collision_margin_x = 2;
const int pipe_collision_margin_y = 2;
const int pipe_hole_size = 80;
typedef struct {
    double x;
    int hole_y;
} pipe_t;
#define pipes_size 4
pipe_t pipes[pipes_size];
const int space_between_pipes = EADK_SCREEN_WIDTH/(pipes_size-1);

int random_pipe_hole_height() {
    return eadk_random() % (EADK_SCREEN_HEIGHT - pipe_hole_size - spr_ground_height);
}

int min(int a, int b) {
    return a<b? a : b;
}
int max(int a, int b) {
    return a>b? a : b;
}

void draw_sprite_line(eadk_color_t line_buffer[], int sprite_x, int y, int sprite_width, const eadk_color_t sprite[]) {
    for (int x = max(0, sprite_x); x<min(sprite_x+sprite_width, EADK_SCREEN_WIDTH); x++) {
        eadk_color_t pixel_color = sprite[x - sprite_x + y * sprite_width];
        if (pixel_color != transparent_color)
            line_buffer[x] = pixel_color;
    }
}

typedef enum {
    GAME_NOT_STARTED,
    GAME_ON,
    GAME_OVER,
} game_state_t;

game_state_t game_state = GAME_NOT_STARTED;

void end_game() {
    blink = 0;
    game_state = GAME_OVER;
    bird_dy = -8;

    // Save high score
    if (score > save.high_score) {
        save.high_score = score;
        save.high_score_speed = save.scroll_speed;
    }

    score=save.high_score;//DEBUG
};

void init_game() {
    blink = 0;
    bird_dy = -5;
    scroll = 0;
    score = 0;
    bird_y = (EADK_SCREEN_HEIGHT-spr_ground_height-bird_draw_size)/2; // Centered

    // Init pipes
    for (int i=0;i<pipes_size;i++) {
        pipes[i].x = EADK_SCREEN_WIDTH + i * space_between_pipes;
        pipes[i].hole_y = random_pipe_hole_height();
    }
}

void update_bird_fall() {
    // Animation
    if (bird_dy < 0) {
        spr_bird_data = spr_bird_frames[bird_anim%4];
        bird_anim++;
    } else spr_bird_data = spr_bird_b_data;

    // Gravity
    bird_dy += 0.5;
    bird_y += bird_dy;

    // Angle stuff
    bird_angle = (bird_angle + 3*atan(bird_dy/5.))/4; // Smoothed
    sin_a = -sin(bird_angle);
    cos_a = cos(bird_angle);
}

int main() {
    // Load high score
    if (extapp_fileExists(save_name)) {
        const char* save_raw_data = extapp_fileRead(save_name, &save_size);
        if (save_raw_data != NULL) {
            union save_reader_t save_reader;
            memcpy(save_reader.raw_data, save_raw_data, sizeof(save_reader));
            save = save_reader.save;
        }
    }

    init_game();

    while (true) {
        eadk_keyboard_state_t keyboard = eadk_keyboard_scan();

        // Game logic
        switch (game_state) {
            case GAME_NOT_STARTED:
                // Start game
                if (eadk_keyboard_key_down(keyboard, eadk_key_ok) && !press_registered) {
                    game_state = GAME_ON;
                    blink=0;
                    press_registered = true;
                }
                blink++;
                break;
            case GAME_OVER:
                update_bird_fall(); // Keep the bird falling

                // Restart game (only once the bird starts falling)
                if (bird_dy > 0 && eadk_keyboard_key_down(keyboard, eadk_key_ok) && !press_registered) {
                    init_game();
                    game_state = GAME_ON;
                    press_registered = true;
                }

                blink++;
                break;
            case GAME_ON:
                // Check bird collision
                if (bird_y+bird_collision_margin_y < 0) // Sky touched?
                    end_game();
                if (bird_y+bird_draw_size-bird_collision_margin_y > EADK_SCREEN_HEIGHT-spr_ground_height) // Ground touched?
                    end_game();
                for (int i=0;i<pipes_size;i++) { // Pipes touched?
                    pipe_t pipe = pipes[i];
                    if (
                        bird_x+bird_draw_size-bird_collision_margin_x > pipe.x+pipe_collision_margin_x &&
                        bird_x+bird_collision_margin_x < pipe.x+spr_pipe_width-pipe_collision_margin_x &&
                        !(
                            bird_y+bird_collision_margin_y > pipe.hole_y-pipe_collision_margin_y &&
                            bird_y+bird_draw_size-bird_collision_margin_y < pipe.hole_y+pipe_hole_size+pipe_collision_margin_y
                        )
                    )
                        end_game();
                }

                // Jump
                if (eadk_keyboard_key_down(keyboard, eadk_key_ok) && !press_registered) {
                    bird_dy = -5;
                    press_registered = true;
                    bird_anim = 0;
                }

                // Make bird fall
                update_bird_fall();

                // Scroll screen
                scroll+=save.scroll_speed;
                for (int i=0;i<pipes_size;i++) { // Scroll pipes
                    if (pipes[i].x+spr_pipe_width >= bird_x && pipes[i].x - save.scroll_speed + spr_pipe_width < bird_x) // Check if pipe passes the bird
                        score++;
                    pipes[i].x -= save.scroll_speed;
                    if (pipes[i].x < -space_between_pipes) { // Wrap around
                        pipes[i].x = EADK_SCREEN_WIDTH;
                        pipes[i].hole_y = random_pipe_hole_height();
                    }
                }

                break;
        }

        if (!eadk_keyboard_key_down(keyboard, eadk_key_ok)) press_registered = false;

        // Calculate some stuff to draw the score properly
        int score_digit_count = score? floor(log10(score))+1 : 1;
        int score_width = 2; // Account for the last rows
        int score_digits = score;
        for (int i=0;i<score_digit_count;i++) {
            score_width += (score_digits%10==1? spr_digit_1_width : spr_digit_width) - 2; // digit 1 has a different width + -2px margin
            score_digits /= 10; // Next digit
        }
        int score_base_x = (EADK_SCREEN_WIDTH+score_width)/2 - 2;

        // Draw everything line by line
        eadk_color_t line_buffer[EADK_SCREEN_WIDTH];
        for (int y=0;y<EADK_SCREEN_HEIGHT;y++) {
            // Draw ground
            if (y >= 240 - spr_ground_height) {
                for (int x = 0; x < EADK_SCREEN_WIDTH; x++) {
                    line_buffer[x] = spr_ground_data[((x+((int)scroll))%spr_ground_width) + (y-240+spr_ground_height)*spr_ground_width];
                }
                goto skip_drawing_above_ground;
            }

            // Draw background
            eadk_color_t bg_color = blue_sky;
            if (y >= parallax_bgs[3].y + parallax_bgs[3].height) bg_color = green_plants;
            else if (y >= parallax_bgs[0].y + parallax_bgs[0].height) bg_color = white_clouds;
            for (int x = 0; x < EADK_SCREEN_WIDTH; x++) // tried using memset and got the wrong colors???
                line_buffer[x] = bg_color;

            for (int i=0;i<bg_layer_count;i++) { // Draw layers
                parallax_bg_t layer = parallax_bgs[i];
                int layer_scroll = scroll/(bg_layer_count-i+1);
                if (y >= layer.y && y < layer.y + layer.height)
                    for (int x = 0; x < EADK_SCREEN_WIDTH; x++) {
                        eadk_color_t pixel_color = layer.sprite[((x+layer_scroll)%layer.width) + (y - layer.y)*layer.width];
                        if (pixel_color != transparent_color)
                            line_buffer[x] = pixel_color;
                    }
            }

            // Draw pipes
            for (int i=0;i<pipes_size;i++) {
                pipe_t pipe = pipes[i];
                if (y >= pipe.hole_y && y <= pipe.hole_y + pipe_hole_size) continue; // Don't draw between pipes
                if (y >= pipe.hole_y + pipe_hole_size + spr_pipe_head_height || y < pipe.hole_y - spr_pipe_head_height) {
                    // Draw pipe segment
                    draw_sprite_line(line_buffer, pipe.x, 0, spr_pipe_width, spr_pipe_segment_data);
                    continue;
                }
                // Draw pipe head
                int sprite_y;
                if (y <= pipe.hole_y) {
                    sprite_y = pipe.hole_y - y - 1;
                } else {
                    sprite_y = y - pipe.hole_y - pipe_hole_size - 1;
                }
                draw_sprite_line(line_buffer, pipe.x, sprite_y, spr_pipe_width, spr_pipe_head_data);
            }

            if (game_state == GAME_NOT_STARTED) {
                // Draw title
                if (y >= digit_y && y < digit_y + spr_title_height) {
                    draw_sprite_line(line_buffer, (EADK_SCREEN_WIDTH-spr_title_width)/2, y-digit_y, spr_title_width, spr_title_data);
                }

                // Draw blinking "Get ready!"
                if (!((blink/16)%2) && y >= ready_y && y < ready_y + spr_ready_height) {
                    draw_sprite_line(line_buffer, (EADK_SCREEN_WIDTH-spr_ready_width)/2, y-ready_y, spr_ready_width, spr_ready_data);
                }
            } else if (!((blink/16)%2)) { // Check blinking
                // Draw score
                if (y >= digit_y && y < digit_y + spr_digit_height) {
                    // Digits are drawn from right to left
                    int score_digits = score;
                    int x = score_base_x;
                    for (int i=0;i<score_digit_count;i++) {
                        int digit = score_digits%10;
                        int width = digit==1? spr_digit_1_width : spr_digit_width; // Digit 1 has a different width than the others
                        x -= width-2; // -2 pixels margin
                        draw_sprite_line(line_buffer, x, y-digit_y, width, spr_digits[digit]);
                        score_digits /= 10; // Next digit
                    }
                }
            }

            skip_drawing_above_ground:

            // Draw rotated bird
            if (y >= bird_y && y < bird_y + bird_draw_size) {
                double rel_y = y-bird_y-(double)bird_draw_size/2;
                for (int x = 0; x<bird_draw_size; x++) {
                    double rel_x = x - (double)bird_draw_size/2;
                    int rotated_x = rel_x * cos_a - rel_y * sin_a + (double)spr_bird_width/2;
                    int rotated_y = rel_x * sin_a + rel_y * cos_a + (double)spr_bird_height/2;
                    if (!(rotated_x >= 0 && rotated_x < spr_bird_width && rotated_y >= 0 && rotated_y < spr_bird_height)) continue;
                    eadk_color_t pixel_color = spr_bird_data[rotated_y*spr_bird_width + rotated_x];
                    if (pixel_color != transparent_color) {
                        int line_x = x + bird_x;
                        if (line_x >= 0 && line_x < EADK_SCREEN_WIDTH)
                            line_buffer[line_x]  = pixel_color;
                    }
                }
            }

            // Display line
            eadk_display_push_rect((eadk_rect_t){0,y,EADK_SCREEN_WIDTH,1}, line_buffer);
        }
        eadk_display_wait_for_vblank();

        if (eadk_keyboard_key_down(keyboard, eadk_key_home)) break;
    }

    // Save file
    extapp_fileWrite(save_name, (char*)&save, sizeof(save));
}
