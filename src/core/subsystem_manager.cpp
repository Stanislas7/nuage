#include "core/subsystem_manager.hpp"

namespace nuage {

void SubsystemManager::add(std::shared_ptr<Subsystem> subsystem) {
    m_subsystems.push_back(subsystem);
}

void SubsystemManager::initAll() {
    for (auto& sys : m_subsystems) {
        std::cout << "[SubsystemManager] Initializing " << sys->getName() << "..." << std::endl;
        sys->init();
    }
}

void SubsystemManager::updateAll(double dt) {
    for (auto& sys : m_subsystems) {
        sys->update(dt);
    }
}

void SubsystemManager::shutdownAll() {
    // Shutdown in reverse order of initialization
    for (auto it = m_subsystems.rbegin(); it != m_subsystems.rend(); ++it) {
        std::cout << "[SubsystemManager] Shutting down " << (*it)->getName() << "..." << std::endl;
        (*it)->shutdown();
    }
    m_subsystems.clear();
}

}
