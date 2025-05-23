#include <bn_cameras.h>
#include <bn_camera_ptr.h>

struct global_data
{
    camera_ptr camera = camera_ptr::create(0, 0);
    const level_ptr *level;

    global_data()
    {
        level = &demo01;
    }
};

global_data *globals;

const int LEFT = 1;
const int RIGHT = 2;
const int JUMP = 4;
const int DASH = 8;
const int CLONE_COUNT = 4;

constexpr int TILE_SIZE = 32;
constexpr int to_tile(int pixel) { return pixel / TILE_SIZE; }
constexpr int to_pixel(int tile) { return tile * TILE_SIZE; }

// Collision resolution
constexpr bool resolve(fixed x, fixed y)
{
    int tile_x = to_tile(x.integer());
    int tile_y = to_tile(y.integer());

    if (tile_x < 0 || tile_x >= globals->level->size_x ||
        tile_y < 0 || tile_y >= globals->level->size_y)
        return true;

    int index = tile_y * globals->level->size_x + tile_x;
    return globals->level->collisions[index] > 0;
}

fixed_t<12> lerp(fixed a, int b, fixed_t<12> t)
{
    return a * (1 - t) + b * t;
}