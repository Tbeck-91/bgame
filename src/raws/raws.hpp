#pragma once

#include <vector>
#include <string>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <tuple>
#include <rltk.hpp>
#include <bitset>
#include "../components/item_carried.hpp"
#include "reaction_input.hpp"

struct clothing_t {
	std::string name= "";
	std::vector<std::string> colors;
	std::string slot = "";
	std::string description = "";
	float armor_class = 0.0F;
};

struct profession_t {
	std::string name = "";	
	short strength = 0;
	short dexterity = 0;
	short constitution = 0;
	short intelligence = 0;
	short wisdom = 0;
	short charisma = 0;
	short comeliness = 0;
	short ethics = 0;
	std::vector< std::tuple< uint8_t, std::string, std::string >> starting_clothes;
};

constexpr int NUMBER_OF_ITEM_CATEGORIES = 6;
constexpr int COMPONENT = 0;
constexpr int TOOL_CHOPPING = 1;
constexpr int TOOL_DIGGING = 2;
constexpr int WEAPON_MELEE = 3;
constexpr int WEAPON_RANGED = 4;
constexpr int WEAPON_AMMO = 5;

struct item_def_t {
	std::string tag = "";
	std::string name = "";
	std::string description = "";
	std::bitset<NUMBER_OF_ITEM_CATEGORIES> categories;
	uint8_t glyph;
	rltk::color_t fg;
	rltk::color_t bg;
	int damage_n=0, damage_d=0, damage_mod=0;
	int range = 0;
	std::string ammo;
	int stack_size = 1;
	int initiative_penalty = 0;
	std::string damage_stat = "";
};

enum provides_t { provides_sleep, provides_food, provides_seating, provides_desk, provides_door, 
		provides_wall, provides_floor, provides_stairs_up, provides_stairs_down, provides_stairs_updown,
		provides_ramp, provides_light };

struct building_provides_t {
	provides_t provides;
	int energy_cost = 0;
	int radius = 0;
	rltk::color_t color = rltk::colors::WHITE;
};

struct building_def_t {
	std::string tag = "";
	std::string name = "";
	std::vector<reaction_input_t> components;
	std::pair<std::string, int> skill;
	std::vector<building_provides_t> provides;	
	int width = 1;
	int height = 1;
	std::vector<rltk::vchar> glyphs;
	bool emits_smoke = false;
};


struct reaction_t {
	std::string tag = "";
	std::string name = "";
	std::string workshop = "";
	std::vector<reaction_input_t> inputs;
	std::vector<std::pair<std::string, int>> outputs;
	std::string skill = "";
	int difficulty = 10;
	bool automatic = false;
	int power_drain = 0;
	bool emits_smoke = false;
};

struct plant_t {
	std::string tag = "";
	std::string name = "";
	uint8_t glyph;
	rltk::color_t fg;
	rltk::color_t bg;
	std::string provides = "";
};

struct life_event_template {
	int min_age = 0;
	int max_age = 0;
	std::string description = "";
	int weight = 0;
	short strength = 0;
	short dexterity = 0;
	short constitution = 0;
	short intelligence = 0;
	short wisdom = 0;
	short charisma = 0;
	short comeliness = 0;
	short ethics = 0;
	std::vector<std::string> skills;
	std::vector<std::string> requires_event;
	std::vector<std::string> precludes_event;
};

extern boost::container::flat_map<std::string, clothing_t> clothing_types;
extern std::vector<profession_t> starting_professions;

extern boost::container::flat_map<std::string, item_def_t> item_defs;
extern boost::container::flat_map<std::string, building_def_t> building_defs;
extern boost::container::flat_map<std::string, reaction_t> reaction_defs;
extern boost::container::flat_map<std::string, std::vector<std::string>> reaction_building_defs;

extern boost::container::flat_map<std::string, life_event_template> life_event_defs;

// Initializer
void load_raws();

// Item creation
void spawn_item_on_ground(const int x, const int y, const int z, const std::string &tag, const std::size_t &material);
void spawn_item_in_container(const std::size_t container_id, const std::string &tag, const std::size_t &material);
void spawn_item_carried(const std::size_t holder_id, const std::string &tag, const std::size_t &material, const item_location_t &loc);

// Biome accessors
std::size_t get_plant_idx(const std::string &tag);
plant_t& get_plant_def(const std::size_t &index);
std::vector<plant_t>& get_plant_defs();
