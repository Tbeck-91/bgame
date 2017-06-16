#include "phase1_sunmoon.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include "sun_fbo.hpp"
#include "../../main/game_calendar.hpp"
#include "gl_utils.hpp"
#include "phase2_terrain.hpp"
#include "../../main/game_camera.hpp"

namespace map_render {
    std::unique_ptr<gl::base_shader_t> shadow_shader;
    std::unique_ptr<gl::base_shader_t> light_shader;
    bool loaded_shadow_shader = false;
    glm::mat4 sun_projection_matrix;
    glm::mat4 sun_modelview_matrix;
    glm::vec3 ambient_color;
    glm::vec3 sun_moon_color;
    glm::vec3 sun_moon_position;
    float light_range;

    void load_shadow_shader() {
        shadow_shader = std::make_unique<gl::base_shader_t>("world_defs/shaders/shadow_vertex.glsl",
                                                            "world_defs/shaders/shadow_fragment.glsl");

        light_shader = std::make_unique<gl::base_shader_t>("world_defs/shaders/light_vertex.glsl",
                                                            "world_defs/shaders/light_fragment.glsl");

        loaded_shadow_shader = true;
    }

    void render_sun_chunk(const gl::chunk_t &chunk, bool set_uniforms) {
        if (!chunk.has_geometry) return;
        if (!chunk.generated_vbo) return;

        GLint world_position;

        if (set_uniforms) {
            world_position = glGetAttribLocation(shadow_shader->program_id, "world_position");
            if (world_position == -1) throw std::runtime_error("Invalid world position in shader");
        }

        glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo_id);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, gl::n_floats*sizeof(float), 0);

        if (set_uniforms) {
            glVertexAttribPointer(world_position, 3, GL_FLOAT, GL_FALSE, gl::n_floats * sizeof(float),
                                  ((char *) nullptr + 3 * sizeof(float)));
            glEnableVertexAttribArray(world_position);
        }

        glDrawArrays(GL_QUADS, 0, chunk.n_quads);

        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void place_sun_moon() {
        bool moon_mode = false;
        light_range = 1000.0f;

        const float time = calendar->hour > 5 && calendar->hour < 18 ? (float)calendar->hour + ((float)calendar->minute/60.0f) : 12.0f;
        //const float time = calendar->hour > 5 && calendar->hour < 18 ? (float)calendar->hour : 5.0f;
        const float time_as_fraction =  1.0f - (time / 24.0f); // Inverted because the sun goes west to east
        const float time_as_degrees = (time_as_fraction * 360.0f) + 270.0f; // Add to put midnight underneath the world
        const float radians = time_as_degrees * 3.14159265f/180.0f;
        constexpr float SUN_DISTANCE = 256.0f;

        float sun_x = (SUN_DISTANCE * std::cos(radians)) + 128.0f;
        float sun_y = (SUN_DISTANCE * std::sin(radians)) + 128.0f;
        const float sun_z = 129.0f;

        if (calendar->hour < 6 || calendar->hour > 17) {
            moon_mode = true;
        }
        //std::cout << time << " : " << time_as_fraction << ", " << sun_x << "/" << sun_y << "/" << sun_z << "\n";

        //glm::mat4 sun_projection_matrix = glm::perspective(90.0f, 1.0f, 1.0f, 600.0f);
        sun_projection_matrix = glm::ortho(-128.0f, 128.0f, -128.0f, 128.0f, 32.0f, 512.0f);
        const glm::vec3 up{0.0f, 1.0f, 0.0f};
        const glm::vec3 target{(float)REGION_WIDTH/2.0f, (float)REGION_DEPTH/2.0f, (float)REGION_HEIGHT/2.0f};
        const glm::vec3 position{sun_x, sun_y, sun_z};
        sun_modelview_matrix = glm::lookAt( position, target, up );

        // Setup ambient light
        rltk::color_t dark_moon{34,63,89};
        const rltk::color_t dawn_light{143,164,191};
        const rltk::color_t noon_sun{201, 226, 255};

        if (moon_mode) {
            float lerp_percent;
            if (calendar->hour < 6) {
                lerp_percent = (float)calendar->hour / 6.0f;
            } else {
                lerp_percent = (24.0f - (float)calendar->hour) / 6.0f;
            }

            int moon_phase = calendar->days_elapsed % 56;
            if (moon_phase > 28) moon_phase = 56 - moon_phase;
            dark_moon.r += moon_phase/2;
            dark_moon.g += moon_phase/2;
            dark_moon.b += moon_phase/2;

            auto ambient_rltk = rltk::lerp(dark_moon, dawn_light, lerp_percent);
            ambient_color = glm::vec3{ (float)ambient_rltk.r / 255.0f, (float)ambient_rltk.g / 255.0f, (float)ambient_rltk.b / 255.0f };
            sun_moon_color = glm::vec3{ 0.7f, 0.7f, 0.8f };
        } else {
            float lerp_percent;
            if (calendar->hour < 12) {
                lerp_percent = ((float)calendar->hour-6.0f) / 6.0f;
            } else {
                lerp_percent = (24.0f - (float)calendar->hour) / 6.0f;
            }
            auto ambient_rltk = rltk::lerp(dawn_light, noon_sun, lerp_percent);
            ambient_color = glm::vec3{ (float)ambient_rltk.r / 255.0f, (float)ambient_rltk.g / 255.0f, (float)ambient_rltk.b / 255.0f };
            sun_moon_color = glm::vec3{ 1.0f, 1.0f, 0.97f };
        }
        sun_moon_position = glm::vec3{ sun_x, sun_y, sun_z };
    }

    void place_light(int direction, float range, const float sun_x, const float sun_y, const float sun_z, const float r, const float g, const float b) {
        sun_projection_matrix = glm::perspective(90.0f, 1.0f, 0.1f, range);
        const glm::vec3 up{0.0f, 1.0f, 0.0f};
        glm::vec3 target;
        switch (direction) {
            case 1 : target = glm::vec3{sun_x + 10.0f, sun_y, sun_z}; break;
            case 2 : target = glm::vec3{sun_x - 10.0f, sun_y, sun_z}; break;
        }
        const glm::vec3 position{sun_x, sun_y, sun_z};
        sun_modelview_matrix = glm::lookAt( position, target, up );
        sun_moon_position = glm::vec3{ sun_x, sun_y, sun_z };
        sun_moon_color = glm::vec3{ r, g, b };
        light_range = range;
    }

    void render_phase_one_sun_moon() {
        // Use sun program
        glUseProgram(map_render::shadow_shader->program_id);

        // Use sun framebuffer and clear
        glBindFramebuffer(GL_FRAMEBUFFER, map_render::sun_fbo);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // setup matrices for sun rendering
        //glClearColor(0.2f, 0.3f, 0.8f, 1.0f);

        // We move the near plane just a bit to make the depth texture a bit more visible.
        // It also increases the precision.
        auto screen_size = rltk::get_window()->getSize();
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_TRUE);
        glClear(GL_DEPTH_BUFFER_BIT);
        glShadeModel(GL_SMOOTH);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        glViewport(0,0,screen_size.x,screen_size.y);

        int sun_projection = glGetUniformLocation(map_render::shadow_shader->program_id, "projection_matrix");
        if (sun_projection == -1) throw std::runtime_error("Unknown uniform slot - projection matrix");
        glUniformMatrix4fv(sun_projection, 1, false, glm::value_ptr(sun_projection_matrix));

        int sun_view = glGetUniformLocation(map_render::shadow_shader->program_id, "view_matrix");
        if (sun_view == -1) throw std::runtime_error("Unknown uniform slot - view matrix");
        glUniformMatrix4fv(sun_view, 1, false, glm::value_ptr(sun_modelview_matrix));

        // render the terrain
        Frustrum frustrum;
        frustrum.update(sun_projection_matrix * sun_modelview_matrix);

        for (const gl::chunk_t &chunk : gl::chunks) {
            if (chunk.has_geometry && chunk.generated_vbo &&
                frustrum.checkSphere(glm::vec3(chunk.base_x, chunk.base_z, chunk.base_y), gl::CHUNK_SIZE)) {
                map_render::render_sun_chunk(chunk);
            }
        }

        // clean up
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void render_to_light_buffer(const bool clear) {
        // Next we initialize the light buffer; this is the first run so we clear it.
        glUseProgram(map_render::light_shader->program_id);
        // Use sun framebuffer and clear
        glBindFramebuffer(GL_FRAMEBUFFER, map_render::light_fbo);
        if (clear) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Setup the program inputs
        map_render::setup_matrices();
        int projection_matrix_loc = glGetUniformLocation(map_render::light_shader->program_id, "projection_matrix");
        if (projection_matrix_loc == -1) throw std::runtime_error("Unknown uniform slot - projection matrix");
        glUniformMatrix4fv(projection_matrix_loc, 1, false, glm::value_ptr( map_render::camera_projection_matrix ));

        int view_matrix_loc = glGetUniformLocation(map_render::light_shader->program_id, "view_matrix");
        if (view_matrix_loc == -1) throw std::runtime_error("Unknown uniform slot - view matrix");
        glUniformMatrix4fv(view_matrix_loc, 1, false, glm::value_ptr( map_render::camera_modelview_matrix ));

        int light_matrix_loc = glGetUniformLocation(map_render::light_shader->program_id, "light_space_matrix");
        if (light_matrix_loc == -1) throw std::runtime_error("Unknown uniform slot - light space matrix");
        glm::mat4 light_matrix = map_render::sun_projection_matrix * map_render::sun_modelview_matrix;
        glUniformMatrix4fv(light_matrix_loc, 1, false, glm::value_ptr( light_matrix ));

        int light_pos_loc = glGetUniformLocation(map_render::light_shader->program_id, "light_position");
        if (light_pos_loc == -1) throw std::runtime_error("Unknown uniform slot - light_position");
        glUniform3fv(light_pos_loc, 1, glm::value_ptr(map_render::sun_moon_position));

        int light_col_loc = glGetUniformLocation(map_render::light_shader->program_id, "light_color");
        if (light_col_loc == -1) throw std::runtime_error("Unknown uniform slot - light_color");
        glUniform3fv(light_col_loc, 1, glm::value_ptr(map_render::sun_moon_color));

        int range_loc = glGetUniformLocation(map_render::light_shader->program_id, "range");
        if (range_loc == -1) throw std::runtime_error("Unknown uniform slot - range");
        glUniform1f(range_loc, light_range);

        GLint world_position;
        world_position = glGetAttribLocation(map_render::light_shader->program_id, "world_position");
        if (world_position == -1) throw std::runtime_error("Invalid world position in shader");

        // Input texture - it needs the shadow map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, map_render::sun_depth_texture);

        int tex1loc = glGetUniformLocation(map_render::light_shader->program_id, "shadow_map");
        if (tex1loc == -1) throw std::runtime_error("Unknown uniform slot - texture 0");
        glUniform1i(tex1loc, 0);

        // Splat out the chunks
        Frustrum frustrum;
        frustrum.update(map_render::camera_projection_matrix * map_render::camera_modelview_matrix);
        for (const gl::chunk_t &chunk : gl::chunks) {
            if (chunk.has_geometry && chunk.generated_vbo &&
                frustrum.checkSphere(glm::vec3(chunk.base_x, chunk.base_z, chunk.base_y), gl::CHUNK_SIZE))
            {
                glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo_id);
                glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(3, GL_FLOAT, gl::n_floats*sizeof(float), 0);

                glVertexAttribPointer(world_position, 3, GL_FLOAT, GL_FALSE, gl::n_floats * sizeof(float),
                                      ((char *) nullptr + 3 * sizeof(float)));
                glEnableVertexAttribArray(world_position);

                int cull_pos = chunk.n_quads;
                auto finder = chunk.z_offsets.find(camera_position->region_z);
                if (finder != chunk.z_offsets.end()) {
                    cull_pos = finder->second;
                    //std::cout << cull_pos << "\n";
                }

                if (cull_pos > 0)
                    glDrawArrays(GL_QUADS, 0, cull_pos);

                glDisableClientState(GL_VERTEX_ARRAY);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
        }
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}