#pragma once

#include <ilen/camera/camera_system.h>
#include <ilen/transform/transform_system.h>

#include <ilen/render/render_system.h>
#include <ilen/render/render_draw_system.h>
#include <ilen/render/post_process_effects/post_process_system.h>

#include <ilen/physics/physics_2d_system.h>

#if !ILEN_ONLY_ECS
#include <ilen/game_logic/script_system.h>
#include <ilen/game_logic/physics_event_system_2d.h>
#endif

#if ILEN_DEBUG
#include <ilen/debug/debug_render_system_2d.h>
#endif