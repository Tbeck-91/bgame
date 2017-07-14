#include "static_model_shader.hpp"

namespace gl {
    static_model_shader_t::static_model_shader_t() {
        load("world_defs/shaders/model_vertex.glsl",
             "world_defs/shaders/model_fragment.glsl");

        in_position_loc = get_attribute_location("in_position");
        world_position_loc = get_uniform_location("world_position");
        normal_loc = get_attribute_location("normal");
        texture_position_loc = get_attribute_location("texture_position");
        projection_matrix_loc = get_uniform_location("projection_matrix");
        view_matrix_loc = get_uniform_location("view_matrix");
        camera_position_loc = get_uniform_location("camera_position");
        my_color_texture_loc = get_uniform_location("my_color_texture");
        flags_loc = get_uniform_location("flags");
        light_position_loc = get_uniform_location("light_position");
        light_color_loc = get_uniform_location("light_color");
    }
}