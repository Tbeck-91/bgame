#pragma once

#include <cstdint>

constexpr uint8_t WORLD_WIDTH = 4;
constexpr uint8_t WORLD_HEIGHT = 4;
constexpr uint16_t WORLD_TILES = WORLD_HEIGHT * WORLD_WIDTH;

constexpr uint8_t REGION_WIDTH = 255;
constexpr uint8_t REGION_HEIGHT = 255;
constexpr uint8_t REGION_DEPTH = 64;
constexpr uint16_t REGION_LAYER_TILES = REGION_WIDTH * REGION_HEIGHT;
constexpr uint32_t REGION_TILES = REGION_HEIGHT * REGION_WIDTH * REGION_DEPTH;

constexpr int NUMBER_OF_TILES_IN_THE_WORLD = (WORLD_WIDTH * REGION_WIDTH) * (WORLD_HEIGHT * REGION_HEIGHT);
