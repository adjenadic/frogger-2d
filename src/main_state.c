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
static Sprite trucks[5];
static Sprite vans[5];
static Sprite minivans[5];

typedef struct {
    float x, y;
    float xspeed, yspeed;
    rafgl_pixel_rgb_t color;
    float size;
    int age;
    int duration;
} Particle;

Particle particles[15][200];

#define NUMBER_OF_TILES 16
rafgl_raster_t tiles[NUMBER_OF_TILES];

#define WORLD_SIZE 13
int tile_world[WORLD_SIZE][WORLD_SIZE];

#define TILE_SIZE 64

static int raster_width = RASTER_WIDTH, raster_height = RASTER_HEIGHT;
int cam_x = (1 + WORLD_SIZE / 2) * TILE_SIZE, cam_y = WORLD_SIZE * TILE_SIZE;

int selected_x, selected_y;

void tilemap_init(void)
{
    for (int y = 0; y < WORLD_SIZE; y++) {
        for (int x = 0; x < WORLD_SIZE; x++) {
            if (y == 6 || y == 12) {
                tile_world[y][x] = 0;
            } else if (y >= 0 && y < 6) {
                if (y == 0) {
                    if (x == WORLD_SIZE - 2) {
                        tile_world[y][x] = 4;
                    } else {
                        tile_world[y][x] = 1;
                    }
                } else if (x == WORLD_SIZE - 2 || ((y == 1 || y == 2) && (x == 2))) {
                    tile_world[y][x] = 2;
                } else {
                    if (randf() + 1 < 0.2f) {
                        tile_world[y][x] = 3;
                    } else {
                        tile_world[y][x] = 1;
                    }
                    if ((y == 1 && x == 7) || (y == 4 && x == 6)
                        || (y == 3 && x == 12) || (y == 5 && x == 10)) {
                        tile_world[y][x] = 6;
                        tile_world[y][x - 1] = 5;
                    }
                }
            } else if (y == 7) {
                tile_world[y][x] = 9;
            } else if (y > 7 && y < 11) {
                tile_world[y][x] = 8;
            } else if (y == 11) {
                tile_world[y][x] = 7;
            }
        }
    }
}

void tilemap_render(rafgl_raster_t *raster)
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

    if (x0 >= WORLD_SIZE) x0 = WORLD_SIZE - 1;
    if (y0 >= WORLD_SIZE) y0 = WORLD_SIZE - 1;
    if (x1 >= WORLD_SIZE) x1 = WORLD_SIZE - 1;
    if (y1 >= WORLD_SIZE) y1 = WORLD_SIZE - 1;

    rafgl_raster_t *draw_tile;

    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            draw_tile = tiles + (tile_world[y][x] % NUMBER_OF_TILES);
            rafgl_raster_draw_raster(raster, draw_tile, x * TILE_SIZE - cam_x,
                                     y * TILE_SIZE - cam_y - draw_tile->height + TILE_SIZE);
        }
    }
}

void particles_init(rafgl_raster_t *raster)
{
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 200; j++) {
            particles[i][j].x = raster->width / 10 * (rand() % 11);
            particles[i][j].y = raster->height / 10 * (rand() % 11);
            particles[i][j].xspeed = rand() % 100 / 100.0 - 0.3;
            particles[i][j].yspeed = rand() % 100 / 100.0 - 0.3;
            particles[i][j].color.rgba = rafgl_RGB(rand() % 256, rand() % 256, rand() % 256);
            particles[i][j].size = rand() % 100 / 100.0 + 1;
            particles[i][j].age = 0;
            particles[i][j].duration = rand() % 500 + 500;
        }
    }
}

void particles_draw(rafgl_raster_t *raster)
{
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 200; j++) {
            if (particles[i][j].age > 0) {
                rafgl_raster_draw_line(raster, particles[i][j].x, (particles[i][j].y),
                                       particles[i][j].x + particles[i][j].size,
                                       (particles[i][j].y + particles[i][j].size), particles[i][j].color.rgba);
            }
        }
    }
}

