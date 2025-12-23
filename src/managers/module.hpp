#pragma once

namespace flightsim {

class Simulator;

class Module {
public:
    virtual ~Module() = default;

    // Called once at startup
    virtual void init(Simulator* sim) { m_sim = sim; }

    // Called every tick - modify state, run logic
    // Not all modules need update (e.g., pure render helpers)
    virtual void update(float dt) {}

    // Called every tick after all updates - draw to screen
    // Not all modules need render (e.g., input, audio)
    virtual void render() {}

    // Called once at shutdown
    virtual void shutdown() {}

    // Optional: for modules that need variable-rate updates
    virtual void fixedUpdate(float fixedDt) {}

    // Module identification
    virtual const char* name() const = 0;

protected:
    Simulator* m_sim = nullptr;
};

}
