#pragma once

#include "../render_engine/shaders/chunk_shader.hpp"
#include "../render_engine/shaders/voxel_shader.hpp"
#include <memory>

namespace assets {
    extern unsigned int spriteshader;
    extern unsigned int worldgenshader;
    extern std::unique_ptr<chunk_shader_t> chunkshader;
    extern unsigned int depthquad_shader;
    extern unsigned int lightstage_shader;
    extern unsigned int tonemap_shader;
    extern unsigned int bloom_shader;
	extern unsigned int sprite_shader;
    extern std::unique_ptr<voxel_shader_t> voxel_shader;
	extern unsigned int cursor_shader;
}