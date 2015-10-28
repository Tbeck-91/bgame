#pragma once

#include "base_system.h"
#include "../../../game/world/world.h"
#include "../ecs.h"
#include "../components/position_component.h"
#include "../../backends/command_manager.h"

namespace engine {
namespace ecs {

class camera_system : public base_system {
    virtual void tick(const double &duration_ms);
};

}
}