# Refactor To-Do List

## Phase 1: Core Systems & Infrastructure
- [x] Create `include/core/types.hpp` (EntityID, Handles)
- [x] Create `include/core/module.hpp` (Base interface)
- [x] Create `include/core/simulator.hpp` (Main class header)
- [x] Create `src/core/simulator.cpp` (Main class implementation)
- [x] Create `include/modules/render_module.hpp` & `src/modules/render_module.cpp` (Basic clearing/setup)
- [x] Update `src/main.cpp` to use `Simulator`
- [x] Update `CMakeLists.txt` to include new Core files
- [x] **Verify:** Compile and run (Window opens/closes, loop works)

## Phase 2: Input & Math Foundation
- [x] Create `include/modules/input_module.hpp` & `src/modules/input_module.cpp`
- [x] Create `include/math/quat.hpp` (Quaternions)
- [x] Create `include/world/transform.hpp` (Transform component)
- [x] Update `Simulator` to include `InputModule`
- [x] Update `CMakeLists.txt` with Input & Math
- [x] **Verify:** Compile and run (Check input polling)

## Phase 3: Simulation (Flight Model)
- [x] Create `include/simulation/aircraft_state.hpp`
- [x] Create `include/simulation/flight_params.hpp`
- [x] Create `include/simulation/flight_model.hpp` & `src/simulation/flight_model.cpp`
- [x] Create simple unit test `tests/test_flight_model.cpp` (optional but recommended)
- [x] **Verify:** Compile flight model logic

## Phase 4: Graphics Infrastructure & Assets
- [x] Create `include/graphics/render_context.hpp`
- [x] Create `include/assets/asset_store.hpp` & `src/assets/asset_store.cpp` (Basic Shader/Mesh storage)
- [x] Create `include/graphics/mesh_builder.hpp` & `src/graphics/mesh_builder.cpp`
- [x] Update `CMakeLists.txt`
- [x] **Verify:** Compile

## Phase 5: Game Modules (Aircraft & Camera)
- [x] Create `include/modules/camera_module.hpp` & `src/modules/camera_module.cpp`
- [x] Create `include/modules/aircraft_module.hpp` & `src/modules/aircraft_module.cpp`
- [x] Update `Simulator` to include Aircraft & Camera
- [x] Update `CMakeLists.txt`
- [x] **Verify:** Compile and run (Render aircraft, camera movement)

## Phase 6: Terrain & Cleanup
- [x] Create `include/modules/terrain_module.hpp` & `src/modules/terrain_module.cpp`
- [x] Update `Simulator` to include Terrain
- [x] Remove old files (`src/game/`, `include/game/`)
- [x] Update `CMakeLists.txt` to remove old sources
- [x] **Verify:** Full simulation run

## Phase 7: Polish & Data (JSON)
- [x] Implement config loading (Stubbed)
- [x] Move shaders to external files