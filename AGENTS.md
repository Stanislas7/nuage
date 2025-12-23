# Nuage Flight Simulator

## Build Commands
```bash
./run.sh          # Build and run
cmake -B build && cmake --build build   # Build only
./build/nuage     # Run after build
```

## Code Style
- PascalCase for classes: `AircraftManager`, `PropertyBus`
- camelCase for methods/variables: `getAirDensity()`, `m_aircraft`
- Prefix members with `m_`
- hpp/cpp in same folder
- No `using namespace std;` in headers

## Key Paths
- `app/` — Entry point, main loop
- `managers/` — All manager classes
- `aircraft/systems/` — IAircraftSystem implementations
- `assets/configs/` — Aircraft JSON configs
