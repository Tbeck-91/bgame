#include "settler_ai_system.hpp"
#include "../messages/messages.hpp"
#include "../game_globals.hpp"
#include "path_finding.hpp"
#include "mining_system.hpp"
#include "inventory_system.hpp"
#include <iostream>
#include <map>

using namespace rltk;

void settler_ai_system::update(const double duration_ms) {
}

void settler_ai_system::settler_calculate_initiative(settler_ai_t &ai, game_stats_t &stats) {
	ai.initiative = rng.roll_dice(1, 6) - stat_modifier(stats.dexterity);
}

void settler_ai_system::wander_randomly(entity_t &entity, position_t &pos) {
	const position_t original = pos;
	const int tile_index = current_region.idx(pos.x, pos.y, pos.z);
	//std::cout << current_region.tiles[tile_index].flags << "\n";
	const int direction = rng.roll_dice(1,6);
	switch (direction) {
		case 1 : if (current_region.tiles[tile_index].flags.test(tile_flags::CAN_GO_UP)) pos.z++; break;
		case 2 : if (current_region.tiles[tile_index].flags.test(tile_flags::CAN_GO_DOWN)) pos.z--; break;
		case 3 : if (current_region.tiles[tile_index].flags.test(tile_flags::CAN_GO_NORTH)) pos.y--; break;
		case 4 : if (current_region.tiles[tile_index].flags.test(tile_flags::CAN_GO_SOUTH)) pos.y++; break;
		case 5 : if (current_region.tiles[tile_index].flags.test(tile_flags::CAN_GO_EAST)) pos.x++; break;
		case 6 : if (current_region.tiles[tile_index].flags.test(tile_flags::CAN_GO_WEST)) pos.x--; break;
	}
	if (current_region.tiles[tile_index].flags.test(tile_flags::SOLID)) pos = original;

	renderable_t * render = entity.component<renderable_t>();
	render->foreground = rltk::colors::YELLOW;
	render->glyph = '@';
	emit(renderables_changed_message{});
}

void settler_ai_system::configure() {
	subscribe<tick_message>([this](tick_message &msg) {
		each<settler_ai_t, game_stats_t, species_t, position_t, name_t>([this] (entity_t &entity, settler_ai_t &ai, game_stats_t &stats, 
			species_t &species, position_t &pos, name_t &name) 
		{
			if (ai.initiative < 1) {
				const int shift_id = ai.shift_id;
				const int hour_of_day = calendar->hour;
				const shift_type_t current_schedule = calendar->defined_shifts[shift_id].hours[hour_of_day];

				switch (current_schedule) {
					case SLEEP_SHIFT : this->do_sleep_time(entity, ai, stats, species, pos, name); break;
					case LEISURE_SHIFT : this->do_leisure_time(entity, ai, stats, species, pos, name); break;
					case WORK_SHIFT : this->do_work_time(entity, ai, stats, species, pos, name); break;
				}
				
				this->settler_calculate_initiative(ai, stats);
			} else {
				--ai.initiative;
			}
		});
	});
}

void settler_ai_system::cancel_action(entity_t &entity, settler_ai_t &ai, game_stats_t &stats, species_t &species, position_t &pos, name_t &name, const std::string reason) {
	// Drop whatever we are doing!
	if (ai.job_type_major == JOB_SLEEP) {
		each<construct_provides_sleep_t, position_t>([&pos] (entity_t &e, construct_provides_sleep_t &bed, position_t &bed_pos) {
			if (bed_pos == pos) bed.claimed = false;
		});
	}

	std::cout << name.first_name << " cancels action: " << reason << "\n";
	ai.job_type_major = JOB_IDLE;
	ai.target_x = 0; 
	ai.target_y = 0; 
	ai.target_z = 0;
}

