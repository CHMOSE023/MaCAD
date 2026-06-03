# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MaCAD** is a parametric 3D CAD platform built on OpenCASCADE (OCCT) for geometry and bgfx for self-directed rendering. Currently at **Milestone 1**: render pipeline + geometry kernel integration verified with a tessellated OCCT box rendered in a bgfx/ImGui window.

**Tech Stack**: C++20 · OpenCASCADE 8.0.0 · bgfx · Dear ImGui · GLFW · glm · spdlog · CMake (Ninja Multi-Config generator on Windows)

**Roadmap**: M2 (2D sketch + constraint solver) → M3 (extrude/revolve features) → M4 (parameter system + recompute) → M5 (assembly) → M6 (dynamic plugin loading)

## Layered Architecture

The codebase enforces strict dependency rules to keep layers decoupled:

```
app              (main loop, window, orchestration)
├─ ui            (ImGui panels: toolbar, feature tree, stats)
├─ render        (bgfx renderer, camera, mesh buffers, shaders)
├─ geometry      (OCCT wrapper: primitives, tessellation)
├─ plugin        (command & plugin interfaces, registry)
├─ sketch        (2D parametric model, constraint solver)
└─ core          (logging, types, MeshData contract)
```

**Critical Rule**: `geometry` is the **only layer** that includes OCCT headers. `render` and `ui` must **never** include OCCT; they communicate through the `core::MeshData` data contract (positions, normals, indices).

### Key Architectural Patterns

1. **Data Contract (`core::MeshData`)**
   - Any `TopoDS_Shape` from OCCT is tessellated into a flat mesh (positions/normals/indices)
   - Single upload path: `geometry::Tessellator` → `core::MeshData` → `render::Mesh` → bgfx buffers
   - Keeps OCCT out of render/UI entirely

2. **Command Pattern (`plugin::ICommand`)**
   - Single entry point for user actions (toolbar buttons, plugins, undo/redo, parameter recompute in later milestones)
   - `FunctionCommand` adapter in `app/Application.cpp` wraps lambdas into commands
   - Plugin registry wires commands to the UI

3. **Shape Wrapping (`geometry::Shape`)**
   - Pimpl wrapper around `TopoDS_Shape` to hide OCCT from headers
   - Only `.cpp` files in geometry include OCCT; `.hpp` files fwd-declare only

4. **Sketch Model (`sketch::Sketch`)**
   - Pure 2D: points, entities (line/circle/arc), constraints
   - **Variable model**: each DOF (point x/y, radius, angle) is a slot in flat `m_vars` array
   - Solver treats entire sketch as a vector of unknowns; entities/constraints reference var indices
   - No OCCT, no rendering—pure math + data structures

## Build System

### Prerequisites