void particles_update(rafgl_raster_t *raster)
{
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 200; j++) {
            if (particles[i][j].age > -1) {
                if (particles[i][j].x + particles[i][j].xspeed > raster->width)
                    particles[i][j].xspeed = -particles[i][j].xspeed;
                if (particles[i][j].x + particles[i][j].xspeed < 0)
                    particles[i][j].xspeed = -particles[i][j].xspeed;
                if (particles[i][j].y + particles[i][j].yspeed > raster->height)
                    particles[i][j].yspeed = -particles[i][j].yspeed;
                if (particles[i][j].y + particles[i][j].yspeed < 0)
                    particles[i][j].yspeed = -particles[i][j].yspeed;

                particles[i][j].x += particles[i][j].xspeed;
                particles[i][j].y += particles[i][j].yspeed;
                particles[i][j].age++;
                if (particles[i][j].age > particles[i][j].duration) {
                    particles[i][j].age = -1;
                }
            }
        }
    }
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

    char tile_path[256];

    for (int i = 0; i < NUMBER_OF_TILES; i++) {
        sprintf(tile_path, "res/tiles/set%d.png", i);
        rafgl_raster_load_from_image(&tiles[i], tile_path);
    }

    tilemap_init();

    rafgl_spritesheet_init(&character, "res/images/character.png", 1, 4);
    rafgl_spritesheet_init(&trucks[0], "res/images/truck.png", 1, 1);
    rafgl_spritesheet_init(&vans[0], "res/images/van.png", 1, 1);
    rafgl_spritesheet_init(&minivans[0], "res/images/minivan.png", 1, 1);

    // INITIAL CAMERA POSITION
    cam_x -= raster_width / 2 + 32;
    cam_y -= raster_height / 2 + 96;

    // INITIAL SPRITE POSITIONS
    character.pos_x = raster_width / 2 - 32;
    character.pos_y = raster_height / 2 + 32;
    character.init_pos_x = character.pos_x;
    character.init_pos_y = character.pos_y;
    character.animation_frame = 0;

    trucks[0].pos_x = raster_width / 2 + 352 - 64;
    trucks[0].pos_y = raster_height / 2 - 226;
    trucks[0].init_pos_x = trucks[0].pos_x;
    trucks[0].init_pos_y = trucks[0].pos_y;
    trucks[0].animation_frame = 0;

    vans[0].pos_x = raster_width / 2 + 352 - 64;
    vans[0].pos_y = raster_height / 2 - 162;
    vans[0].init_pos_x = vans[0].pos_x;
    vans[0].init_pos_y = vans[0].pos_y;
    vans[0].animation_frame = 0;

    minivans[0].pos_x = raster_width / 2 - 352 - 64;
    minivans[0].pos_y = raster_height / 2 - 98;
    minivans[0].init_pos_x = minivans[0].pos_x;
    minivans[0].init_pos_y = minivans[0].pos_y;
    minivans[0].animation_frame = 0;

    rafgl_texture_init(&texture);
    particles_init(&raster);
}

int pressed;
float location = 0;
float selector = 0;

int direction = 2;
int jump_interval = 10;

int win_state = 0;
int reset_state = 0;

void main_state_reset()
{
    cam_x = (1 + WORLD_SIZE / 2) * TILE_SIZE;
    cam_y = WORLD_SIZE * TILE_SIZE;

    cam_x -= raster_width / 2 + 32;
    cam_y -= raster_height / 2 + 96;

    character.pos_x = character.init_pos_x;
    character.pos_y = character.init_pos_y;
    character.animation_frame = 0;
    direction = 2;

    trucks[0].pos_x = raster_width / 2 + 352 - 64;
    trucks[0].pos_y = raster_height / 2 - 226;
    trucks[0].init_pos_x = trucks[0].pos_x;
    trucks[0].init_pos_y = trucks[0].pos_y;
    trucks[0].animation_frame = 0;

    vans[0].pos_x = raster_width / 2 + 352 - 64;
    vans[0].pos_y = raster_height / 2 - 162;
    vans[0].init_pos_x = vans[0].pos_x;
    vans[0].init_pos_y = vans[0].pos_y;
    vans[0].animation_frame = 0;

    minivans[0].pos_x = raster_width / 2 - 352 - 64;
    minivans[0].pos_y = raster_height / 2 - 98;
    minivans[0].init_pos_x = minivans[0].pos_x;
    minivans[0].init_pos_y = minivans[0].pos_y;
    minivans[0].animation_frame = 0;
}