void settler_ai_system::do_sleep_time(entity_t &entity, settler_ai_t &ai, game_stats_t &stats, species_t &species, position_t &pos, name_t &name) {
	if (ai.job_type_major != JOB_IDLE && ai.job_type_major != JOB_SLEEP) {
		cancel_action(entity, ai, stats, species, pos, name, "Bed time!");
		return;
	}
	if (ai.job_type_major == JOB_IDLE) {
		ai.job_type_major = JOB_SLEEP;
		ai.job_type_minor = JM_FIND_BED;
		ai.job_status = "Looking for a bed";
		return;
	}
	if (ai.job_type_major != JOB_SLEEP) throw std::runtime_error("Sleep mode, but shouldn't have made it to here.\n");

	if (ai.job_type_minor == JM_FIND_BED) {
		// Find nearest unclaimed bed
		std::map<float, std::pair<construct_provides_sleep_t,position_t>> bed_candidates;
		each<construct_provides_sleep_t, position_t>([&bed_candidates, &pos] (entity_t &e, construct_provides_sleep_t &bed, position_t &bed_pos) {
			if (!bed.claimed) {
				bed_candidates[distance3d(pos.x, pos.y, pos.z, bed_pos.x, bed_pos.y, bed_pos.z)] = std::make_pair(bed, bed_pos);
			}
		});
		if (bed_candidates.empty()) std::cout << "Warning: no bed found. We should implement ground sleeping.\n";

		// Claim it
		bed_candidates.begin()->second.first.claimed = true;
		ai.target_x = bed_candidates.begin()->second.second.x;
		ai.target_y = bed_candidates.begin()->second.second.y;
		ai.target_z = bed_candidates.begin()->second.second.z;

		// Set the path
		ai.current_path = find_path(pos, position_t{ai.target_x, ai.target_y, ai.target_z});
		if (!ai.current_path->success) {
			std::cout << "Warning: no path to bed found.\n";
			ai.current_path.reset();
		}
		ai.job_type_minor = JM_GO_TO_BED;
		ai.job_status = "Going to bed";
		return;
	}

	if (ai.job_type_minor == JM_GO_TO_BED) {
		if (!ai.current_path) {
			ai.job_type_minor = JM_FIND_BED;
			ai.job_status = "Looking for a bed";
			std::cout << "No path to bed - trying again to find one.\n";
			return;
		}
		if (pos == ai.current_path->destination) {
			ai.job_type_minor = JM_SLEEP;
			ai.job_status = "Sleeping";
			return;
		}
		position_t next_step = ai.current_path->steps.front();
		pos.x = next_step.x;
		pos.y = next_step.y;
		pos.z = next_step.z;
		ai.current_path->steps.pop_front();
		emit(renderables_changed_message{});
		return;
	}

	if (ai.job_type_minor == JM_SLEEP) {
		renderable_t * render = entity.component<renderable_t>();
		render->foreground = rltk::colors::BLUE;

		if (rng.roll_dice(1,6) < 3) {
			render->glyph = '@';
		} else {
			render->glyph = 'Z';
		}

		emit(renderables_changed_message{});
	}
}

void settler_ai_system::do_leisure_time(entity_t &entity, settler_ai_t &ai, game_stats_t &stats, species_t &species, position_t &pos, name_t &name) {
	if (ai.job_type_major == JOB_SLEEP) {
		cancel_action(entity, ai, stats, species, pos, name, "Time to wake up");
		return;
	}
	wander_randomly(entity, pos);
}

void settler_ai_system::do_work_time(entity_t &entity, settler_ai_t &ai, game_stats_t &stats, species_t &species, position_t &pos, name_t &name) {
	if (ai.job_type_major == JOB_SLEEP) {
		cancel_action(entity, ai, stats, species, pos, name, "Time to wake up");
		return;
	}
	if (ai.job_type_major == JOB_IDLE) {
		// Find something to do!
		const int idx = current_region.idx(pos.x, pos.y, pos.z);
		if (ai.permitted_work[JOB_MINING] && mining_map[idx]<250 && is_item_category_available(TOOL_DIGGING)) {
			renderable_t * render = entity.component<renderable_t>();
			render->foreground = rltk::colors::WHITE;
			emit(renderables_changed_message{});
			ai.job_type_major = JOB_MINE;
			ai.job_type_minor = JM_FIND_PICK;
			ai.job_status = "Finding mining tools.";
			return;
		}
	} 
	if (ai.job_type_major == JOB_MINE) {
		do_mining(entity, ai, stats, species, pos, name);
	}
	//wander_randomly(entity, pos);
}