1. **OpenCASCADE 8.0.0 SDK** (prebuilt)
   - Download: `occt-combined-with-debug-no-pch.zip` from [OCCT GitHub](https://github.com/Open-Cascade-SAS/OCCT) V8_0_0 release
   - Extract and set `OpenCASCADE_DIR` env var to the `cmake/` dir (containing `OpenCASCADEConfig.cmake`)
     ```powershell
     $env:OpenCASCADE_DIR = "C:\OpenCASCADE\opencascade-8.0.0-vc14-64\cmake"
     ```
   - Add OCCT runtime DLLs to `PATH`: 
     ```
     C:\OpenCASCADE\opencascade-8.0.0-vc14-64\win64\vc14\bin
     C:\OpenCASCADE\3rdparty-vc14-64
     ```

2. **CMake 3.24+**
3. **Visual Studio 2022** with Ninja generator (or MSVC toolchain)

### Building

```powershell
# Configure (downloads all FetchContent deps: glm, spdlog, GLFW, bgfx, ImGui)
cmake --preset MaCAD

# Build Debug
cmake --build --preset x64-debug

# Build Release
cmake --build --preset x64-release

# Run (ensure OCCT DLLs are on PATH first, or use run.bat)
.\build\MaCAD\bin\Debug\MaCAD.exe
```

**Alternative**: To build OCCT from source instead of prebuilt (30–60+ minutes first build):
add `"MACAD_OCCT_SOURCE": "fetch"` as a cacheVariable to a new preset in `CMakePresets.json`.

### Build Gotchas

1. **bgfx submodules on flaky networks**: If FetchContent fails to clone bgfx.cmake with its submodules, pre-populate `.deps/bgfx.cmake/` with a shallow clone:
   ```bash
   git clone --depth 1 --recursive https://github.com/bkaradzic/bgfx.cmake.git .deps/bgfx.cmake
   ```
   The build will detect and reuse it automatically.

2. **Shader compilation**: Milestone 1 embeds only the DX11 (`s_5_0`) shader profile; see `cmake/Shaders.cmake` for multi-backend support.

3. **OCCT runtime DLLs**: Must be on `PATH` at runtime. The included `run.bat` handles this.

## Module Breakdown

### `src/core`
- **Types.hpp**: Foundational type aliases (`vec3`, `mat4`, `StrongId` phantom-type ID, `Result` type)
- **MeshData.hpp**: The geometry→render bridge (vertices, normals, indices)
- **Log.hpp/cpp**: spdlog wrapper + convenience macros (`MACAD_LOG_INFO`, etc.)
- **Parameter.hpp, Document.hpp**: Reserved interfaces for M4+ parameter system

### `src/geometry`
- **Tessellator.hpp/cpp**: `TopoDS_Shape` → `MeshData` via OCCT's `BRepMesh`
- **Primitives.hpp/cpp**: OCCT wrappers (`MakeBox`, etc.)
- **Shape.hpp/cpp**: Pimpl wrapper hiding `TopoDS_Shape` from app/render layers

### `src/render`
- **Renderer.hpp/cpp**: Owns bgfx device, view, program; driven per-frame (beginFrame → drawMesh → endFrame)
- **Camera.hpp/cpp**: Orbital camera (left-drag rotate, middle/right-drag pan, wheel zoom)
- **Mesh.hpp/cpp**: GPU mesh object; uploads `MeshData` to bgfx vertex/index buffers
- **shaders/**: `vs_mesh.sc`, `fs_mesh.sc`, `varying.def.sc` (bgfx shader format)

### `src/ui`
- **ImGuiLayer.hpp/cpp**: Owns ImGui context, GLFW + bgfx backends
- **Panels.hpp/cpp**: Immediate-mode panels (toolbar, feature tree placeholder, stats overlay)
- **SketchView.hpp**: Sketch 2D renderer (M2+)

### `src/plugin`
- **ICommand.hpp**: Interface for user-invokable actions
- **IPlugin.hpp**: Plugin entry point (reserved for M6 dynamic loading)
- **PluginRegistry.hpp/cpp**: Static registry; wires commands to toolbar

### `src/sketch`
- **Sketch.hpp/cpp**: 2D parametric model (points, entities, constraints)
- **SketchPlane.hpp**: Plane definition
- **Solver.hpp/cpp**: Constraint solver (M2+)

### `src/app`
- **Application.hpp/cpp**: Top-level orchestrator; owns window, renderer, UI, plugin registry
- **main.cpp**: Entry point

## Common Development Tasks

### Adding a Built-in Command (M1 Demo)

Commands registered in `Application::registerBuiltinCommands()` follow the `ICommand` interface:

```cpp
m_registry.registerCommand(std::make_shared<FunctionCommand>(
    "macad.geometry.createBox",  // stable id
    "Create Box",                // toolbar label
    [this] { rebuildBox(...); }  // lambda
));
```

The command is automatically added to the toolbar via `Panels::draw()`.

### Adding a Sketch Constraint (M2+)

1. Add the constraint type to `sketch::ConstraintKind` enum
2. Create residual equations in `sketch::Solver` (Newton/least-squares)
3. Add convenience builder in `Sketch` class (e.g., `addDistance()`)
4. Wire UI interaction in `SketchView` (picking, dragging, constraint panel)

### Tessellating a New Geometry Type

1. Build `TopoDS_Shape` in geometry layer using OCCT API
2. Pass to `Tessellator::Tessellate()` → returns `MeshData`
3. Upload to GPU via `render::Mesh::upload()` → rendered automatically

### Testing (Not Structured in M1; Future)

No unit test framework yet. Manual validation is done via:
- Visual inspection of the rendered box in the window
- Stats panel (backend, FPS, vertex/triangle counts)
- Toggling "Create Box" to verify geometry→tessellate→render round-trip re-runs

## Compiler & Code Style

- **C++20 standard** (`cmake_minimum_required(Version 3.24)`)
- **Namespaces**: `macad::` root, then module-specific (`app`, `geometry`, `render`, etc.)
- **Logging**: Always use `MACAD_LOG_*` macros for diagnostics
- **No exceptions at API boundaries**: Use `Result<T>` for error handling
- **Move semantics**: Prefer move constructors/assignment for non-trivial types
- **MSVC compiler options**: `/MP` (parallel compilation) enabled by default

## File Organization Notes

- `.deps/`: FetchContent cache (not committed; auto-generated)
- `build/`: CMake build output (ignored by `.gitignore`)
- `cmake/`: Build system helpers (Dependencies.cmake, OCCT.cmake, Shaders.cmake)
- `third_party/imgui_bgfx/`: Vendored ImGui bgfx rendering backend (no upstream support)
- `docs/ROADMAP.md`: Detailed milestone breakdown (Chinese + English notes)

## Key Points for Future Work

1. **M2 Sketch Solver**: Variable model is ready; add Newton/least-squares solver in `Solver.cpp`
2. **M3 Features**: Sketch profiles feed into `BRepPrimAPI_MakePrism`/`MakeRevol` → tessellate result
3. **M4 Parameters**: Implement `core::Parameter` + DAG + dirty marking for automatic recompute
4. **M5 Assembly**: Multi-document support; assembly constraints & solver
5. **M6 Plugins**: Add `loadPluginLibrary()` to registry; resolve `macadCreatePlugin` factory from DLL

All module boundaries are already scaffolded; each milestone fills in the interaction logic while respecting the dependency graph.
