#pragma once

#include <rltk.hpp>

using namespace rltk;

struct grazer_ai {
	grazer_ai() {}
	grazer_ai(const int init_mod) : initiative_modifier(init_mod) {}
	int initiative = 0;
	int initiative_modifier = 0;

	std::size_t serialization_identity = 20;

	void save(std::ostream &lbfile) {
		serialize(lbfile, initiative);
		serialize(lbfile, initiative_modifier);
	}

	static grazer_ai load(std::istream &lbfile) {
		grazer_ai c;
		deserialize(lbfile, c.initiative);
		deserialize(lbfile, c.initiative_modifier);
		return c;
	}
};