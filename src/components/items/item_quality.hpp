#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include "../../bengine/ecs_impl.hpp"
#include <string>

namespace item_quality {
	constexpr uint8_t AWFUL = 1;
	constexpr uint8_t POOR = 2;
	constexpr uint8_t AVERAGE = 3;
	constexpr uint8_t ABOVE_AVERAGE = 4;
	constexpr uint8_t GOOD = 5;
	constexpr uint8_t GREAT = 6;
	constexpr uint8_t MASTERWORK = 7;
}

struct item_quality_t {

	item_quality_t() {}
	item_quality_t(const uint8_t q) : quality(q) {}
	uint8_t quality = 3;

	std::string get_quality_text() {
		switch (quality) {
		case 1: return std::string("awful");
		case 2: return std::string("poor");
		case 3: return std::string("average");
		case 4: return std::string("above average");
		case 5: return std::string("good");
		case 6: return std::string("great");
		case 7: return std::string("masterwork");
		}
	}

	template<class Archive>
	void serialize(Archive & archive)
	{
		// Nothing to save
		archive( quality );
	}
};

CEREAL_REGISTER_TYPE(bengine::impl::component_store_t<bengine::impl::component_t<item_quality_t>>)
