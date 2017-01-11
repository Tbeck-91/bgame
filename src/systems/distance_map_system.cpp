#include "distance_map_system.hpp"
#include "../messages/tick_message.hpp"
#include "../components/grazer_ai.hpp"
#include "../components/position.hpp"
#include "../components/corpse_harvestable.hpp"
#include "../planet/region.hpp"
#include "../messages/entity_moved_message.hpp"
#include "../components/construct_provides_sleep.hpp"
#include "../components/settler_ai.hpp"
#include "../messages/messages.hpp"
#include "../main/game_globals.hpp"
#include "../components/item.hpp"
#include "inventory_system.hpp"
#include <unordered_set>

dijkstra_map huntables_map;
dijkstra_map butcherables_map;
dijkstra_map bed_map;
dijkstra_map settler_map;
dijkstra_map architecure_map;
dijkstra_map blocks_map;

using namespace rltk;

void distance_map_system::configure() {
    system_name = "Distance Maps";
    subscribe_mbox<huntable_moved_message>();
    subscribe_mbox<butcherable_moved_message>();
    subscribe_mbox<bed_changed_message>();
    subscribe_mbox<settler_moved_message>();
    subscribe_mbox<architecture_changed_message>();
    subscribe_mbox<blocks_changed_message>();
    subscribe_mbox<map_changed_message>();
}

void distance_map_system::update(const double duration_ms) {
    each_mbox<huntable_moved_message>([this] (const huntable_moved_message &msg) { update_huntables = true; });
    each_mbox<butcherable_moved_message>([this] (const butcherable_moved_message &msg) { update_butcherables = true; });
    each_mbox<bed_changed_message>([this] (const bed_changed_message &msg) { update_bed_map = true; });
    each_mbox<settler_moved_message>([this] (const settler_moved_message &msg) { update_settler_map = true; });
    each_mbox<architecture_changed_message>([this] (const architecture_changed_message &msg) { update_architecture_map = true; });
    each_mbox<blocks_changed_message>([this] (const blocks_changed_message &msg) { update_blocks_map = true; });
    each_mbox<map_changed_message>([this] (const map_changed_message &msg) {
        update_huntables = true;
        update_butcherables = true;
        update_bed_map = true;
        update_settler_map = true;
        update_architecture_map = true;
        update_blocks_map = true;
    });

    if (update_huntables) {
        std::vector<int> huntables;
        each<grazer_ai, position_t>([&huntables] (entity_t &e, grazer_ai &ai, position_t &pos) {
            huntables.emplace_back(mapidx(pos));
        });
        huntables_map.update(huntables);

        update_huntables = false;
    }

    if (update_butcherables) {
        std::vector<int> butcherables;
        each<corpse_harvestable, position_t>([&butcherables] (entity_t &e, corpse_harvestable &corpse, position_t &pos) {
            butcherables.emplace_back(mapidx(pos));
        });
        butcherables_map.update(butcherables);

        update_butcherables = false;
    }

    if (update_bed_map) {
        std::vector<int> beds;
        each<construct_provides_sleep_t, position_t>([&beds] (entity_t &e, construct_provides_sleep_t &bed, position_t &pos) {
            if (!bed.claimed) {
                beds.emplace_back(mapidx(pos));
            }
        });
        bed_map.update(beds);

        update_bed_map = false;
    }

    if (update_settler_map) {
        std::vector<int> settlers;
        each<settler_ai_t, position_t>([&settlers] (entity_t &e, settler_ai_t &settler, position_t &pos) {
            settlers.emplace_back(mapidx(pos));
        });
        settler_map.update(settlers);

        update_settler_map = false;
    }

    if (update_architecture_map) {
        std::vector<int> targets;
        for (auto it = designations->architecture.begin(); it != designations->architecture.end(); ++it) {
            targets.emplace_back(it->first);
        }
        architecure_map.update(targets);
        update_architecture_map = false;
    }

    if (update_blocks_map) {
        std::unordered_set<int> used;
        std::vector<int> targets;

        each_if<item_t>([] (entity_t &e, item_t &i) { return i.claimed == false && i.item_tag == "block"; },
                        [&used, &targets] (entity_t &e, item_t &i) {
                            auto pos = get_item_location(e.id);
                            if (pos) {
                                const int idx = mapidx(pos.get());
                                if (used.find(idx)==used.end()) {
                                    used.insert(idx);
                                    targets.emplace_back(idx);
                                }
                            }
                        }
        );
        blocks_map.update(targets);
        update_blocks_map = false;
    }
}