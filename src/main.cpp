#include <bn_core.h>
#include <bn_keypad.h>
#include <bn_blending.h>
#include <bn_cameras.h>
#include <bn_camera_ptr.h>
#include <bn_sprite_ptr.h>
#include <bn_regular_bg_ptr.h>
#include <bn_log.h>
#include <bn_vector.h>

// Backgrounds and sprites
#include <bn_regular_bg_items_bg_test01.h>
#include "bn_sprite_items_spr_test01.h"

// Include modules
#include "physics.h"
#include "maps.h"

using namespace bn;
using namespace keypad;

#include "main.h"

// Enhanced player with physics integration
struct player_ptr : entity_base
{
    player_ptr()
    {
        osp = sprite_items::spr_test01.create_sprite(0, 0);
        sp()->set_camera(globals->camera);
        sp()->set_position(to_pixel(globals->level->init_x), to_pixel(globals->level->init_y));
        _x = sp()->x();
        _y = sp()->y();
    }

    bool check_level_collision(fixed test_x, fixed test_y) const override
    {
        constexpr int sprite_width = 32;
        constexpr int sprite_height = 32;

        fixed left = test_x - sprite_width / 2;
        fixed right = test_x + sprite_width / 2 - 1;
        fixed top = test_y - sprite_height / 2;
        fixed bottom = test_y + sprite_height / 2 - 1;

        return resolve(left, top) ||
               resolve(right, top) ||
               resolve(left, bottom) ||
               resolve(right, bottom) ||
               resolve(test_x, bottom);
    }

    void update()
    {
        auto &pm = physics_manager::instance();

        // Handle horizontal movement with pushing
        velocity_x = 0;
        if (left_held())
        {
            velocity_x = -move_speed * (held(key_type::R) + 1);
        }
        if (right_held())
        {
            velocity_x = move_speed * (held(key_type::R) + 1);
        }

        if (velocity_x != 0)
        {
            fixed target_x = sp()->x() + velocity_x;

            // Check globals->level collision first
            if (!check_level_collision(target_x, sp()->y()))
            {
                // Try to push clones
                auto push_result = pm.try_push_horizontal(-1, true, target_x);

                if (push_result.success)
                {
                    // Apply the push to all entities
                    pm.apply_push(push_result, -1, true, velocity_x);
                }
            }
        }

        // Apply GRAVITY and vertical movement
        velocity_y += GRAVITY;

        if (velocity_y != 0)
        {
            fixed target_y = sp()->y() + velocity_y;

            if (velocity_y > 0)
            {
                bool level_collision = check_level_collision(sp()->x(), target_y);
                bool entity_support = pm.check_entity_support(-1, true, target_y);

                if (level_collision || entity_support)
                {
                    velocity_y = 0;
                    on_ground = true;
                    jump_count = 0;
                }
                else
                {
                    set_position(sp()->x(), target_y);
                    on_ground = false;
                }
            }
            else if (velocity_y < 0)
            {
                bool level_collision = check_level_collision(sp()->x(), target_y);
                bool entity_collision = pm.check_entity_support(-1, true, target_y);

                if (level_collision || entity_collision)
                {
                    velocity_y = 0;
                }
                else
                {
                    set_position(sp()->x(), target_y);
                    on_ground = false;
                }
            }
        }

        // Jumping
        if (a_pressed())
        {
            if (jump_count < max_jumps)
            {
                velocity_y = -24 * GRAVITY;
                jump_count++;
                on_ground = false;
            }
        }
    }
};

// Enhanced clone with physics integration
struct clone_ptr : entity_base
{
    int init_x, init_y, h = 0;

    int history[256];

    // Check if this clone is in recording mode (ghost mode)
    bool is_recording() const { return h < 256; }

    clone_ptr(int init_x_, int init_y_)
        : init_x(init_x_), init_y(init_y_)
    {
        osp = sprite_items::spr_test01.create_sprite(0, 0);
        sp()->set_camera(globals->camera);
        sp()->set_position(init_x, init_y);
        sp()->set_blending_enabled(true);
        _x = sp()->x();
        _y = sp()->y();
    }

