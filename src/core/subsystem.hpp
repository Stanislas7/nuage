#pragma once

#include <string>

namespace nuage {

class Subsystem {
public:
    virtual ~Subsystem() = default;

    // Called once at startup
    virtual void init() = 0;

    // Called every frame
    virtual void update(double dt) = 0;

    // Called on shutdown
    virtual void shutdown() = 0;

    // Unique name for debugging/logging
    virtual std::string getName() const = 0;
};

}
