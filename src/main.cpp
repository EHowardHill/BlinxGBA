#include <bn_core.h>
#include <bn_keypad.h>
#include <bn_cameras.h>
#include <bn_camera_ptr.h>
#include <bn_sprite_ptr.h>
#include <bn_regular_bg_ptr.h>
#include <bn_log.h>

// Backgrounds
#include <bn_regular_bg_items_bg_test01.h>
#include <bn_regular_bg_items_bg_test02.h>

// Sprites
#include "bn_sprite_items_spr_test01.h"

using namespace bn;
using namespace keypad;

constexpr int TILE_SIZE = 32;
constexpr int to_tile(int pixel) { return pixel / TILE_SIZE; }
constexpr int to_pixel(int tile) { return tile * TILE_SIZE; }

struct level_ptr
{
    const int collisions[512];
    int size_x;
    int size_y;
    int init_x;
    int init_y;
};

// Fixed collision resolution
constexpr bool resolve(const level_ptr *level, fixed x, fixed y)
{
    int tile_x = to_tile(x.integer());
    int tile_y = to_tile(y.integer());

    // Bounds checking
    if (tile_x < 0 || tile_x >= level->size_x ||
        tile_y < 0 || tile_y >= level->size_y)
        return true;

    int index = tile_y * level->size_x + tile_x;
    return level->collisions[index] > 0;
}

class Player
{
public:
    sprite_ptr sp = sprite_items::spr_test01.create_sprite(0, 0);
    fixed_t<4> velocity_y = 0;
    fixed_t<4> velocity_x = 0;
    const fixed_t<4> gravity = 0.5;
    const fixed_t<4> move_speed = 2;
    camera_ptr *camera;
    const level_ptr *level;
    bool on_ground = false;
    int jump_count = 0;
    const int max_jumps = 2;

    Player(camera_ptr *camera_, const level_ptr *level_) : camera(camera_), level(level_)
    {
        sp.set_camera(*camera);
        sp.set_position(to_pixel(level->init_x), to_pixel(level->init_y));
    }

    bool check_collision(fixed x, fixed y)
    {
        // Fixed: 32x32 sprite dimensions for pixel-perfect collision
        constexpr int sprite_width = 32;
        constexpr int sprite_height = 32;

        // For a 32x32 sprite centered at (x, y), check the bounds
        // Sprite occupies from (x-16, y-16) to (x+15, y+15)
        fixed left = x - sprite_width / 2;        // x - 16
        fixed right = x + sprite_width / 2 - 1;   // x + 15 (rightmost pixel)
        fixed top = y - sprite_height / 2;        // y - 16
        fixed bottom = y + sprite_height / 2 - 1; // y + 15 (bottommost pixel)

        // Check all corners that could touch different tiles
        return resolve(level, left, top) ||     // Top-left corner
               resolve(level, right, top) ||    // Top-right corner
               resolve(level, left, bottom) ||  // Bottom-left corner
               resolve(level, right, bottom) || // Bottom-right corner
               resolve(level, x, bottom);       // Bottom-center for better ground detection
    }

    void update()
    {
        BN_LOG("Keypad: ", held(key_type::R));

        // Handle horizontal movement
        velocity_x = 0;
        if (held(key_type::LEFT))
        {
            velocity_x = -move_speed * (held(key_type::R) + 1);
        }
        if (held(key_type::RIGHT))
        {
            velocity_x = move_speed * (held(key_type::R) + 1);
        }

        // Apply horizontal movement with collision check
        fixed new_x = sp.x() + velocity_x;
        if (!check_collision(new_x, sp.y()))
        {
            sp.set_x(new_x);
        }

        // Apply gravity
        velocity_y += gravity;

        // Apply vertical movement with collision check
        fixed new_y = sp.y() + velocity_y;

        // Check for ground collision
        if (velocity_y > 0 && check_collision(sp.x(), new_y))
        {
            velocity_y = 0;
            on_ground = true;
            jump_count = 0; // Reset jump count when landing
        }
        // Check for ceiling collision
        else if (velocity_y < 0 && check_collision(sp.x(), new_y))
        {
            velocity_y = 0;
        }
        else
        {
            sp.set_y(new_y);
            on_ground = false;
        }

        // Double jumping
        if (pressed(key_type::A))
        {
            if (jump_count < max_jumps)
            {
                velocity_y = -8;
                jump_count++;
                on_ground = false;
            }
        }

        BN_LOG("Ground: ", on_ground, " Jumps: ", jump_count);

        // Update camera to follow player
        camera->set_position(sp.x(), sp.y());
    }
};

int main()
{
    core::init();

    const level_ptr level = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0,
         0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 0, 0,
         0, 4, 0, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 0,
         3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
        21,
        21,
        7,
        5};

    auto camera = camera_ptr::create(0, 0);
    auto bg = regular_bg_items::bg_test02.create_bg(
        to_pixel(level.size_x / 2) + 16,
        to_pixel(level.size_y / 2) + 16);
    bg.set_camera(camera);

    Player player = {&camera, &level};

    while (true)
    {
        player.update();
        core::update();
    }

    return 0;
}