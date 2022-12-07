#include <main_state.h>
#include <glad/glad.h>
#include <math.h>

#include <rafgl.h>

#include <game_constants.h>

static rafgl_raster_t bg;
static rafgl_raster_t upscaled_bg;
static rafgl_raster_t raster, raster2;
static rafgl_raster_t checker;

static rafgl_texture_t texture;

typedef struct Sprite {
    rafgl_spritesheet_t sprite;
    int pos_x;
    int pos_y;
    int init_pos_x;
    int init_pos_y;
    int animation_frame;
} Sprite;

static Sprite character;
static Sprite truck;
static Sprite minivan;

#define NUMBER_OF_TILES 16
rafgl_raster_t tiles[NUMBER_OF_TILES];

#define WORLD_SIZE_W 13
#define WORLD_SIZE_H 13
int tile_world[WORLD_SIZE_W][WORLD_SIZE_H];

#define TILE_SIZE 64

static int raster_width = RASTER_WIDTH, raster_height = RASTER_HEIGHT;
int cam_x = (1 + WORLD_SIZE_W / 2) * TILE_SIZE, cam_y = WORLD_SIZE_H * TILE_SIZE;

int selected_x, selected_y;

void init_tilemap(void)
{
    for (int y = 0; y < WORLD_SIZE_H; y++) {
        for (int x = 0; x < WORLD_SIZE_W; x++) {
            if (y == 0 || y == 6 || y == 12) {
                tile_world[y][x] = 0;
            } else if (y > 0 && y < 6) {
                tile_world[y][x] = 1;
            } else if (y == 7) {
                tile_world[y][x] = 4;
            } else if (y > 6 && y < 11) {
                tile_world[y][x] = 3;
            } else if (y == 11) {
                tile_world[y][x] = 2;
            }
        }
    }
}

void render_tilemap(rafgl_raster_t *raster)
{
    int x, y;
    int x0 = cam_x / TILE_SIZE;
    int x1 = x0 + (raster_width / TILE_SIZE) + 1;
    int y0 = cam_y / TILE_SIZE;
    int y1 = y0 + (raster_height / TILE_SIZE) + 2;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;

    if (x0 >= WORLD_SIZE_W) x0 = WORLD_SIZE_W - 1;
    if (y0 >= WORLD_SIZE_H) y0 = WORLD_SIZE_H - 1;
    if (x1 >= WORLD_SIZE_W) x1 = WORLD_SIZE_W - 1;
    if (y1 >= WORLD_SIZE_H) y1 = WORLD_SIZE_H - 1;

    rafgl_raster_t *draw_tile;

    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            draw_tile = tiles + (tile_world[y][x] % NUMBER_OF_TILES);
            rafgl_raster_draw_raster(raster, draw_tile, x * TILE_SIZE - cam_x,
                                     y * TILE_SIZE - cam_y - draw_tile->height + TILE_SIZE);
        }
    }

    //rafgl_raster_draw_rectangle(raster, selected_x * TILE_SIZE - cam_x, selected_y * TILE_SIZE - cam_y, TILE_SIZE, TILE_SIZE, rafgl_RGB(255, 255, 255));
}

