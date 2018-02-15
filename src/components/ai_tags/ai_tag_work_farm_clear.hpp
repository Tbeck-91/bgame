#pragma once

#include "../position.hpp"
#include "../../bengine/path_finding.hpp"
#include <memory>

struct ai_tag_work_farm_clear {

    enum clear_steps { FIND_HOE, FETCH_HOE, FIND_TARGET, GOTO_TARGET, CLEAR_TARGET };

	clear_steps step = FIND_HOE;
	std::size_t tool_id = 0;
	std::shared_ptr<bengine::navigation_path<position_t>> current_path; // Not serialized
};