void settler_ai_system::do_mining(entity_t &e, settler_ai_t &ai, game_stats_t &stats, species_t &species, position_t &pos, name_t &name) {
	//std::cout << name.first_name << ": " << ai.job_status << "\n";

	if (ai.job_type_minor == JM_FIND_PICK) {
		inventory_item_t pick = claim_closest_item_by_category(TOOL_DIGGING, pos);
		if (!pick.pos) throw std::runtime_error("Found a pick but can't go there!");
		ai.current_path = find_path(pos, pick.pos.get());
		if (!ai.current_path->success) throw std::runtime_error("Found a pick, but failed to path to it");
		ai.job_type_minor = JM_GO_TO_PICK;
		ai.target_x = pick.pos.get().x;
		ai.target_y = pick.pos.get().y;
		ai.target_z = pick.pos.get().z;
		ai.job_status = "Traveling to digging tool";
		ai.current_tool = pick.id;
		return;
	}

	if (ai.job_type_minor == JM_GO_TO_PICK) {
		if (pos == ai.current_path->destination) {
			// We're at the pick
			ai.current_path.reset();
			ai.job_type_minor = JM_COLLECT_PICK;
			ai.job_status = "Collect digging tool";
			return;
		}
		// Travel to pick
		position_t next_step = ai.current_path->steps.front();
		pos.x = next_step.x;
		pos.y = next_step.y;
		pos.z = next_step.z;
		ai.current_path->steps.pop_front();
		emit(renderables_changed_message{});
		return;
	}

	if (ai.job_type_minor == JM_COLLECT_PICK) {
		// Find the pick, remove any position or stored components, add a carried_by component
		try { delete_component<position_t>(ai.current_tool); } catch (...) {}
		try { delete_component<item_stored_t>(ai.current_tool); } catch (...) {}
		entity(ai.current_tool)->assign(item_carried_t{ INVENTORY, e.id });

		// Notify the inventory system of the change	
		emit(inventory_changed_message{});
		emit(renderables_changed_message{});

		ai.job_type_minor = JM_GO_TO_SITE;
		ai.job_status = "Travelling to dig site";
		return;
	}

	if (ai.job_type_minor == JM_GO_TO_SITE) {
		const int idx = current_region.idx(pos.x, pos.y, pos.z);
		if (mining_map[idx]==0) {
			// We're at a diggable site
			ai.job_type_minor = JM_DIG;
			ai.job_status = "Digging";
			return;
		}
		// Look at adjacent mining map entries, and path to the closest one. If there is no option,
		// Drop the pick.
		int current_direction = 0;
		int min_value = 512;
		if (mining_map[current_region.idx(pos.x, pos.y-1, pos.z)] < min_value && current_region.tiles[idx].flags.test(tile_flags::CAN_GO_NORTH)) { 
			min_value = mining_map[current_region.idx(pos.x, pos.y-1, pos.z)]; 
			current_direction = 1; 
		}
		if (mining_map[current_region.idx(pos.x, pos.y+1, pos.z)] < min_value && current_region.tiles[idx].flags.test(tile_flags::CAN_GO_SOUTH)) { 
			min_value = mining_map[current_region.idx(pos.x, pos.y+1, pos.z)]; 
			current_direction = 2; 
		}
		if (mining_map[current_region.idx(pos.x-1, pos.y, pos.z)] < min_value && current_region.tiles[idx].flags.test(tile_flags::CAN_GO_WEST)) { 
			min_value = mining_map[current_region.idx(pos.x-1, pos.y, pos.z)]; 
			current_direction = 3; 
		}
		if (mining_map[current_region.idx(pos.x+1, pos.y, pos.z)] < min_value && current_region.tiles[idx].flags.test(tile_flags::CAN_GO_EAST)) { 
			min_value = mining_map[current_region.idx(pos.x+1, pos.y, pos.z)]; 
			current_direction = 4; 
		}
		if (mining_map[current_region.idx(pos.x, pos.y, pos.z-1)] < min_value && current_region.tiles[idx].flags.test(tile_flags::CAN_GO_DOWN)) { 
			min_value = mining_map[current_region.idx(pos.x, pos.y, pos.z-1)]; 
			current_direction = 5; 
		}
		if (mining_map[current_region.idx(pos.x, pos.y, pos.z+1)] < min_value && current_region.tiles[idx].flags.test(tile_flags::CAN_GO_UP)) { 
			min_value = mining_map[current_region.idx(pos.x, pos.y, pos.z+1)]; 
			current_direction = 6; 
		}

		if (current_direction == 0) {
			ai.job_type_minor = JM_DROP_PICK;
			ai.job_status = "Dropping digging tools.";
			return;
		}

		switch (current_direction) {
			case 1 : --pos.y; break;
			case 2 : ++pos.y; break;
			case 3 : --pos.x; break;
			case 4 : ++pos.x; break;
			case 5 : --pos.z; break;
			case 6 : ++pos.z; break;
		}
		emit(renderables_changed_message{});

		return;
	}

	if (ai.job_type_minor == JM_DIG) {
		// Determine the digging target from here
		// Make a skill roll, and if successful complete the action
		// When complete, move to dropping the pick
		const int idx = current_region.idx(pos.x, pos.y, pos.z);
		const int target_idx = mining_targets[idx];
		const int target_operation = designations->mining[target_idx];

		if (target_operation == 1) {
			// Dig
			current_region.tiles[target_idx].flags.reset(tile_flags::SOLID);
			current_region.tiles[target_idx].contents = 0;
			current_region.calculate_render_tile(target_idx);
		} else if (target_operation == 2) {
			// Channel
			current_region.tiles[target_idx].flags.reset(tile_flags::SOLID);
			current_region.tiles[target_idx].contents = 0;
			current_region.tiles[target_idx].base_type = 0;
			current_region.calculate_render_tile(target_idx);
			
			// Add ramp
			const int below = target_idx - (REGION_WIDTH * REGION_HEIGHT);
			current_region.tiles[below].flags.reset(tile_flags::SOLID);
			current_region.tiles[below].flags.set(tile_flags::CONSTRUCTION);
			current_region.tiles[below].contents = 4118;
			current_region.calculate_render_tile(below);
		} else if (target_operation == 3) {
			// Ramp
			current_region.tiles[target_idx].flags.reset(tile_flags::SOLID);
			current_region.tiles[target_idx].flags.set(tile_flags::CONSTRUCTION);
			current_region.tiles[target_idx].contents = 4118;
			current_region.calculate_render_tile(target_idx);

			const int above = target_idx + (REGION_WIDTH * REGION_HEIGHT);
			current_region.tiles[above].flags.reset(tile_flags::SOLID);
			current_region.tiles[above].base_type = 0;
			current_region.tiles[above].contents = 0;
			current_region.calculate_render_tile(above);
			
		} else if (target_operation == 4) {
			// Up
			current_region.tiles[target_idx].flags.reset(tile_flags::SOLID);
			current_region.tiles[target_idx].flags.set(tile_flags::CONSTRUCTION);
			current_region.tiles[target_idx].contents = 4111;
			current_region.calculate_render_tile(target_idx);
		} else if (target_operation == 5) {
			// Down
			current_region.tiles[target_idx].flags.reset(tile_flags::SOLID);
			current_region.tiles[target_idx].flags.set(tile_flags::CONSTRUCTION);
			current_region.tiles[target_idx].contents = 4110;
			current_region.calculate_render_tile(target_idx);
		} else if (target_operation == 6) {
			// UpDown
			current_region.tiles[target_idx].flags.reset(tile_flags::SOLID);
			current_region.tiles[target_idx].flags.set(tile_flags::CONSTRUCTION);
			current_region.tiles[target_idx].contents = 4109;
			current_region.calculate_render_tile(target_idx);
		}

		for (int Z=-2; Z<3; ++Z) {
			for (int Y=-2; Y<3; ++Y) {
				for (int X=-2; X<3; ++X) {
					current_region.determine_tile_standability(pos.x + X, pos.y + Y, pos.z + Z);
					current_region.determine_tile_connectivity(pos.x + X, pos.y + Y, pos.z + Z);
				}
			}
		}

		designations->mining[target_idx] = 0;
		emit(recalculate_mining_message{});
		emit(map_dirty_message{});
		ai.job_type_minor = JM_DROP_PICK;
		ai.job_status = "Dropping digging tools.";
		return;
	}

	if (ai.job_type_minor == JM_DROP_PICK) {
		// NOTE that this is broken and needs fixing
		try { delete_component<item_carried_t>(ai.current_tool); } catch (...) {}
		try { entity(ai.current_tool)->assign(position_t{ pos.x, pos.y, pos.z }); } catch (...) {}
		ai.job_type_major = JOB_IDLE;
		ai.job_status = "Idle";
		renderable_t * render = e.component<renderable_t>();
		render->foreground = rltk::colors::YELLOW;
		render->glyph = '@';
		emit(renderables_changed_message{});
		emit(inventory_changed_message{});	
		emit(item_claimed_message{ai.current_tool, false});
		ai.current_tool = 0;
	}
}
