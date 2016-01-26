#pragma once

#include "../../engine/ecs.hpp"
#include <memory>

using std::unique_ptr;
using engine::base_system;

unique_ptr<base_system> make_camera_system();
unique_ptr<base_system> make_renderable_system();
unique_ptr<base_system> make_viewshed_system();
unique_ptr<base_system> make_calendar_system();
unique_ptr<base_system> make_settler_ai_system();
unique_ptr<base_system> make_obstruction_system();
unique_ptr<base_system> make_power_system();
unique_ptr<base_system> make_input_system();
unique_ptr<base_system> make_inventory_system();
unique_ptr<base_system> make_cordex_ai_system();
unique_ptr<base_system> make_damage_system();
unique_ptr<base_system> make_particle_system();
unique_ptr<base_system> make_global_illumination_system();
unique_ptr<base_system> make_hud_render_system();
unique_ptr<base_system> make_map_render_system();
unique_ptr<base_system> make_gui_render_system();
unique_ptr<base_system> make_entity_cleanup_system();
