#include "map_render_system.hpp"
#include "../raws/raws.hpp"
#include "../messages/messages.hpp"
#include "../game_globals.hpp"
#include <iostream>

using namespace rltk;

void map_render_system::configure() {
	subscribe<renderables_changed_message>([this](renderables_changed_message &msg) {
		this->renderables_changed = true;
	});
	renderables_changed = true;
}

void map_render_system::update(const double duration_ms) {
	if (dirty) update_clipping_rectangle();

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
		--camera_position->region_x;
		if (camera_position->region_x < 0) camera_position->region_x = 0;
		dirty = true;
		update_clipping_rectangle();
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
		++camera_position->region_x;
		if (camera_position->region_x > REGION_WIDTH) camera_position->region_x = REGION_WIDTH;
		dirty = true;
		update_clipping_rectangle();
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
		--camera_position->region_y;
		if (camera_position->region_y < 0) camera_position->region_y = 0;
		dirty = true;
		update_clipping_rectangle();
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
		++camera_position->region_y;
		if (camera_position->region_y > REGION_HEIGHT) camera_position->region_y = REGION_HEIGHT;
		dirty = true;
		update_clipping_rectangle();
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Period) && (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))) {
		--camera_position->region_z;
		if (camera_position->region_z < 0) camera_position->region_z = 0;
		dirty = true;
		update_clipping_rectangle();
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Comma) && (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))) {
		++camera_position->region_z;
		if (camera_position->region_z > REGION_DEPTH) camera_position->region_z = REGION_DEPTH;
		dirty = true;
		update_clipping_rectangle();
	}

	if (clip_left == -1) update_clipping_rectangle();

	if (renderables_changed) {
		for (auto &i : renderables) {
			i.reset();
		}
		each<renderable_t, position_t>([this] (entity_t &entity, renderable_t &render, position_t &pos) {
			this->renderables[current_region.idx(pos.x, pos.y, pos.z)] = rltk::vchar{render.glyph, render.foreground, render.background};
		});
		dirty = true;
	}

	if (dirty) {
		term(1)->clear();

		int Y = 2;
		for (int y=clip_top; y<clip_bottom; ++y) {
			int X = 0;
			for (int x=clip_left; x<clip_right; ++x) {
				term(1)->set_char(X, Y, get_render_char(x, y, camera_position->region_z));
				++X;
			}
			++Y;
		}

		dirty = false;
	}
}

void map_render_system::update_clipping_rectangle() {
	const int term_width = term(1)->term_width;
	const int term_height = term(1)->term_height;
	const int half_width = term_width / 2;
	const int half_height = term_height / 2;

	clip_left = camera_position->region_x - half_width;
	clip_right = camera_position->region_x + half_width;
	clip_top = camera_position->region_y - half_height -2;
	clip_bottom = camera_position->region_y + half_height -2;

	if (clip_left < 0) clip_left = 0;
	if (clip_right > REGION_WIDTH) clip_right = REGION_WIDTH;
	if (clip_top < 0) clip_top = 0;
	if (clip_bottom > REGION_HEIGHT) clip_bottom = REGION_HEIGHT;
}

vchar map_render_system::get_render_char(const int x, const int y, const int z) const {

	int max_dive_depth = 5;
	if (game_master_mode == DESIGN) max_dive_depth = 1;
	int dive_depth = 0;
	boost::optional<vchar> result;

	while (dive_depth < max_dive_depth && !result) {
		const int idx = current_region.idx(x, y, z-dive_depth);

		if (renderables[idx]) {
			result = renderables[idx].get();
		} else {
			result = current_region.tiles[idx].render_as;
		}
		++dive_depth;
	}
	if (!result) {
		return vchar{' ', rltk::colors::GREY, rltk::colors::BLACK};
	} else {
		const int darken_amount = (dive_depth-1) * 40;
		vchar darkened = result.get();
		darkened.foreground = darken(darken_amount, darkened.foreground);
		darkened.background = darken(darken_amount, darkened.background);
		return darkened;
	}
}