    bool check_level_collision(fixed test_x, fixed test_y) const override
    {
        constexpr int sprite_width = 32;
        constexpr int sprite_height = 32;

        fixed left = test_x - sprite_width / 2;
        fixed right = test_x + sprite_width / 2 - 1;
        fixed top = test_y - sprite_height / 2;
        fixed bottom = test_y + sprite_height / 2 - 1;

        return resolve(left, top) ||
               resolve(right, top) ||
               resolve(left, bottom) ||
               resolve(right, bottom) ||
               resolve(test_x, bottom);
    }

    void physics(int my_index)
    {
        int code = history[h % 256];
        auto &pm = physics_manager::instance();

        // Handle horizontal movement with pushing
        velocity_x = 0;
        if (code & LEFT)
        {
            velocity_x = -move_speed * ((code & DASH) > 0 ? 2 : 1);
        }
        if (code & RIGHT)
        {
            velocity_x = move_speed * ((code & DASH) > 0 ? 2 : 1);
        }

        if (velocity_x != 0)
        {
            fixed target_x = sp()->x() + velocity_x;

            // During recording phase, ignore all collisions - move freely
            if (is_recording())
            {
                set_position(target_x, sp()->y());
            }
            else
            {
                // Check globals->level collision first
                if (!check_level_collision(target_x, sp()->y()))
                {
                    // Try to push other entities
                    auto push_result = pm.try_push_horizontal(my_index, false, target_x);

                    if (push_result.success)
                    {
                        // Apply the push to all entities
                        pm.apply_push(push_result, my_index, false, velocity_x);
                    }
                }
            }
        }

        // Apply GRAVITY and handle vertical movement
        velocity_y += GRAVITY;

        if (velocity_y != 0)
        {
            fixed target_y = sp()->y() + velocity_y;

            // During recording phase, ignore all collisions - move freely
            if (is_recording())
            {
                set_position(sp()->x(), target_y);
                on_ground = false;
            }
            else
            {
                // Check for landing on other entities or globals->level
                if (velocity_y > 0)
                {
                    bool level_collision = check_level_collision(sp()->x(), target_y);
                    bool entity_support = pm.check_entity_support(my_index, false, target_y);

                    if (level_collision || entity_support)
                    {
                        velocity_y = 0;
                        on_ground = true;
                        jump_count = 0;
                    }
                    else
                    {
                        set_position(sp()->x(), target_y);
                        on_ground = false;
                    }
                }
                else if (velocity_y < 0)
                {
                    bool level_collision = check_level_collision(sp()->x(), target_y);
                    bool entity_collision = pm.check_entity_support(my_index, false, target_y);

                    if (level_collision || entity_collision)
                    {
                        velocity_y = 0;
                    }
                    else
                    {
                        set_position(sp()->x(), target_y);
                        on_ground = false;
                    }
                }
            }
        }

        // Jumping
        if (code & JUMP)
        {
            if (jump_count < max_jumps)
            {
                velocity_y = -24 * GRAVITY;
                jump_count++;
                on_ground = false;
            }
        }
    }

    // Check if respawn position would overlap with other entities
    bool check_respawn_collision(int my_index) const
    {
        auto &pm = physics_manager::instance();
        player_ptr *player = static_cast<player_ptr *>(pm.player_);
        vector<clone_ptr, CLONE_COUNT> *clones_2 = static_cast<vector<clone_ptr, CLONE_COUNT> *>(pm.clones_);

        entity_bounds my_bounds(init_x, init_y);

        // Check collision with player
        entity_bounds player_bounds(player->sp()->x(), player->sp()->y());
        if (bounds_overlap(my_bounds, player_bounds))
        {
            return true;
        }

        // Check collision with other clones
        for (int i = 0; i < clones_2->size(); ++i)
        {
            if (i != my_index)
            {
                entity_bounds other_bounds((*clones_2)[i].x(), (*clones_2)[i].y());
                if (bounds_overlap(my_bounds, other_bounds))
                {
                    return true;
                }
            }
        }

        return false;
    }