void main_state_init(GLFWwindow *window, void *args, int width, int height)
{
    rafgl_raster_load_from_image(&bg, "res/images/background.png");
    rafgl_raster_load_from_image(&checker, "res/images/checker32.png");

    raster_width = width;
    raster_height = height;

    rafgl_raster_init(&upscaled_bg, raster_width, raster_height);
    rafgl_raster_bilinear_upsample(&upscaled_bg, &bg);

    rafgl_raster_init(&raster, raster_width, raster_height);
    rafgl_raster_init(&raster2, raster_width, raster_height);

    int i;

    char tile_path[256];

    for (i = 0; i < NUMBER_OF_TILES; i++) {
        sprintf(tile_path, "res/tiles/set%d.png", i);
        rafgl_raster_load_from_image(&tiles[i], tile_path);
    }

    init_tilemap();

    rafgl_spritesheet_init(&character, "res/images/character.png", 10, 4);
    rafgl_spritesheet_init(&truck, "res/images/truck.png", 1, 1);
    rafgl_spritesheet_init(&minivan, "res/images/minivan.png", 1, 1);

    cam_x -= raster_width / 2 + 32;
    cam_y -= raster_height / 2 + 96;

    character.pos_x = raster_width / 2 - 32;
    character.pos_y = raster_height / 2 + 32;
    character.init_pos_x = character.pos_x;
    character.init_pos_y = character.pos_y;
    character.animation_frame = 0;

    truck.pos_x = raster_width / 2 + 352 - 64;
    truck.pos_y = raster_height / 2 - 226;
    truck.init_pos_x = truck.pos_x;
    truck.init_pos_y = truck.pos_y;
    truck.animation_frame = 0;

    minivan.pos_x = raster_width / 2 - 352 - 64;
    minivan.pos_y = raster_height / 2 - 98;
    minivan.init_pos_x = minivan.pos_x;
    minivan.init_pos_y = minivan.pos_y;
    minivan.animation_frame = 0;

    rafgl_texture_init(&texture);
}

int pressed;
float location = 0;
float selector = 0;

int animation_running = 0;
int direction = 0;

int hero_speed = 150;
int hover_frames = 0;
int jump_interval = 15;

int rand_num = 0;
int rand_cnt = 300;

