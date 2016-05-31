#include "panel_render_system.hpp"
#include "../game_globals.hpp"
#include "../raws/raws.hpp"
#include "../components/components.hpp"
#include "../messages/messages.hpp"
#include <sstream>

using namespace rltk;
using namespace rltk::colors;

const color_t GREEN_BG{0,32,0};

void panel_render_system::update(const double duration_ms) {
	term(3)->clear(vchar{' ', WHITE, GREEN_BG});
	term(3)->box(DARKEST_GREEN);

	// Mode switch controls
	if (game_master_mode == PLAY) {
		term(3)->print(1,1,"PLAY", WHITE, DARKEST_GREEN);
		render_play_mode();
	} else {
		term(3)->print(1,1,"PLAY (ESC)", GREEN, GREEN_BG);
	}

	if (game_master_mode == DESIGN) {
		term(3)->print(13,1,"DESIGN", WHITE, DARKEST_GREEN);
		render_design_mode();
	} else {
		term(3)->print(10,1,"(D)ESIGN", GREEN, GREEN_BG);
	}

	if (game_master_mode == PLAY) {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
			game_master_mode = DESIGN;
			pause_mode = PAUSED;
			emit(map_dirty_message{});
		}
	}
	if (game_master_mode == DESIGN) {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
			game_master_mode = PLAY;
			emit(map_dirty_message{});
			emit(recalculate_mining_message{});
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
			game_design_mode = DIGGING;
			emit(map_dirty_message{});
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::B)) {
			game_design_mode = BUILDING;
			emit(map_dirty_message{});
		}
	}
}

void panel_render_system::configure() {

}

void panel_render_system::render_play_mode() {
	// Controls Help
	if (pause_mode == PAUSED) {
		term(3)->print(1,3,"SPACE : Unpause", GREEN, GREEN_BG);
	} else {
		term(3)->print(1,3,"SPACE : Pause", GREEN, GREEN_BG);
	}
	term(3)->print(1,4,".     : One Step", GREEN, GREEN_BG);

	// Mouse tips
	int mouse_x, mouse_y;
	std::tie(mouse_x, mouse_y) = get_mouse_position();

	// Since we're using an 8x8, it's just a matter of dividing by 8 to find the terminal-character
	// coordinates. There will be a helper function for this once we get into retained GUIs.
	const int terminal_x = mouse_x / 8;
	const int terminal_y = mouse_y / 8;

	if (terminal_x >= 0 && terminal_x < term(1)->term_width && terminal_y >= 0 && terminal_y < term(1)->term_height) {
		const int world_x = std::min(clip_left + terminal_x, REGION_WIDTH);
		const int world_y = std::min(clip_top + terminal_y-2, REGION_HEIGHT);
		const int idx = current_region.idx(world_x, world_y, camera_position->region_z);

		{
			const int base_tile_type = current_region.tiles[idx].base_type;
			std::stringstream ss;
			auto finder = tile_types.find(base_tile_type);
			if (finder != tile_types.end()) {
				ss << finder->second.name;
				term(3)->print(1, term(3)->term_height - 2, ss.str(), GREEN, GREEN_BG);
			}
		}
		{
			const int base_tile_content = current_region.tiles[idx].contents;
			std::stringstream ss;
			auto finder = tile_contents.find(base_tile_content);
			if (finder != tile_contents.end()) {
				ss << finder->second.name;
				term(3)->print(1, term(3)->term_height - 3, ss.str(), GREEN, GREEN_BG);
			}
		}
		{
			std::stringstream ss;
			if (current_region.tiles[idx].flags.test(tile_flags::SOLID)) ss << "Solid ";
			if (current_region.tiles[idx].flags.test(tile_flags::TREE)) ss << "Tree ";
			if (current_region.tiles[idx].flags.test(tile_flags::CONSTRUCTION)) ss << "Construct ";
			term(3)->print(1, term(3)->term_height - 4, ss.str(), GREEN, GREEN_BG);
		}
		int count = 0;
		each<name_t, position_t>([&count, &world_x, &world_y] (entity_t &entity, name_t &name, position_t &pos) {
			if (pos.x == world_x && pos.y == world_y && pos.z == camera_position->region_z) {
				term(3)->print(1, term(3)->term_height - 5 - count, name.first_name + std::string(" ") + name.last_name, GREEN, GREEN_BG);
				++count;
			}
		});
	}
}

