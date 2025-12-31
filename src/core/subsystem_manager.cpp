#include "core/subsystem_manager.hpp"
#include <unordered_map>

namespace nuage {

void SubsystemManager::add(std::shared_ptr<Subsystem> subsystem) {
    const std::string name = subsystem ? subsystem->getName() : std::string();
    for (const auto& existing : m_subsystems) {
        if (existing && existing->getName() == name) {
            throw std::runtime_error("Duplicate subsystem name: " + name);
        }
    }
    m_subsystems.push_back(subsystem);
}

void SubsystemManager::initAll() {
    std::unordered_map<std::string, std::size_t> nameToIndex;
    nameToIndex.reserve(m_subsystems.size());
    for (std::size_t i = 0; i < m_subsystems.size(); ++i) {
        if (m_subsystems[i]) {
            nameToIndex[m_subsystems[i]->getName()] = i;
        }
    }

    for (std::size_t i = 0; i < m_subsystems.size(); ++i) {
        auto& sys = m_subsystems[i];
        if (!sys) {
            continue;
        }
        for (const auto& dep : sys->dependencies()) {
            auto it = nameToIndex.find(dep);
            if (it == nameToIndex.end()) {
                throw std::runtime_error("Missing dependency for subsystem " + sys->getName() + ": " + dep);
            }
            if (it->second > i) {
                throw std::runtime_error("Subsystem " + sys->getName() + " must be added after dependency " + dep);
            }
        }
    }

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
