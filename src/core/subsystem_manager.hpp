#pragma once

#include "core/subsystem.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>

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

private:
    std::vector<std::shared_ptr<Subsystem>> m_subsystems;
};

}
