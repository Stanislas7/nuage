# Codebase Diagnosis: Structural Organization & UI Extraction

**Date:** December 26, 2025
**Scope:** Folder Structure, Pause Logic Extraction, and Maintainability

## 1. Logic Dispersion

While the "Flight Session" extraction was a major step, several areas still suffer from "App-heavy" logic or lack clear categorization.

### Problems
*   **Mixed UI Responsibilities**: `App` still manually orchestrates the "Pause" state by calling `UIManager` setters.
*   **Flat Directory Structure**: `src/core` and `src/graphics` are becoming "catch-all" folders for both base primitives and specialized logic.
*   **Initialization Flow**: `main.cpp` is cleaner, but it still has to know too much about config paths.

## 2. Refactoring Plan: Modular Architecture

### Phase 1: UI Modularization
- [ ] Create `src/ui/overlays/pause_overlay.hpp`.
- [ ] Move `refreshPauseUI` logic into this new component.
- [ ] Make the "Overlay" system in `UIManager` more generic so it can host different screens (Pause, Menu, Game Over).

### Phase 2: Folder Reorganization
- [ ] **Properties**: Move `src/core/property_*` to `src/core/properties/`.
- [ ] **Session**: Move `src/core/flight_*` to `src/core/session/`.
- [ ] **Renderers**: Move specialized renderers (`Skybox`, `TerrainRenderer`) to `src/graphics/renderers/`.

### Phase 3: Flight Bootstrapper
- [ ] Create a `FlightBootstrapper` or `Launcher` helper to handle the JSON config resolution currently sitting in `main.cpp`.

---

## Summary of Action Plan

1.  **Extract Pause Logic**: Decouple the UI state from the main loop.
2.  **Clean up Folders**: Use specialized subdirectories.
3.  **Standardize**: Ensure all headers use consistent naming and paths.