inline bool is_mining_designation_valid(const int &x, const int &y, const int &z, const game_mining_mode_t &mode) {
	return true;
}

void panel_render_system::render_design_mode() {
	if (game_design_mode == DIGGING) {
		term(3)->print(1,3, "Digging", WHITE, DARKEST_GREEN);

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) game_mining_mode = DIG;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) game_mining_mode = CHANNEL;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) game_mining_mode = RAMP;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::U)) game_mining_mode = UP;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::J)) game_mining_mode = DOWN;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::I)) game_mining_mode = UPDOWN;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)) game_mining_mode = DELETE;

		if (game_mining_mode == DIG) { term(3)->print(1,8, "(d) Dig", WHITE, DARKEST_GREEN); } else { term(3)->print(1,8, "(d) Dig", GREEN, GREEN_BG); }
		if (game_mining_mode == CHANNEL) { term(3)->print(1,9, "(c) Channel", WHITE, DARKEST_GREEN); } else { term(3)->print(1,9, "(c) Channel", GREEN, GREEN_BG); }
		if (game_mining_mode == RAMP) { term(3)->print(1,10, "(r) Ramp", WHITE, DARKEST_GREEN); } else { term(3)->print(1,10, "(r) Ramp", GREEN, GREEN_BG); }
		if (game_mining_mode == UP) { term(3)->print(1,11, "(u) Up Stairs", WHITE, DARKEST_GREEN); } else { term(3)->print(1,11, "(u) Up Stairs", GREEN, GREEN_BG); }
		if (game_mining_mode == DOWN) { term(3)->print(1,12, "(j) Down Stairs", WHITE, DARKEST_GREEN); } else { term(3)->print(1,12, "(j) Down Stairs", GREEN, GREEN_BG); }
		if (game_mining_mode == UPDOWN) { term(3)->print(1,13, "(i) Up/Down Stairs", WHITE, DARKEST_GREEN); } else { term(3)->print(1,13, "(i) Up/Down Stairs", GREEN, GREEN_BG); }
		if (game_mining_mode == DELETE) { term(3)->print(1,14, "(x) Clear", WHITE, DARKEST_GREEN); } else { term(3)->print(1,14, "(x) Clear", GREEN, GREEN_BG); }

		int mouse_x, mouse_y;
		std::tie(mouse_x, mouse_y) = get_mouse_position();
		const int terminal_x = mouse_x / 8;
		const int terminal_y = mouse_y / 8;

		if (terminal_x >= 0 && terminal_x < term(1)->term_width && terminal_y >= 0 && terminal_y < term(1)->term_height) {
			if (get_mouse_button_state(rltk::button::LEFT)) {
				const int world_x = std::min(clip_left + terminal_x, REGION_WIDTH);
				const int world_y = std::min(clip_top + terminal_y-2, REGION_HEIGHT);
				const int idx = current_region.idx(world_x, world_y, camera_position->region_z);
				if (is_mining_designation_valid(world_x, world_y, camera_position->region_z, game_mining_mode)) {
					switch (game_mining_mode) {
						case DIG : designations->mining[idx] = 1; break;
						case CHANNEL : designations->mining[idx] = 2; break;
						case RAMP : designations->mining[idx] = 3; break;
						case UP : designations->mining[idx] = 4; break;
						case DOWN : designations->mining[idx] = 5; break;
						case UPDOWN : designations->mining[idx] = 6; break;
						case DELETE : designations->mining[idx] = 0; break;
					}
					emit(map_dirty_message{});
				}
			}
		}

	} else {
		term(3)->print(1,3, "(D)igging", GREEN, GREEN_BG);
	}

	if (game_design_mode == BUILDING) {
		term(3)->print(1,4, "Building", WHITE, DARKEST_GREEN);
	} else {
		term(3)->print(1,4, "(B)uilding", GREEN, GREEN_BG);
	}
}