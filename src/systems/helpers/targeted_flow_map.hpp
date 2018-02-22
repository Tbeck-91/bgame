#pragma once
#include "../../planet/region/region.hpp"
#include "../../components/position.hpp"
#include "pathfinding.hpp"
#include "../ai/distance_map_system.hpp"
#include "../../bengine/geometry.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <tuple>

// Implements a generic flow map, of the type used by mining and architecture maps.

namespace flow_maps {

	struct path_result_t
	{
		std::shared_ptr<navigation_path_t> path;
		int target;
	};

	class map_t
	{
	public:
		map_t() = default;
		std::vector<std::tuple<int, int>> targets; // 0 = target, 1 = position

		void fill_map(const std::vector<std::tuple<int, int>> &new_targets) noexcept
		{
			targets.clear();
			for (const auto &t : new_targets)
			{
				const int idx = std::get<0>(t);
				if (region::flag(idx, tile_flags::CAN_STAND_HERE) && systems::distance_map::reachable_from_cordex.get(idx) < systems::dijkstra::MAX_DIJSTRA_DISTANCE) {
					auto[x, y, z] = idxmap(idx);
					std::cout << x << "," << y << "," << z << "\n";
					targets.emplace_back(t);
				}
			}
		}

		path_result_t find_nearest_reachable_target(const position_t &pos)
		{
			std::map<int, std::tuple<int, int>> searcher; // index = range, body = position
			for (const auto &t : targets)
			{
				const auto cordex_reachable = systems::distance_map::reachable_from_cordex.get(std::get<0>(t));
				const auto[x, y, z] = idxmap(std::get<0>(t));
				std::cout << x << "," << y << "," << z << " = " << cordex_reachable << "\n";
				const auto range = static_cast<int>(bengine::distance3d(pos.x, pos.y, pos.z, x, y, z));
				if (cordex_reachable < systems::dijkstra::MAX_DIJSTRA_DISTANCE-1)
					searcher.insert(std::make_pair(range, t));
			}

			for (const auto search : searcher)
			{
				const auto[x, y, z] = idxmap(std::get<0>(search.second));
				std::cout << "Path to " << x << "," << y << "," << z << "\n";
				auto path = find_path(pos, position_t{ x,y,z });
				if (path->success) return path_result_t{ std::move(path), std::get<1>(search.second) };
			}
			path_result_t result{};
			result.target = 0;
			return result;
		}

		bool is_target(const int &idx)
		{
			for (const auto &t : targets)
			{
				if (std::get<0>(t) == idx) return true;
			}
			return false;
		}
	};

}