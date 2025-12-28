#pragma once

#include "core/subsystem.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <typeinfo>

namespace nuage {

class SubsystemManager {
public:
    void add(std::shared_ptr<Subsystem> subsystem);
    
    // Initialize all registered subsystems
    void initAll();

    // Update all subsystems
    void updateAll(double dt);

    // Shutdown all subsystems in reverse order
    void shutdownAll();

    template<typename T>
    std::shared_ptr<T> get() {
        for (auto& sys : m_subsystems) {
            auto casted = std::dynamic_pointer_cast<T>(sys);
            if (casted) return casted;
        }
        return nullptr;
    }

    template<typename T>
    std::shared_ptr<T> getRequired() {
        auto system = get<T>();
        if (!system) {
            throw std::runtime_error(std::string("Required subsystem not found: ") + typeid(T).name());
        }
        return system;
    }

private:
    std::vector<std::shared_ptr<Subsystem>> m_subsystems;
};

}
