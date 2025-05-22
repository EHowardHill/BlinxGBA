#include "physics.h"

// Entity bounds implementation
entity_bounds::entity_bounds(fixed x, fixed y)
{
    constexpr int sprite_width = 32;
    constexpr int sprite_height = 32;
    left = x - sprite_width / 2;
    right = x + sprite_width / 2 - 1;
    top = y - sprite_height / 2;
    bottom = y + sprite_height / 2 - 1;
}

// Check if two entity bounds overlap
bool bounds_overlap(const entity_bounds &a, const entity_bounds &b)
{
    return a.left < b.right && a.right > b.left &&
           a.top < b.bottom && a.bottom > b.top;
}

// Singleton instance
physics_manager &physics_manager::instance()
{
    static physics_manager inst;
    return inst;
}