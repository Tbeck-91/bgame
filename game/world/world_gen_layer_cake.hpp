#pragma once

#include "world_height_map.hpp"
#include "world_gen_hydrology.hpp"
#include "../../engine/rng.h"

void make_world_layers ( heightmap_t* base_map, engine::random_number_generator &rng );