    // Returns true if clone should be destroyed
    bool update(int my_index)
    {
        if (h < 256)
        {
            // Recording phase - ghost mode (no collisions)
            int code = 0;
            if (left_held())
                code += 1;
            if (right_held())
                code += 2;
            if (a_pressed())
                code += 4;
            if (r_held())
                code += 8;

            history[h] = code;

            // Set visual indicator for recording mode
            if (h == 0)
            {
                sp()->set_blending_enabled(true);
            }
        }
        else
        {
            // Playback phase - normal physics
            if (h % 256 == 0)
            {
                // Check if respawn position is blocked
                if (check_respawn_collision(my_index))
                {
                    return true; // Signal for destruction
                }

                set_position(init_x, init_y);
                velocity_y = 0;
                on_ground = false;
                jump_count = 0;

                // Reset visual appearance for playback
                sp()->set_blending_enabled(true);
            }

            physics(my_index);
        }
        h += 1;
        return false; // Continue existing
    }
};

// Physics manager implementation - here we have access to complete types
entity_base *get_entity(int index, bool is_player)
{
    auto &pm = physics_manager::instance();
    if (is_player)
        return static_cast<player_ptr *>(pm.player_);
    return &(*static_cast<vector<clone_ptr, CLONE_COUNT> *>(pm.clones_))[index];
}

bool would_collide_with_entity(int moving_entity, bool moving_is_player,
                               fixed test_x, fixed test_y,
                               int other_entity, bool other_is_player)
{
    if (moving_is_player == other_is_player && moving_entity == other_entity)
        return false;

    auto &pm = physics_manager::instance();
    vector<clone_ptr, CLONE_COUNT> *clones_2 = static_cast<vector<clone_ptr, CLONE_COUNT> *>(pm.clones_);

    // Skip collision if either entity is a recording clone
    if (!moving_is_player && moving_entity >= 0 && moving_entity < clones_2->size())
    {
        if ((*clones_2)[moving_entity].is_recording())
            return false;
    }
    if (!other_is_player && other_entity >= 0 && other_entity < clones_2->size())
    {
        if ((*clones_2)[other_entity].is_recording())
            return false;
    }

    entity_base *other = get_entity(other_entity, other_is_player);
    entity_bounds moving_bounds(test_x, test_y);
    entity_bounds other_bounds(other->x(), other->y());

    return bounds_overlap(moving_bounds, other_bounds);
}

push_result physics_manager::try_push_horizontal(int pusher_index, bool is_player, fixed target_x)
{
    push_result result;
    entity_base *pusher = get_entity(pusher_index, is_player);
    vector<clone_ptr, CLONE_COUNT> *clones_2 = static_cast<vector<clone_ptr, CLONE_COUNT> *>(this->clones_);

    // Skip collision detection if the pusher is a recording clone
    if (!is_player && pusher_index >= 0 && pusher_index < clones_2->size())
    {
        if ((*clones_2)[pusher_index].is_recording())
        {
            result.success = true;
            result.final_position = target_x;
            return result;
        }
    }

    // Check what entities would be collided with
    vector<pair<int, bool>, CLONE_COUNT> colliding_entities;

    // Check collision with player
    if (!is_player && would_collide_with_entity(pusher_index, is_player, target_x, pusher->y(), -1, true))
    {
        colliding_entities.push_back({-1, true});
    }

    // Check collision with clones
    for (int i = 0; i < clones_2->size(); ++i)
    {
        if (is_player || i != pusher_index)
        {
            if (would_collide_with_entity(pusher_index, is_player, target_x, pusher->y(), i, false))
            {
                colliding_entities.push_back({i, false});
            }
        }
    }

    if (colliding_entities.empty())
    {
        // No collision, can move freely
        result.success = true;
        result.final_position = target_x;
        return result;
    }

    // Try to push all colliding entities
    fixed movement = target_x - pusher->x();
    bool can_push_all = true;

    for (auto [entity_idx, entity_is_player] : colliding_entities)
    {
        entity_base *entity = get_entity(entity_idx, entity_is_player);
        fixed entity_target = entity->x() + movement;

        // Check if this entity can move to its target position
        if (entity->check_level_collision(entity_target, entity->y()))
        {
            can_push_all = false;
            break;
        }

        // Check if this entity would collide with other entities after being pushed
        // Check collision with player
        if (!entity_is_player && would_collide_with_entity(entity_idx, entity_is_player, entity_target, entity->y(), -1, true))
        {
            // Would push into player, check if player can be pushed too
            if (find(colliding_entities.begin(), colliding_entities.end(), make_pair(-1, true)) == colliding_entities.end())
            {
                can_push_all = false;
                break;
            }
        }

        // Check collision with clones
        for (int i = 0; i < clones_2->size(); ++i)
        {
            if (entity_is_player || i != entity_idx)
            {
                if (would_collide_with_entity(entity_idx, entity_is_player, entity_target, entity->y(), i, false))
                {
                    // Would push into another clone, check if that clone is also being pushed
                    if (find(colliding_entities.begin(), colliding_entities.end(), make_pair(i, false)) == colliding_entities.end())
                    {
                        can_push_all = false;
                        break;
                    }
                }
            }
        }

        if (!can_push_all)
            break;
    }

    if (can_push_all)
    {
        result.success = true;
        result.final_position = target_x;
        // Store which entities would be pushed
        for (auto [entity_idx, entity_is_player] : colliding_entities)
        {
            result.pushed_entities.push_back(entity_is_player ? -1 : entity_idx);
        }
    }

    return result;
}

