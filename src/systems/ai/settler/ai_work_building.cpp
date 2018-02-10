#include "ai_work_building.hpp"
#include "templated_work_steps_t.hpp"
#include "../../../components/ai_tags/ai_tag_work_building.hpp"
#include "../../helpers/inventory_assistant.hpp"
#include "../../../bengine/telemetry.hpp"
#include "../../../raws/buildings.hpp"
#include "../../../raws/defs/building_def_t.hpp"
#include "../../../components/buildings/building.hpp"
#include "../../../components/buildings/construct_provides_sleep.hpp"
#include "../../../components/lightsource.hpp"
#include "../../../components/buildings/construct_provides_door.hpp"
#include "../../../components/lever.hpp"
#include "../../../components/buildings/receives_signal.hpp"
#include "../../../components/buildings/smoke_emitter.hpp"
#include "../../physics/visibility_system.hpp"
#include "../workflow_system.hpp"
#include "../../physics/topology_system.hpp"
#include "../../gui/particle_system.hpp"
#include "../../../global_assets/building_designations.hpp"

namespace systems {
	namespace ai_building {

		using namespace bengine;
		using namespace jobs_board;
		using namespace inventory;

		namespace jobs_board {
			void evaluate_building(job_board_t &board, entity_t &e, position_t &pos, job_evaluator_base_t *jt) {
				if (building_designations->buildings.empty()) return; // Nothing to do

				for (const auto &designation : building_designations->buildings) {
					const auto building_id = designation.building_entity;
					const auto building = entity(building_id);
					if (building != nullptr && building->component<claimed_t>() == nullptr) {
						const auto bpos = building->component<position_t>();
						if (bpos != nullptr) {
							const auto distance = distance3d(pos.x, pos.y, pos.z, bpos->x, bpos->y, bpos->z);
							board.insert(std::make_pair(distance, jt));
						}
					}
				}
			}
		}

		static const char * job_tag = "Construction";
		static work::templated_work_steps_t<ai_tag_work_building> work;

		inline void evaluate_candidate_building(entity_t &e, ai_tag_work_building &b, ai_tag_my_turn_t &t, position_t &pos)
		{
			const auto target_entity = entity(b.building_target.building_entity);
			if (target_entity)
			{
				if (target_entity->component<claimed_t>() != nullptr)
				{
					// The building is already being worked on - bail out
					work.cancel_work_tag(e);
				}
				else
				{
					// We're good
					b.step = ai_tag_work_building::building_steps::SELECT_COMPONENT;
					const auto building_component = target_entity->component<building_t>();
					if (building_component) {
						std::stringstream ss;
						ss << "Building " << get_building_def(building_component->tag)->name;
						work.set_status(e, ss.str().c_str());
						target_entity->assign(claimed_t{ e.id });
					} else
					{
						work.cancel_work_tag(e);
					}
				}
			}
			else
			{
				// Building doesn't exist - bail out and remove it.
				building_designations->buildings.pop_back();
				work.cancel_work_tag(e);
			}
		}

		inline void select_building(entity_t &e, ai_tag_work_building &b, ai_tag_my_turn_t &t, position_t &pos) {
			// Select the building
			if (building_designations->buildings.empty()) {
				// If there are no buildings - bail out.
				work.cancel_work_tag(e);
			}
			else if (building_designations->buildings.size() == 1)
			{
				// There's only one building, so we select it
				b.building_target = building_designations->buildings.back();
				evaluate_candidate_building(e, b, t, pos);
			} else
			{
				// Select a random building from the list; this is to prevent getting stuck in loops for buildings we can't get to
				const auto roll = rng.roll_dice(1, building_designations->buildings.size()) - 1;
				b.building_target = building_designations->buildings[roll];
				evaluate_candidate_building(e, b, t, pos);
			}
		}

		inline void select_component(entity_t &e, ai_tag_work_building &b, ai_tag_my_turn_t &t, position_t &pos) {
			auto has_components = true;
			for (auto &component : b.building_target.component_ids) {
				if (!component.second) {
					has_components = false;
					b.current_tool = component.first;
					const auto item_loc = get_item_location(b.current_tool);
					if (!item_loc) {
						// We couldn't find the designated component
						work.cancel_work_tag(e);
						delete_component<claimed_t>(b.building_target.building_entity);
						return;
					}

					b.current_path = find_path(pos, *item_loc, true); // Allow adjacent to component
					if (b.current_path->success) {
						component.second = true;
						b.step = ai_tag_work_building::building_steps::GO_TO_COMPONENT;
						return;
					}
					else {
						// We couldn't find a way there
						work.cancel_work_tag(e);
						delete_component<claimed_t>(b.building_target.building_entity);
					}
					return;
				}
			}

			if (has_components) {
				b.step = ai_tag_work_building::building_steps::ASSEMBLE;
			}
		}

