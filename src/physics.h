#pragma once

#include <bn_core.h>
#include <bn_vector.h>
#include <bn_fixed.h>
#include <bn_sprite_ptr.h>
#include <bn_optional.h>

using namespace bn;

// Entity bounds for collision detection
struct entity_bounds
{
    fixed left, right, top, bottom;

    entity_bounds(fixed x, fixed y);
};

// Push result for chain pushing
struct push_result
{
    bool success = false;
    fixed final_position = 0;
    vector<int, 64> pushed_entities; // indices of entities that would be pushed
};

const fixed_t<4> GRAVITY = 0.25;

// Base entity interface for polymorphic collision handling
struct entity_base
{
    fixed x() const { return _x; }
    fixed y() const { return _y; }
    virtual bool check_level_collision(fixed test_x, fixed test_y) const = 0;
    virtual ~entity_base() = default;

    optional<sprite_ptr> osp;
    fixed_t<4> move_speed = 2;
    fixed_t<4> velocity_y = 0;
    fixed_t<4> velocity_x = 0;
    int max_jumps = 1;
    int jump_count = 0;
    bool on_ground = false;

    sprite_ptr *sp()
    {
        return &osp.value();
    }

    void set_position(fixed new_x, fixed new_y)
    {
        osp.value().set_position(new_x, new_y);
        _x = new_x;
        _y = new_y;
    }

    fixed _x = 0, _y = 0;
};

// Utility functions that don't need complete type information
bool bounds_overlap(const entity_bounds &a, const entity_bounds &b);

// Physics manager interface - implementations will be in main.cpp to avoid circular dependencies
struct physics_manager
{
    static physics_manager &instance();

    // Simple data storage - no complex logic here
    void register_entities(void *player_2, void *clones_2)
    {
        this->player_ = player_2;
        this->clones_ = clones_2;
    }

    // These will be implemented in main.cpp where complete types are available
    push_result try_push_horizontal(int pusher_index, bool is_player, fixed target_x);
    bool check_entity_support(int entity_index, bool is_player, fixed test_y);
    void apply_push(const push_result &result, int pusher_index, bool is_player, fixed movement);

    // Store as void* to avoid circular dependency, cast in main.cpp
    void *player_ = nullptr;
    void *clones_ = nullptr;
};