bool physics_manager::check_entity_support(int entity_index, bool is_player, fixed test_y)
{
    entity_base *entity = get_entity(entity_index, is_player);
    vector<clone_ptr, CLONE_COUNT> *clones_2 = static_cast<vector<clone_ptr, CLONE_COUNT> *>(this->clones_);

    // Skip collision if the entity checking support is a recording clone
    if (!is_player && entity_index >= 0 && entity_index < clones_2->size())
    {
        if ((*clones_2)[entity_index].is_recording())
        {
            return false; // Recording clones don't get support from other entities
        }
    }

    // Check if standing on player
    if (!is_player && would_collide_with_entity(entity_index, is_player, entity->x(), test_y, -1, true))
    {
        return true;
    }

    // Check if standing on clones
    for (int i = 0; i < clones_2->size(); ++i)
    {
        if (is_player || i != entity_index)
        {
            if (would_collide_with_entity(entity_index, is_player, entity->x(), test_y, i, false))
            {
                return true;
            }
        }
    }

    return false;
}

void physics_manager::apply_push(const push_result &result, int pusher_index, bool is_player, fixed movement)
{
    // Move the pusher
    entity_base *pusher = get_entity(pusher_index, is_player);
    pusher->set_position(result.final_position, pusher->y());

    // Move all pushed entities
    player_ptr *player = static_cast<player_ptr *>(this->player_);
    vector<clone_ptr, CLONE_COUNT> *clones_2 = static_cast<vector<clone_ptr, CLONE_COUNT> *>(this->clones_);

    for (int pushed_idx : result.pushed_entities)
    {
        if (pushed_idx == -1)
        {
            // Push player
            player->set_position(player->x() + movement, player->y());
        }
        else
        {
            // Push clone
            entity_base *clone = &(*clones_2)[pushed_idx];
            clone->set_position(clone->x() + movement, clone->y());
        }
    }
}

// Main function
int main()
{
    core::init();
    globals = new global_data();

    blending::set_transparency_alpha(0.5);

    auto bg = globals->level->bg_item->create_bg(
        to_pixel(globals->level->size_x / 2) + 16,
        to_pixel(globals->level->size_y / 2) + 16);
    bg.set_camera(globals->camera);

    player_ptr player;
    vector<clone_ptr, CLONE_COUNT> clones;
    physics_manager::instance().register_entities(&player, &clones);

    while (true)
    {
        if (b_pressed())
        {
            clone_ptr new_clone = {player.sp()->x().integer(), player.sp()->y().integer()};
            clones.push_back(new_clone);
        }

        // Update clones (iterate backwards to safely remove during iteration)
        for (int i = clones.size() - 1; i >= 0; --i)
        {
            bool should_destroy = clones.at(i).update(i);
            if (should_destroy)
            {
                clones.erase(clones.begin() + i);
            }
        }

        // Update camera to follow player
        if (player.on_ground || (player.sp()->y() - globals->camera.y() > 36))
        {
            globals->camera.set_position(player.sp()->x(), lerp(globals->camera.y(), player.sp()->y().integer() + 24, 0.2));
        }
        else if (player.sp()->y() - globals->camera.y() < 0)
        {
            globals->camera.set_position(player.sp()->x(), player.sp()->y());
        }
        else
        {
            globals->camera.set_position(player.sp()->x(), globals->camera.y());
        }

        // Update player
        player.update();
        core::update();
    }

    return 0;
}