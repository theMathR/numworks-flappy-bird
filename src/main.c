#include "eadk.h"
#include <math.h>
#include <string.h>
#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Flappy Bird";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

const eadk_color_t light_blue = 0x949f;

INCBIN(eadk_color_t, spr_ground, "spritesbin/groundsegment.bin");
const int spr_ground_width = 24;
const int spr_ground_height = 40;

INCBIN(eadk_color_t, spr_bird, "spritesbin/yellowbird-upflap.bin");
const int spr_bird_width = 34;
const int spr_bird_height = 24;
const int bird_square_size = 40;


int scroll = 0;
int bird_x = 50;
int bird_y = 60;
double bird_angle;

int main() {
    while (true) {

        bird_angle += 0.01;
        scroll++;

        for (int y=0;y<EADK_SCREEN_HEIGHT;y++) {

            eadk_color_t line_buffer[EADK_SCREEN_WIDTH];

            // Draw background
            if (y < 240 - spr_ground_height) {
                memset(line_buffer, light_blue, sizeof(line_buffer));
            } else {
                for (int x = 0; x < EADK_SCREEN_WIDTH; x++) {
                    line_buffer[x] = spr_ground_data[((x+scroll)%spr_ground_width) + (y-240+spr_ground_height)*spr_ground_width];
                }
            }

            // Draw bird
            if (y >= bird_y && y < bird_y + bird_square_size) {
                double sin_a = -sin(bird_angle);
                double cos_a = cos(bird_angle);
                double y_ = y-bird_y-(double)bird_square_size/2;
                for (int x = 0; x<bird_square_size; x++) {
                    line_buffer[x + bird_x] = eadk_color_red;
                    double x_ = x - (double)bird_square_size/2;
                    int rotated_x = x_ * cos_a - y_ * sin_a + (double)spr_bird_width/2;
                    int rotated_y = x_ * sin_a + y_ * cos_a + (double)spr_bird_height/2;
                    if (!(rotated_x >= 0 && rotated_x < spr_bird_width && rotated_y >= 0 && rotated_y < spr_bird_height)) continue;
                    const eadk_color_t pixel_color = spr_bird_data[rotated_y*spr_bird_width + rotated_x];
                    if (pixel_color != 0x1234) {
                        line_buffer[x + bird_x]  = pixel_color;
                    }
                }
            }

            eadk_display_push_rect((eadk_rect_t){0,y,EADK_SCREEN_WIDTH,1}, line_buffer);
        }
        eadk_display_wait_for_vblank();

        if (eadk_keyboard_key_down(eadk_keyboard_scan(), eadk_key_home)) return 0;
    }
}
