#include "settler_work_time.hpp"
#include "work_types/mining_work.hpp"
#include "work_types/chopping_work.hpp"
#include "work_types/building_work.hpp"
#include "work_types/reaction_work.hpp"
#include "work_types/equip_melee_work.hpp"
#include "work_types/equip_ranged_work.hpp"
#include "work_types/equip_armor_work.hpp"
#include "work_types/hunting_work.hpp"
#include "work_types/guard_duty.hpp"
#include "work_types/demolition_work.hpp"

#include "../../messages/messages.hpp"
#include "../../main/game_globals.hpp"
#include "../path_finding.hpp"
#include "../mining_system.hpp"
#include "../inventory_system.hpp"
#include "../workflow_system.hpp"
#include "../wildlife_population_system.hpp"
#include "../weapons_helpers.hpp"
#include "../components/item_carried.hpp"
#include "idle_mode.hpp"
#include "settler_glyph.hpp"
#include "settler_job_status.hpp"
#include "settler_drop_tool.hpp"
#include "settler_cancel_action.hpp"
#include "pathfinding.hpp"
#include "initiative.hpp"
#include "../../messages/log_message.hpp"
#include "../../components/logger.hpp"
#include "../../components/health.hpp"
#include "../../components/renderable.hpp"
#include "../../components/corpse_harvestable.hpp"
#include "../../components/construct_provides_sleep.hpp"
#include "../../components/viewshed.hpp"
#include "../../components/smoke_emitter.hpp"
#include "../../components/grazer_ai.hpp"
#include "../../components/sentient_ai.hpp"
#include "../../components/lightsource.hpp"
#include "../../components/falling.hpp"
#include "world_queries.hpp"
#include "settler_sleep.hpp"
#include "settler_wander.hpp"
#include "settler_move_to.hpp"

#include <iostream>
#include <map>

using namespace rltk;
using tasks::become_idle;
using tasks::change_settler_glyph;
using tasks::change_job_status;
using tasks::drop_current_tool;
using tasks::cancel_action;
using tasks::follow_path;
using tasks::follow_result_t;
using tasks::calculate_initiative;

