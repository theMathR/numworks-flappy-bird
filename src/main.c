#include "eadk.h"
#include <math.h>
#include <string.h>
#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Flappy Bird";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

const eadk_color_t light_blue = 0x949f;
const eadk_color_t transparent_color = 0x1234;

INCBIN(eadk_color_t, spr_ground, "spritesbin/groundsegment.bin");
const int spr_ground_width = 24;
const int spr_ground_height = 40;

INCBIN(eadk_color_t, spr_bird_a, "spritesbin/yellowbird-upflap.bin");
INCBIN(eadk_color_t, spr_bird_b, "spritesbin/yellowbird-midflap.bin");
INCBIN(eadk_color_t, spr_bird_c, "spritesbin/yellowbird-downflap.bin");
const eadk_color_t* spr_bird_frames[] = {spr_bird_a_data, spr_bird_b_data, spr_bird_c_data};
const int spr_bird_width = 34;
const int spr_bird_height = 24;
const int bird_draw_size = 40;

INCBIN(eadk_color_t, spr_pipe_head, "spritesbin/pipe_head.bin");
INCBIN(eadk_color_t, spr_pipe_segment, "spritesbin/pipe_segment.bin");
const int spr_pipe_head_height = 26;
const int spr_pipe_width = 52;

int scroll = 0;

const int bird_x = 50;
int bird_y;
double bird_dy;
double bird_angle = 0;
double sin_a = 0;
double cos_a = 1;
const int bird_collision_margin_x = 10;
const int bird_collision_margin_y = 10;

const eadk_color_t* spr_bird_data = spr_bird_c_data;
const int bird_anim_speed = 5;
int bird_anim = 2*bird_anim_speed;

bool press_registered = true; // True because you press OK to launch the app

const int pipe_collision_margin_x = 2;
const int pipe_collision_margin_y = 2;
const int pipe_hole_size = 80;
typedef struct {
    int x;
    int hole_y;
} pipe_t;
#define pipes_size 4
pipe_t pipes[pipes_size];
const int space_between_pipes = EADK_SCREEN_WIDTH/(pipes_size-1);

int random_pipe_hole_height() {
    return eadk_random() % (EADK_SCREEN_HEIGHT - pipe_hole_size - spr_ground_height);
}

int min(int a, int b) {
    return a<b ? a : b;
}
int max(int a, int b) {
    return a>b ? a : b;
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
    game_state = GAME_OVER;
    bird_dy = -8;
};

void init_game() {
    bird_dy = -5;
    scroll = 0;
    bird_y = (EADK_SCREEN_HEIGHT-spr_ground_height-bird_draw_size)/2; // Centered

    // Init pipes
    for (int i=0;i<pipes_size;i++) {
        pipes[i].x = EADK_SCREEN_WIDTH + i * space_between_pipes;
        pipes[i].hole_y = random_pipe_hole_height();
    }
}

void update_bird_fall() {
    // Animation
    spr_bird_data = spr_bird_frames[bird_anim/bird_anim_speed];
    if (bird_anim < 2 * bird_anim_speed)
        bird_anim++;

    // Gravity
    bird_dy += 0.5;
    bird_y += bird_dy;

    // Angle stuff
    bird_angle = atan(bird_dy/5.);
    sin_a = -sin(bird_angle);
    cos_a = cos(bird_angle);
}

int main() {
    init_game();

    while (true) {
        eadk_keyboard_state_t keyboard = eadk_keyboard_scan();

        // Game logic
        switch (game_state) {
            case GAME_NOT_STARTED:
                // Start game
                if (eadk_keyboard_key_down(keyboard, eadk_key_ok) && !press_registered) {
                    game_state = GAME_ON;
                    press_registered = true;
                }

                break;
            case GAME_OVER:
                update_bird_fall(); // Keep the bird falling

                // Restart game (only once the bird starts falling)
                if (bird_dy > 0 && eadk_keyboard_key_down(keyboard, eadk_key_ok) && !press_registered) {
                    init_game();
                    game_state = GAME_ON;
                    press_registered = true;
                }

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
                int scroll_speed = 1;
                scroll+=scroll_speed;
                for (int i=0;i<pipes_size;i++) { // Scroll pipes
                    pipes[i].x -= scroll_speed;
                    if (pipes[i].x < -space_between_pipes) { // Wrap around
                        pipes[i].x = EADK_SCREEN_WIDTH;
                        pipes[i].hole_y = random_pipe_hole_height();
                    }
                }

                break;
        }

        if (!eadk_keyboard_key_down(keyboard, eadk_key_ok)) press_registered = false;

        // Draw everything line by line
        eadk_color_t line_buffer[EADK_SCREEN_WIDTH];
        for (int y=0;y<EADK_SCREEN_HEIGHT;y++) {
            // Draw ground
            if (y >= 240 - spr_ground_height) {
                for (int x = 0; x < EADK_SCREEN_WIDTH; x++) {
                    line_buffer[x] = spr_ground_data[((x+scroll)%spr_ground_width) + (y-240+spr_ground_height)*spr_ground_width];
                }
                goto skip_drawing_above_ground;
            }

            // Draw background
            memset(line_buffer, light_blue, sizeof(eadk_color_t) * EADK_SCREEN_WIDTH);

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

        if (eadk_keyboard_key_down(keyboard, eadk_key_home)) return 0;
    }
}
