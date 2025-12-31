#pragma once

#include "core/subsystem.hpp"
#include "core/properties/property_bus.hpp"
#include "core/properties/property_paths.hpp"

namespace nuage {

class SimSubsystem : public Subsystem {
public:
    void init() override {
        PropertyBus::global().set(Properties::Sim::PAUSED, false);
        PropertyBus::global().set(Properties::Sim::TIME, 0.0);
    }

    void update(double dt) override {
        bool paused = PropertyBus::global().get(Properties::Sim::PAUSED, false);
        if (!paused) {
            double time = PropertyBus::global().get(Properties::Sim::TIME, 0.0);
            PropertyBus::global().set(Properties::Sim::TIME, time + dt);
        }
    }

    void shutdown() override {}
    std::string getName() const override { return "Simulation"; }
};

}