void do_work_time(entity_t &entity, settler_ai_t &ai, game_stats_t &stats, species_t &species, position_t &pos, name_t &name) {
	if (ai.job_type_major == JOB_SLEEP) {
		cancel_action(entity, ai, stats, species, pos, name, "Time to wake up");
		return;
	}
	if (ai.job_type_major == JOB_IDLE) {
		// Find something to do!
		const auto idx = mapidx(pos.x, pos.y, pos.z);
		
		if (ai.permitted_work[JOB_GUARDING] && !designations->guard_points.empty() && shooting_range(entity, pos)>0) {
			for (auto &g : designations->guard_points) {
				if (!g.first) {
					g.first = true;
					ai.job_type_major = JOB_GUARD;
					ai.job_type_minor = JM_FIND_GUARDPOST;
					ai.target_x = g.second.x;
					ai.target_y = g.second.y;
					ai.target_z = g.second.z;
					change_job_status(ai, name, "starting guard duty.", true);
					return;
				}
			}
		}

		if (ai.permitted_work[JOB_MINING] && mining_map[idx]<250 && is_item_category_available(TOOL_DIGGING)) {
			change_settler_glyph(entity, vchar{1, rltk::colors::WHITE, rltk::colors::BLACK});
			ai.job_type_major = JOB_MINE;
			ai.job_type_minor = JM_FIND_PICK;
			change_job_status(ai, name, "doing some mining.", true);
			return;
		}
		if (ai.permitted_work[JOB_CHOPPING] && designations->chopping.size() > 0 && is_item_category_available(TOOL_CHOPPING)) {
			change_settler_glyph(entity, vchar{1, rltk::colors::Brown, rltk::colors::BLACK});
			ai.job_type_major = JOB_CHOP;
			ai.job_type_minor = JM_FIND_AXE;
			change_job_status(ai, name, "chopping down a tree.", true);
			return;
		}
		if (ai.permitted_work[JOB_CONSTRUCTION] && designations->buildings.size() > 0) {
			ai.building_target.reset();

			ai.building_target = (designations->buildings.back());
			designations->buildings.pop_back();

			if (ai.building_target) {
				change_settler_glyph(entity, vchar{1, rltk::colors::Pink, rltk::colors::BLACK});
				ai.job_type_major = JOB_CONST;
				ai.job_type_minor = JM_SELECT_COMPONENT;
				change_job_status(ai, name, "performing construction.", true);
			}
			return;
		}
		if (ai.permitted_work[JOB_CONSTRUCTION] && designations->deconstructions.size() > 0) {
			unbuild_t building = designations->deconstructions.back();
			designations->deconstructions.pop_back();
			if (building.is_building) {
				ai.target_id = building.building_id;
				change_settler_glyph(entity, vchar{1, rltk::colors::Pink, rltk::colors::BLACK});
				ai.job_type_major = JOB_DECONSTRUCT;
				ai.job_type_minor = JM_FIND_DECONSTRUCT;
				change_job_status(ai, name, "performing demolition.", true);
			} else {
				std::cout << building.building_id << "\n";
				ai.target_id = building.building_id;
				change_settler_glyph(entity, vchar{1, rltk::colors::RED, rltk::colors::BLACK});
				ai.job_type_major = JOB_DEMOLISH;
				ai.job_type_minor = JM_FIND_DEMOLISH;
				change_job_status(ai, name, "performing structural demolition.", true);
			}
			return;
		}

		// Look for a queued order to perform
		if (!designations->build_orders.empty()) {
			boost::optional<reaction_task_t> autojob = find_queued_reaction_task(ai);

			if (autojob) {
				auto finder = reaction_defs.find(autojob.get().reaction_tag);
				change_settler_glyph(entity, vchar{1, get_task_color(finder->second.skill), rltk::colors::BLACK});
				ai.job_type_major = JOB_REACTION;
				ai.job_type_minor = JM_SELECT_INPUT;
				change_job_status(ai, name, autojob.get().job_name, true);
				ai.reaction_target = autojob;
				return;
			}
		}

		// Look for an automatic reaction to perform
		boost::optional<reaction_task_t> autojob = find_automatic_reaction_task(ai);
		if (autojob) {
			auto finder = reaction_defs.find(autojob.get().reaction_tag);
			change_settler_glyph(entity, vchar{1, get_task_color(finder->second.skill), rltk::colors::BLACK});
			ai.job_type_major = JOB_REACTION;
			ai.job_type_minor = JM_SELECT_INPUT;
			change_job_status(ai, name, autojob.get().job_name, true);
			ai.reaction_target = autojob;
			return;
		}

		// If we don't have a ranged weapon, and one is available, equip it
		std::pair<bool, std::string> ranged_status = has_ranged_weapon(entity);
		if (is_item_category_available(WEAPON_RANGED) && ranged_status.first==false) {
			change_settler_glyph(entity, vchar{1, rltk::colors::WHITE, rltk::colors::BLACK});
			ai.job_type_major = JOB_EQUIP_RANGED;
			ai.job_type_minor = JM_FIND_RANGED_WEAPON;
			change_job_status(ai, name, "Finding a ranged weapon.", false);
			return;
		}

		// Likewise, search for ammo if available
		bool has_ammo = has_appropriate_ammo(entity, ranged_status.second, pos);
		if (designations->standing_order_upgrade > standing_orders::SO_UPGRADE_NEVER && ranged_status.first && !has_ammo && is_ammo_available(ranged_status.second)) {
			change_settler_glyph(entity, vchar{1, rltk::colors::WHITE, rltk::colors::BLACK});
			ai.job_type_major = JOB_EQUIP_AMMO;
			ai.job_type_minor = JM_FIND_AMMO;
			change_job_status(ai, name, "Finding ammunition.");
			return;
		}

		// Butcher corpses
		if (ai.permitted_work[JOB_BUTCHER] && butcher_and_corpses_exist()) {
			change_settler_glyph(entity, vchar{1, rltk::colors::RED, rltk::colors::BLACK});
			ai.job_type_major = JOB_BUTCHERING;
			ai.job_type_minor = JM_BUTCHER_FIND_CORPSE;
			change_job_status(ai, name, "Finding corpse to butcher.", true);
			return;
		}

		// Hunt
		if (butcher_exist() && ai.permitted_work[JOB_HUNTING] && ranged_status.first && has_ammo && !get_hunting_candidates(pos).empty()) {
			change_settler_glyph(entity, vchar{1, rltk::colors::GREEN, rltk::colors::BLACK});
			ai.job_type_major = JOB_HUNT;
			ai.job_type_minor = JM_HUNT_FIND_TARGET;
			change_job_status(ai, name, "Finding target to hunt.", true);
			return;
		}

		// If we don't have a melee weapon, and one is available, equip it
		if (designations->standing_order_upgrade > standing_orders::SO_UPGRADE_NEVER && is_item_category_available(WEAPON_MELEE) && has_melee_weapon(entity)==false) {
			change_settler_glyph(entity, vchar{1, rltk::colors::WHITE, rltk::colors::BLACK});
			ai.job_type_major = JOB_EQUIP_MELEE;
			ai.job_type_minor = JM_FIND_MELEE_WEAPON;
			change_job_status(ai, name, "Finding a melee weapon.", false);
			return;
		}

		// Look for improved armor
		if (designations->standing_order_upgrade > standing_orders::SO_UPGRADE_NEVER) {
			int max_range = -1;
			if (designations->standing_order_upgrade == standing_orders::SO_UPGRADE_NEARBY) max_range = 15;
			boost::optional<std::size_t> better_armor = find_armor_upgrade(entity, max_range);
			if (better_armor) {
				change_settler_glyph(entity, vchar{1, rltk::colors::WHITE, rltk::colors::BLACK});
				ai.job_type_major = JOB_EQUIP_ARMOR;
				ai.job_type_minor = JM_FIND_ARMOR;
				ai.target_id = better_armor.get();
				change_job_status(ai, name, "Finding armor.", true);
				return;
			}
		}

	} else if (ai.job_type_major == JOB_MINE) {
		do_mining(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_CHOP) {
		do_chopping(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_CONST) {
		do_building(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_REACTION) {
		do_reaction(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_EQUIP_MELEE) {
		do_equip_melee(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_EQUIP_RANGED) {
		do_equip_ranged(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_EQUIP_AMMO) {
		do_equip_ammo(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_EQUIP_ARMOR) {
		do_equip_armor(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_HUNT) {
		do_hunting(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_BUTCHERING) {
		do_butchering(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_GUARD) {
		do_guard_duty(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_DECONSTRUCT) {
		do_deconstruction(entity, ai, stats, species, pos, name);
		return;
	} else if (ai.job_type_major == JOB_DEMOLISH) {
		do_demolition(entity, ai, stats, species, pos, name);
		return;
	}
	wander_randomly(entity, pos);
}