void main_state_win()
{
    rafgl_pixel_rgb_t result;

    for (int y = 0; y < raster_height; y++) {
        for (int x = 0; x < raster_width; x++) {
            rafgl_pixel_rgb_t sampled = pixel_at_m(raster, x, y);

            int brightness = (sampled.r + sampled.g + sampled.b) / 3;
            result.r = brightness;
            result.g = brightness;
            result.b = brightness;

            pixel_at_m(raster, x, y) = result;
        }
    }

    rafgl_raster_draw_string(&raster, "GAME OVER", raster_width / 2 - 75, raster_height / 2 - 30, rafgl_RGB(0, 0, 0),
                             640);
    rafgl_raster_draw_string(&raster, "PRESS SPACE TO PLAY AGAIN", raster_width / 2 - 200, raster_height / 2,
                             rafgl_RGB(0, 0, 0), 640);
    particles_draw(&raster);
    particles_update(&raster);
}

void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args)
{
    if (game_data->is_lmb_down && game_data->is_rmb_down) {
        pressed = 1;
        location = rafgl_clampf(game_data->mouse_pos_y, 0, raster_height - 1);
        selector = 1.0f * location / raster_height;
    } else {
        pressed = 0;
    }

    selected_x = rafgl_clampi((game_data->mouse_pos_x + cam_x) / TILE_SIZE, 0, WORLD_SIZE - 1);
    selected_y = rafgl_clampi((game_data->mouse_pos_y + cam_y) / TILE_SIZE, 0, WORLD_SIZE - 1);

    // MAIN STATE RESET
    if (((character.pos_x > trucks[0].pos_x - 64 && character.pos_x < trucks[0].pos_x + 128) &&
         (character.pos_y < trucks[0].pos_y + 5 && character.pos_y > trucks[0].pos_y - 5)) ||
        ((character.pos_x > minivans[0].pos_x - 64 && character.pos_x < minivans[0].pos_x + 128) &&
         (character.pos_y < minivans[0].pos_y + 5 && character.pos_y > minivans[0].pos_y - 5)) ||
        ((character.pos_x > vans[0].pos_x - 64 && character.pos_x < vans[0].pos_x + 128) &&
         (character.pos_y < vans[0].pos_y + 5 && character.pos_y > vans[0].pos_y - 5)) ||
        (tile_world[(character.pos_y + cam_y) / TILE_SIZE][(character.pos_x + cam_x) / TILE_SIZE] == 1) ||
        (tile_world[(character.pos_y + cam_y) / TILE_SIZE][(character.pos_x + cam_x) / TILE_SIZE] == 6)) {
        main_state_reset();
        reset_state = 80;
    }

    // VEHICLE MOVEMENT
    trucks[0].pos_x -= 3;
    if (trucks[0].pos_x <= trucks[0].init_pos_x + 128 - TILE_SIZE * WORLD_SIZE) {
        trucks[0].pos_x = trucks[0].init_pos_x;
    }

    vans[0].pos_x -= 6;
    if (vans[0].pos_x <= vans[0].init_pos_x + 128 - TILE_SIZE * WORLD_SIZE) {
        vans[0].pos_x = vans[0].init_pos_x;
    }

    minivans[0].pos_x += 5;
    if (minivans[0].pos_x >= minivans[0].init_pos_x - 160 + TILE_SIZE * WORLD_SIZE) {
        minivans[0].pos_x = minivans[0].init_pos_x;
    }

    // CHARACTER MOVEMENT
    jump_interval--;
    if (win_state == 0) {
        if (game_data->keys_pressed[RAFGL_KEY_S] || game_data->keys_pressed[RAFGL_KEY_DOWN]) {
            if (jump_interval < 0 && cam_y < raster_height / 2) {
                cam_y += 64;
                jump_interval = 15;
                direction = 0;

                trucks[0].pos_y -= 64;
                vans[0].pos_y -= 64;
                minivans[0].pos_y -= 64;
            }
        } else if (game_data->keys_pressed[RAFGL_KEY_A] || game_data->keys_pressed[RAFGL_KEY_LEFT]) {
            if (jump_interval < 0 && cam_x >= -576) {
                cam_x -= 64;
                jump_interval = 15;
                direction = 1;

                trucks[0].init_pos_x += 64;
                trucks[0].pos_x += 64;
                vans[0].init_pos_x += 64;
                vans[0].pos_x += 64;
                minivans[0].init_pos_x += 64;
                minivans[0].pos_x += 64;
            }
        } else if (game_data->keys_pressed[RAFGL_KEY_W] || game_data->keys_pressed[RAFGL_KEY_UP]) {
            if (jump_interval < 0 && cam_y >= -352) {
                cam_y -= 64;
                jump_interval = 15;
                direction = 2;

                trucks[0].pos_y += 64;
                vans[0].pos_y += 64;
                minivans[0].pos_y += 64;
            }
        } else if (game_data->keys_pressed[RAFGL_KEY_D] || game_data->keys_pressed[RAFGL_KEY_RIGHT]) {
            if (jump_interval < 0 && cam_x < raster_width / 8) {
                cam_x += 64;
                jump_interval = 15;
                direction = 3;

                trucks[0].init_pos_x -= 64;
                trucks[0].pos_x -= 64;
                vans[0].init_pos_x -= 64;
                vans[0].pos_x -= 64;
                minivans[0].init_pos_x -= 64;
                minivans[0].pos_x -= 64;
            }
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

    tilemap_render(&raster);

    rafgl_raster_draw_spritesheet(&raster, &character, character.animation_frame, direction, character.pos_x,
                                  character.pos_y);
    rafgl_raster_draw_spritesheet(&raster, &trucks[0], trucks[0].animation_frame, 0, trucks[0].pos_x, trucks[0].pos_y);
    rafgl_raster_draw_spritesheet(&raster, &vans[0], vans[0].animation_frame, 0, vans[0].pos_x, vans[0].pos_y);
    rafgl_raster_draw_spritesheet(&raster, &minivans[0], minivans[0].animation_frame, 0, minivans[0].pos_x,
                                  minivans[0].pos_y);

    // MAIN STATE WIN
    if (tile_world[(character.pos_y + cam_y) / TILE_SIZE][(character.pos_x + cam_x) / TILE_SIZE] == 4) {
        main_state_win();
        win_state = 1;
        if (game_data->keys_pressed[RAFGL_KEY_SPACE]) {
            main_state_reset();
            win_state = 0;
        }
    }

    // MAIN STATE RESET SPRITESHEET
    if (reset_state > 0) {
        rafgl_pixel_rgb_t result;
        for (int y = character.pos_y; y < character.pos_y + 64; y++) {
            for (int x = character.pos_x; x < character.pos_x + 64; x++) {
                if (reset_state % 2 == 0) {
                    rafgl_pixel_rgb_t sampled = pixel_at_m(raster, x, y);
                    rafgl_pixel_rgb_t sampled2 = pixel_at_m(character.sprite.sheet, x % 64, y % 64);

                    result.r = sampled.r + 330;
                    result.g = sampled.g + 330;
                    result.b = sampled.b + 330;

                    if (sampled2.rgba != rafgl_RGB(255, 0, 254)) {
                        pixel_at_m(raster, x, y) = result;
                    }
                }
            }
        }
        reset_state--;
    }
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
