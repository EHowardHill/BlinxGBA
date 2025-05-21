#include <bn_core.h>
#include <bn_keypad.h>
#include <bn_sprite_ptr.h>
#include <bn_regular_bg_ptr.h>

// Backgrounds
#include <bn_regular_bg_items_bg_test01.h>

// Sprites
#include "bn_sprite_items_spr_test01.h"

// AUX Functions
#include "functions.cpp"

using namespace bn;

int main()
{
    core::init();

    auto bg = regular_bg_items::bg_test01.create_bg(0, 0);
    sprite_ptr sp = sprite_items::spr_test01.create_sprite(0, 0);

    const fixed ground_y = 50;
    fixed_t<4> velocity_y = 0;
    const fixed_t<4> gravity = 0.5;

    while (true)
    {
        velocity_y += gravity;
        sp.set_y(sp.y() + velocity_y);

        if (sp.y() > ground_y)
        {
            sp.set_y(ground_y);
            velocity_y = 0;
        }

        if (keypad::pressed(keypad::key_type::A))
        {
            if (sp.y() == ground_y && velocity_y == 0)
            {
                velocity_y = -8;
            }
        }

        core::update();
    }

    return 0;
}