void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args)
{
    if (game_data->is_lmb_down && game_data->is_rmb_down) {
        pressed = 1;
        location = rafgl_clampf(game_data->mouse_pos_y, 0, raster_height - 1);
        selector = 1.0f * location / raster_height;
    } else {
        pressed = 0;
    }

    selected_x = rafgl_clampi((game_data->mouse_pos_x + cam_x) / TILE_SIZE, 0, WORLD_SIZE_W - 1);
    selected_y = rafgl_clampi((game_data->mouse_pos_y + cam_y) / TILE_SIZE, 0, WORLD_SIZE_H - 1);

    int sum_x = character.pos_x + cam_x;
    int sum_y = character.pos_y + cam_y;

    if (rand_num == rand_cnt) {

        rand_cnt = 300;
    } else {
        rand_cnt--;
    }

    truck.pos_x -= 1;
    if (truck.pos_x <= truck.init_pos_x + 128 - TILE_SIZE * WORLD_SIZE_W) {
        truck.pos_x = truck.init_pos_x;
    }

    minivan.pos_x += 3;
    if (minivan.pos_x >= minivan.init_pos_x - 192 + TILE_SIZE * WORLD_SIZE_W) {
        minivan.pos_x = minivan.init_pos_x;
    }

    if ((character.pos_x > truck.pos_x - 64 && character.pos_x < truck.pos_x + 128) && (character.pos_y < truck.pos_y + 5 && character.pos_y > truck.pos_y - 5) ||
        (character.pos_x > minivan.pos_x - 64 && character.pos_x < minivan.pos_x + 128) && (character.pos_y < minivan.pos_y + 5 && character.pos_y > minivan.pos_y - 5)) {
        //printf("(%d, %d), (%d, %d)\n", character.pos_x, character.pos_y, truck.pos_x, truck.pos_y);
        cam_x = (1 + WORLD_SIZE_W / 2) * TILE_SIZE;
        cam_y = WORLD_SIZE_H * TILE_SIZE;

        cam_x -= raster_width / 2 + 32;
        cam_y -= raster_height / 2 + 96;

        character.pos_x = character.init_pos_x;
        character.pos_y = character.init_pos_y;
        character.animation_frame = 0;

        truck.pos_x = raster_width / 2 + 352 - 64;
        truck.pos_y = raster_height / 2 - 226;
        truck.init_pos_x = truck.pos_x;
        truck.init_pos_y = truck.pos_y;
        truck.animation_frame = 0;

        minivan.pos_x = raster_width / 2 - 352 - 64;
        minivan.pos_y = raster_height / 2 - 98;
        minivan.init_pos_x = minivan.pos_x;
        minivan.init_pos_y = minivan.pos_y;
        minivan.animation_frame = 0;
    }

    animation_running = 1;

    jump_interval--;
    if (game_data->keys_pressed[RAFGL_KEY_S]) {
        if (jump_interval < 0 && cam_y < raster_height / 2) {
            cam_y += 64;
            truck.pos_y -= 64;
            minivan.pos_y -= 64;
            jump_interval = 15;
            direction = 0;
        }
    } else if (game_data->keys_pressed[RAFGL_KEY_A]) {
        if (jump_interval < 0 && cam_x >= -576) {
            cam_x -= 64;
            truck.init_pos_x += 64;
            truck.pos_x += 64;
            minivan.init_pos_x += 64;
            minivan.pos_x += 64;
            jump_interval = 15;
            direction = 1;
        }
    } else if (game_data->keys_pressed[RAFGL_KEY_W]) {
        if (jump_interval < 0 && cam_y >= -352) {
            cam_y -= 64;
            truck.pos_y += 64;
            minivan.pos_y += 64;
            jump_interval = 15;
            direction = 2;
        }
    } else if (game_data->keys_pressed[RAFGL_KEY_D]) {
        if (jump_interval < 0 && cam_x < raster_width / 8) {
            cam_x += 64;
            truck.init_pos_x -= 64;
            truck.pos_x -= 64;
            minivan.init_pos_x -= 64;
            minivan.pos_x -= 64;
            jump_interval = 15;
            direction = 3;
        }
    } else {
        animation_running = 0;
    }

    if (animation_running) {
        if (hover_frames == 0) {
            character.animation_frame = (character.animation_frame + 1) % 10;
            hover_frames = 5;
        } else {
            hover_frames--;
        }
    }

    int x, y;
    float xn, yn;
    rafgl_pixel_rgb_t sampled, sampled2, resulting, resulting2;

    for (y = 0; y < raster_height; y++) {
        yn = 1.0f * y / raster_height;
        for (x = 0; x < raster_width; x++) {
            xn = 1.0f * x / raster_width;

            sampled = pixel_at_m(upscaled_bg, x, y);
            sampled2 = rafgl_point_sample(&bg, xn, yn);

            resulting = sampled;
            resulting2 = sampled2;

            resulting.rgba = rafgl_RGB(0, 0, 0);
            resulting = sampled;

            pixel_at_m(raster, x, y) = resulting;
            pixel_at_m(raster2, x, y) = resulting2;

            if (pressed && rafgl_distance1D(location, y) < 3 && x > raster_width - 15) {
                pixel_at_m(raster, x, y).rgba = rafgl_RGB(255, 0, 0);
            }
        }
    }

    //if(game_data->mouse_pos_x < raster_width / 10) {
    //    camx -=10;
    //}
    //if(game_data->mouse_pos_x > raster_width - raster_width / 10) {
    //    camx +=10;
    //}
    //if(game_data->mouse_pos_y < raster_height / 10) {
    //    camy -=10;
    //}
    //if(game_data->mouse_pos_y > raster_height - raster_height / 10) {
    //    camy +=10;
    //}

    //if (game_data->keys_pressed[RAFGL_KEY_KP_ADD]) {
    //    tile_world[selected_y][selected_x]++;
    //    tile_world[selected_y][selected_x] %= NUMBER_OF_TILES;
    //}

    render_tilemap(&raster);

    rafgl_raster_draw_spritesheet(&raster, &character, character.animation_frame, direction, character.pos_x, character.pos_y);
    rafgl_raster_draw_spritesheet(&raster, &truck, truck.animation_frame, 0, truck.pos_x, truck.pos_y);
    rafgl_raster_draw_spritesheet(&raster, &minivan, minivan.animation_frame, 0, minivan.pos_x, minivan.pos_y);
}

void main_state_render(GLFWwindow *window, void *args)
{
    rafgl_texture_load_from_raster(&texture, &raster);
    rafgl_texture_show(&texture, 0);
}

void main_state_cleanup(GLFWwindow *window, void *args)
{
    rafgl_raster_cleanup(&raster);
    rafgl_raster_cleanup(&raster2);
    rafgl_texture_cleanup(&texture);
}
