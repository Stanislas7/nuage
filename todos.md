# Nuage Refactor TODO

## Phase 0: Spike (Validate Architecture)

**Why:** Before restructuring 20+ files, prove the PropertyBus pattern works. If it feels wrong, you've only wasted 1 hour instead of 10.

**What to do:**
1. Create `PropertyBus.hpp/.cpp` in current structure (don't move files yet)
2. Create one test system: `TestEngineSystem` that reads `input/throttle`, writes `engine/thrust`
3. Wire it into existing `FlightModel::update()` temporarily
4. Run the sim — does throttle still work? Does the data flow make sense?
5. If yes → proceed to Phase 1. If no → rethink before committing.

- [x] Create `PropertyBus` class (simple `std::unordered_map<string, double>`)
- [x] Create `TestEngineSystem` that reads/writes to PropertyBus
- [x] Temporarily wire into `FlightModel` to test data flow
- [x] Verify throttle → thrust still works
- [x] Delete test code OR keep it as the real `EngineSystem`

---

## Phase 1: Folder Restructure
- [x] Create new folder structure (`app/`, `managers/`, `aircraft/`, etc.)
- [x] Move `math/` files (Vec3, Mat4, Quat) — merge hpp/cpp together
- [x] Move `graphics/` files (Mesh, Shader, MeshBuilder)
- [x] Delete old `include/` and `src/` split structure
- [x] Update `CMakeLists.txt` with new paths

## Phase 2: Core Managers
- [x] Create `App.hpp/.cpp` (rename from Simulator)
- [x] Implement fixed timestep loop in `App::run()`
- [x] Create `InputManager.hpp/.cpp` (from InputModule)
- [x] Create `AssetManager.hpp/.cpp` (from AssetStore)
- [x] Create `CameraManager.hpp/.cpp` (from CameraModule)
- [x] Create `WorldManager.hpp/.cpp` (from TerrainModule)
- [x] Create `AircraftManager.hpp/.cpp` (from AircraftModule)
- [x] Create `AtmosphereManager.hpp/.cpp` (new)

## Phase 3: PropertyBus
- [x] Implement `PropertyBus.hpp/.cpp` (enhanced with Vec3/Quat helpers)
- [ ] Add unit tests for PropertyBus (deferred to end)
- [x] Define standard property paths (documented in PropertyPaths.hpp)
- [x] Add `getSystem<T>()` to Aircraft for clean API access

## Phase 4: Aircraft Systems
- [x] Create `IAircraftSystem.hpp` interface
- [x] Create `Aircraft.hpp/.cpp` container class
- [x] Extract `FlightDynamics` system from FlightModel
- [x] Create `EngineSystem` (thrust, N1, fuel flow)
- [x] Create `FuelSystem` (tank management)
- [x] Create `EnvironmentSystem` (pushes atmosphere data to PropertyBus)
- [x] Wire systems to read/write PropertyBus
- [ ] Load aircraft config from JSON (deferred - using defaults for now)

## Phase 5: Integration
- [x] Connect `FlightDynamics` → `AtmosphereManager::getAirDensity()` (via EnvironmentSystem push)
- [x] Connect `CameraManager` → follows player aircraft
- [x] Connect `InputManager` → writes to aircraft PropertyBus
- [x] Test full loop: input → physics → render

## Phase 6: Cleanup & Code Quality
- [x] Update FlightDynamics to use Properties:: constants
- [x] Update EngineSystem to use Properties:: constants
- [x] Update FuelSystem to use Properties:: constants
- [x] Update Aircraft to use Properties:: constants
- [x] Implement exponential decay for N1 spool rate (frame-rate independent)
- [x] EnvironmentSystem pushes density to PropertyBus (testable EngineSystem)
- [x] Final build test - builds successfully

## Phase 4: Aircraft Systems
- [ ] Create `IAircraftSystem.hpp` interface
- [ ] Create `Aircraft.hpp/.cpp` container class
- [ ] Extract `FlightDynamics` system from FlightModel
- [ ] Create `EngineSystem` (thrust, N1, fuel flow)
- [ ] Create `FuelSystem` (tank management)
- [ ] Wire systems to read/write PropertyBus
- [ ] Load aircraft config from JSON

## Phase 5: Integration
- [ ] Connect `FlightDynamics` → `AtmosphereManager::getAirDensity()`
- [ ] Connect `CameraManager` → follows player aircraft
- [ ] Connect `InputManager` → writes to aircraft PropertyBus
- [ ] Test full loop: input → physics → render

## Phase 6: Cleanup
- [ ] Remove old `FlightModel` class
- [ ] Remove old Module base class
- [ ] Remove old `simulation/` folder
- [ ] Update all `#include` paths
- [ ] Final build test

---

## Quick Wins (Do Anytime)
- [ ] Add `AtmosphereManager::getAirDensity()` with ISA model
- [ ] Add origin shift check in `WorldManager`
- [ ] Add `CameraMode::Cockpit` view
- [ ] Add keyboard shortcut to cycle camera modes
