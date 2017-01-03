#pragma once

#include <rltk.hpp>
#include "../utils/dijkstra_map.hpp"
#include "../components/position.hpp"

extern dijkstra_map huntables_map;

class distance_map_system : public rltk::base_system {
public:
    virtual void update(const double duration_ms) override final;
    virtual void configure() override final;
private:
    bool update_huntables = true;
};