		inline void goto_component(entity_t &e, ai_tag_work_building &b, ai_tag_my_turn_t &t, position_t &pos) {
			work.follow_path(b, pos, e, [&b]() {
				// Cancel
				b.current_path.reset();
				b.step = ai_tag_work_building::building_steps::SELECT_COMPONENT;
			}, [&b]() {
				// Success
				b.current_path.reset();
				b.step = ai_tag_work_building::building_steps::COLLECT_COMPONENT;
			});
		}

		inline void collect_component(entity_t &e, ai_tag_work_building &b, ai_tag_my_turn_t &t, position_t &pos) {
			inventory_system::pickup_item(b.current_tool, e.id);
			b.step = ai_tag_work_building::building_steps::GO_TO_BUILDING;
		}

		inline void goto_building(entity_t &e, ai_tag_work_building &b, ai_tag_my_turn_t &t, position_t &pos) {
			auto building_entity = entity(b.building_target.building_entity);
			if (!building_entity) {
				// Oops - building entity doesn't exist!
				work.cancel_work_tag(e);
				return;
			}
			const auto bpos = building_entity->component<position_t>();
			if (!bpos) {
				work.cancel_work_tag(e);
				return;
			}

			if (!b.current_path) {
				b.current_path = find_path(pos, *bpos, true); // Allow adjacent
				if (!b.current_path->success) {
					// We can't get there.
					if (b.current_tool > 0) {
						inventory_system::drop_item(b.current_tool, pos.x, pos.y, pos.z);
					}
					work.cancel_work_tag(e);
					delete_component<claimed_t>(b.building_target.building_entity);
					return;
				}
			}

			// Are we there yet?
			const auto distance = distance2d(pos.x, pos.y, bpos->x, bpos->y);
			const auto same_z = pos.z == b.building_target.z;
			if (pos == b.current_path->destination || (same_z && distance < 1.5F)) {
				// We're at the site
				b.current_path.reset();
				b.step = ai_tag_work_building::building_steps::DROP_COMPONENT;
				return;
			}

			work.follow_path(b, pos, e, [&b, &e]() {
				// Cancel
				unclaim_by_id(b.current_tool);
				work.cancel_work_tag(e);
				delete_component<claimed_t>(b.building_target.building_entity);
			}, [&b]() {
				// Success - do nothing
			});
		}

		inline void drop_component(entity_t &e, ai_tag_work_building &b, ai_tag_my_turn_t &t, position_t &pos) {
			inventory_system::drop_item(b.current_tool, pos.x, pos.y, pos.z);
			b.current_tool = 0;
			b.step = ai_tag_work_building::building_steps::SELECT_COMPONENT;
		}

		inline void assemble(entity_t &e, ai_tag_work_building &b, ai_tag_my_turn_t &t, position_t &pos) {
			// Build it!
			const auto tag = b.building_target.tag;
			auto finder = get_building_def(tag);
			if (finder == nullptr) throw std::runtime_error("Building tag unknown!");

			if (work::skill_check_or_damage(e, finder->skill.first.c_str(), finder->skill.second, work, 1, "Construction accident"))
			{
				// Destroy components
				std::size_t material = 0;
				for (auto &comp : b.building_target.component_ids) {
					auto component_ptr = entity(comp.first);
					if (component_ptr) {
						auto comptag = component_ptr->component<item_t>()->item_tag;
						material = component_ptr->component<item_t>()->material;
						delete_item(comp.first);
						entity(b.building_target.building_entity)->component<building_t>()->built_with.emplace_back(std::make_pair(comptag, material));
					}
				}

				// Place the building, and assign any provide tags
				call_home("AI", "Building", finder->tag);
				entity(b.building_target.building_entity)->component<building_t>()->complete = true;
				delete_component<claimed_t>(b.building_target.building_entity);
				visibility::opacity_is_dirty();

				for (const auto &provides : finder->provides) {
					if (provides.provides == provides_sleep) {
						entity(b.building_target.building_entity)->assign(construct_provides_sleep_t{});
					}
					else if (provides.provides == provides_light) {
						entity(b.building_target.building_entity)->assign(lightsource_t{ provides.radius, provides.color });
					}
					else if (provides.provides == provides_door) {
						entity(b.building_target.building_entity)->assign(construct_door_t{});
					}
					else if (provides.provides == provides_wall || provides.provides == provides_floor
						|| provides.provides == provides_stairs_up
						|| provides.provides == provides_stairs_down ||
						provides.provides == provides_stairs_updown
						|| provides.provides == provides_ramp || provides.provides == provides_stonefall_trap
						|| provides.provides == provides_cage_trap || provides.provides == provides_blades_trap
						|| provides.provides == provides_spikes)
					{
						topology::perform_construction(b.building_target.building_entity, tag, material);
					}
					else if (provides.provides == provides_signal_recipient) {
						entity(b.building_target.building_entity)->assign(receives_signal_t{});
					}
					else if (provides.provides == provides_lever) {
						entity(b.building_target.building_entity)->assign(lever_t{});
					}
				}
				if (finder->emits_smoke) {
					entity(b.building_target.building_entity)->assign(smoke_emitter_t{});
				}

				render::models_changed = true;
				inventory_system::inventory_has_changed();
				workflow_system::update_workflow();
				const auto pos = entity(b.building_target.building_entity)->component<position_t>();
				if (pos) {
					particles::block_destruction_effect(pos->x, pos->y, pos->z, 0.5f, 0.5f, 0.5f, particles::PARTICLE_LUMBERJACK);
				}

				// Become idle
				work.cancel_work_tag(e);

				building_designations->buildings.erase(
					std::remove_if(building_designations->buildings.begin(),
						building_designations->buildings.end(),
						[&b](building_designation_t &bt) { return bt.building_entity == b.building_target.building_entity; }),
					building_designations->buildings.end()
				);
			}
		}

