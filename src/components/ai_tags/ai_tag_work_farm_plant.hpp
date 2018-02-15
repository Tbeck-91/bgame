#pragma once

#include "../position.hpp"
#include "../../bengine/path_finding.hpp"
#include <memory>

struct ai_tag_work_farm_plant {

    enum plant_steps { FIND_HOE, FETCH_HOE, FIND_SEED, FETCH_SEED, FIND_TARGET, FETCH_TARGET, PLANT };

	plant_steps step = FIND_HOE;
	std::size_t tool_id = 0;
	std::size_t seed_id = 0;
	position_t farm_position;
	std::shared_ptr<bengine::navigation_path<position_t>> current_path; // Not serialized
};

