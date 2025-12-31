#pragma once

#include "core/properties/property_bus.hpp"

namespace nuage {

class PropertyContext {
public:
    PropertyContext() = default;
    PropertyContext(PropertyBus& global, PropertyBus& local) {
        bind(global, local);
    }

    void bind(PropertyBus& global, PropertyBus& local) {
        m_global = &global;
        m_local = &local;
    }

    PropertyBus& global() { return *m_global; }
    const PropertyBus& global() const { return *m_global; }
    PropertyBus& local() { return *m_local; }
    const PropertyBus& local() const { return *m_local; }

private:
    PropertyBus* m_global = nullptr;
    PropertyBus* m_local = nullptr;
};

}