		void dispatch(entity_t &e, ai_tag_work_building &h, ai_tag_my_turn_t &t, position_t &pos) {
			switch (h.step) {
			case ai_tag_work_building::building_steps::SELECT_BUILDING: select_building(e, h, t, pos); break;
			case ai_tag_work_building::building_steps::SELECT_COMPONENT: select_component(e, h, t, pos); break;
			case ai_tag_work_building::building_steps::GO_TO_COMPONENT: goto_component(e, h, t, pos); break;
			case ai_tag_work_building::building_steps::COLLECT_COMPONENT: collect_component(e, h, t, pos); break;
			case ai_tag_work_building::building_steps::GO_TO_BUILDING : goto_building(e, h, t, pos); break;
			case ai_tag_work_building::building_steps::DROP_COMPONENT: drop_component(e, h, t, pos); break;
			case ai_tag_work_building::building_steps::ASSEMBLE: assemble(e, h, t, pos); break;
			}
		}

		void run(const double &duration_ms) {
			work.do_work(jobs_board::evaluate_building, dispatch, job_tag);
			/*
			if (first_run) {
				first_run = false;
				register_job_offer<ai_tag_work_building>(jobs_board::evaluate_building);
			}

			ai_work_template<ai_tag_work_building> work;
			work.do_ai("Construction", [&work](entity_t &e, ai_tag_work_building &b, ai_tag_my_turn_t &t, position_t &pos) {
				if (b.step == ai_tag_work_building::building_steps::SELECT_BUILDING) {
					// Select the building
					if (designations->buildings.empty()) {
						work.cancel_work_tag(e);
						return;
					}
					b.building_target = (designations->buildings.back());
					designations->buildings.pop_back();
					b.step = ai_tag_work_building::building_steps::SELECT_COMPONENT;
					return;
				}
				else if (b.step == ai_tag_work_building::building_steps::SELECT_COMPONENT) {
					// Component selection
					bool has_components = true;
					for (std::pair<std::size_t, bool> &component : b.building_target.component_ids) {
						if (!component.second) {
							has_components = false;
							b.current_tool = component.first;
							auto item_loc = get_item_location(b.current_tool);
							if (!item_loc) {
								designations->buildings.push_back(b.building_target);
								work.cancel_work_tag(e);
								return;
							}
							b.current_path = find_path(pos, *item_loc);
							if (b.current_path->success) {
								component.second = true;
								b.step = ai_tag_work_building::building_steps::GO_TO_COMPONENT;
								return;
							}
							else {
								designations->buildings.push_back(b.building_target);
								work.cancel_work_tag(e);
							}
							return;
						}
					}

					if (has_components) {
						b.step = ai_tag_work_building::building_steps::ASSEMBLE;
					}
					return;
				}
				else if (b.step == ai_tag_work_building::building_steps::GO_TO_COMPONENT) {
					work.follow_path(b, pos, e, [&b]() {
						// Cancel
						b.current_path.reset();
						b.step = ai_tag_work_building::building_steps::SELECT_COMPONENT;
					}, [&b]() {
						// Success
						b.current_path.reset();
						b.step = ai_tag_work_building::building_steps::COLLECT_COMPONENT;
					});
					return;
				}
				else if (b.step == ai_tag_work_building::building_steps::COLLECT_COMPONENT) {
					inventory_system::pickup_item(b.current_tool, e.id );
					b.step = ai_tag_work_building::building_steps::GO_TO_BUILDING;
					return;
				}
				else if (b.step == ai_tag_work_building::building_steps::GO_TO_BUILDING) {
					auto building_entity = entity(b.building_target.building_entity);
					if (!building_entity) {
						work.cancel_work_tag(e);
						return;
					}
					auto bpos = building_entity->component<position_t>();
					if (!bpos) {
						work.cancel_work_tag(e);
						return;
					}

					if (!b.current_path) {
						b.current_path = find_path(pos, *bpos);
						if (!b.current_path->success) {
							work.cancel_work_tag(e);
							return;
						}
					}

					// Are we there yet?
					const float distance = distance2d(pos.x, pos.y, bpos->x, bpos->y);
					const bool same_z = pos.z == b.building_target.z;
					if (pos == b.current_path->destination || (same_z && distance < 1.4F)) {
						// We're at the site
						b.current_path.reset();
						b.step = ai_tag_work_building::building_steps::DROP_COMPONENT;
						return;
					}

					work.follow_path(b, pos, e, [&b, &e, &work]() {
						// Cancel
						unclaim_by_id(b.current_tool);
						designations->buildings.push_back(b.building_target);
						work.cancel_work_tag(e);
					}, [&b]() {
						// Success - do nothing
					});

					return;
				}
				else if (b.step == ai_tag_work_building::building_steps::DROP_COMPONENT) {
					inventory_system::drop_item(b.current_tool, pos.x, pos.y, pos.z );
					b.current_tool = 0;
					b.step = ai_tag_work_building::building_steps::SELECT_COMPONENT;
					return;
				}
				else if (b.step == ai_tag_work_building::building_steps::ASSEMBLE) {
					// Build it!
					std::string tag = b.building_target.tag;
					auto finder = get_building_def(tag);
					if (finder == nullptr) throw std::runtime_error("Building tag unknown!");

					// Make a skill roll
					const std::string skill = finder->skill.first;
					const int difficulty = finder->skill.second;
					auto stats = e.component<game_stats_t>();
					auto skill_check = skill_roll(e.id, *stats, rng, skill, difficulty);

					if (skill_check >= SUCCESS) {
						// Destroy components
						std::size_t material = 0;
						for (auto &comp : b.building_target.component_ids) {
							auto component_ptr = entity(comp.first);
							if (component_ptr) {
								std::string comptag = component_ptr->component<item_t>()->item_tag;
								material = component_ptr->component<item_t>()->material;
								delete_item(comp.first);
								entity(b.building_target.building_entity)->component<building_t>()->built_with.push_back(
									std::make_pair(comptag, material));
							}
						}

						// Place the building, and assign any provide tags
						call_home("AI", "Building", finder->tag);
						entity(b.building_target.building_entity)->component<building_t>()->complete = true;
						visibility::opacity_is_dirty();

						for (const building_provides_t &provides : finder->provides) {
							if (provides.provides == provides_sleep) {
								entity(b.building_target.building_entity)->assign(construct_provides_sleep_t{});
							}
							else if (provides.provides == provides_light) {
								entity(b.building_target.building_entity)->assign(lightsource_t{ provides.radius, provides.color });
							}
							else if (provides.provides == provides_door) {
								entity(b.building_target.building_entity)->assign(construct_door_t{});
							}
							else if (provides.provides == provides_wall || provides.provides == provides_floor
								|| provides.provides == provides_stairs_up
								|| provides.provides == provides_stairs_down ||
								provides.provides == provides_stairs_updown
								|| provides.provides == provides_ramp || provides.provides == provides_stonefall_trap
								|| provides.provides == provides_cage_trap || provides.provides == provides_blades_trap
								|| provides.provides == provides_spikes) 
							{
								topology::perform_construction(b.building_target.building_entity, tag, material );
							}
							else if (provides.provides == provides_signal_recipient) {
								entity(b.building_target.building_entity)->assign(receives_signal_t{});
							}
							else if (provides.provides == provides_lever) {
								entity(b.building_target.building_entity)->assign(lever_t{});
							}
						}
						if (finder->emits_smoke) {
							entity(b.building_target.building_entity)->assign(smoke_emitter_t{});
						}

						render::models_changed = true;
						inventory_system::inventory_has_changed();
						workflow_system::update_workflow();
						// TODO: emit(map_changed_message{});
						auto pos = entity(b.building_target.building_entity)->component<position_t>();
						if (pos) {
							particles::block_destruction_effect(pos->x, pos->y, pos->z, 0.5f, 0.5f, 0.5f, particles::PARTICLE_LUMBERJACK);
						}

						// Become idle
						work.cancel_work_tag(e);
					}

					return;
				}
			});
			*/
		}
	}
}