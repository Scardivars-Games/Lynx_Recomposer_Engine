#pragma once

#include <lynx/camera/camera_system.h>
#include <lynx/transform/transform_system.h>

#include <lynx/render/render_system.h>
#include <lynx/render/render_draw_system.h>
#include <lynx/render/post_process_effects/post_process_system.h>

#include <lynx/physics/physics_2d_system.h>

#if !LYNX_ONLY_ECS
#include <lynx/game_logic/script_system.h>
#include <lynx/game_logic/physics_event_system_2d.h>
#endif

#if LYNX_DEBUG
#include <lynx/debug/debug_render_system_2d.h>
#endif