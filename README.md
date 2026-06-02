# MaCAD

A parametric 3D CAD platform built on a self-rendered viewport. Geometry is
powered by **OpenCASCADE (OCCT)**; rendering is built entirely on **bgfx** .

> **Status: Milestone 1 ✅** — render pipeline + geometry kernel passthrough.
> An OCCT box is tessellated and drawn through bgfx in a GLFW window with an
> ImGui overlay. Sketch, constraints, feature tree, parameters, assembly and
> plugins are scaffolded as module/interface boundaries only.
>
> 📍 Full roadmap & milestone tracking: [docs/ROADMAP.md](docs/ROADMAP.md)

## Tech stack

C++20 · OpenCASCADE · bgfx · Dear ImGui · GLFW · glm · spdlog · CMake (VS)

## Architecture

```
app  ──▶ ui · render · geometry · plugin · core
geometry ──▶ core + OCCT      (only layer that includes OCCT)
render/ui ──▶ core            (must NOT include OCCT)
```

Geometry and rendering are decoupled through one data contract,
[`core::MeshData`](src/core/MeshData.hpp): any `TopoDS_Shape` is tessellated
([`geometry::Tessellator`](src/geometry/Tessellator.cpp)) into `MeshData`, which
the renderer uploads ([`render::Mesh`](src/render/Mesh.cpp)). User actions go
through the [`ICommand`](src/plugin/ICommand.hpp) contract — the same one
plugins will use (M6).

| Layer | Path | Responsibility |
|-------|------|----------------|
| core | `src/core` | logging, types, `MeshData`, reserved Parameter/Document |
| geometry | `src/geometry` | OCCT wrapper: primitives + tessellation |
| render | `src/render` | bgfx device, camera, mesh buffers, shaders |
| ui | `src/ui` | ImGui layer + panels (toolbar/feature tree/stats) |
| plugin | `src/plugin` | command + plugin interfaces (static registry) |
| app | `src/app` | window, main loop, wiring |

## Dependencies

`bgfx.cmake`, GLFW, glm, spdlog and Dear ImGui are fetched automatically via
CMake `FetchContent` (see [`cmake/Dependencies.cmake`](cmake/Dependencies.cmake)).

**OCCT** is heavy, so it is handled separately
([`cmake/OCCT.cmake`](cmake/OCCT.cmake)) with two modes via `MACAD_OCCT_SOURCE`:

- `prebuilt` (default, recommended): uses an installed OCCT SDK. Set the env
  var or cache var `OpenCASCADE_DIR` to the SDK's `cmake/` directory (the one
  containing `OpenCASCADEConfig.cmake`).
- `fetch`: builds OCCT from source via FetchContent. **Expect a 30–60+ minute
  first build.**

## Build (Windows, Visual Studio 2022 Ninja )

```powershell
# 1. Point at a prebuilt OCCT 8.0.0 SDK
#    (occt-combined-with-debug-no-pch.zip from the V8_0_0 GitHub release,
#     extracted so that <root>\cmake\OpenCASCADEConfig.cmake exists)

# 环境变量或 CMake 缓存变量均可;环境变量优先级更高且对 IDE 友好
$env:OpenCASCADE_DIR = "C:\OpenCASCADE\opencascade-8.0.0-vc14-64\cmake"

# 2. Configure + build
cmake --preset x64-debug
cmake --build --preset x64-debug

# 3. 环境变量PATH添加： OCCT SDK的bin目录（包含运行时DLL）
Path = C:\OpenCASCADE\opencascade-8.0.0-vc14-64\win64\vc14\bin
其他运行时dll：C:\OpenCASCADE\3rdparty-vc14-64 

```

To build OCCT from source instead, use the `x64-debug-occt-source` preset.

### Notes / gotchas (from first bring-up)

- **bgfx submodules**: bgfx (bgfx/bx/bimg) is pulled as git submodules of
  `bgfx.cmake`. On flaky networks the in-CMake clone fails. A pre-populated
  local copy at `.deps/bgfx.cmake` (shallow clone with submodules) is picked up
  automatically by [`cmake/Dependencies.cmake`](cmake/Dependencies.cmake).
- **Backend**: Milestone 1 embeds only the DX11 (`s_5_0`) shader profile and
  forces the Direct3D11 bgfx backend. Multi-backend support means compiling more
  profiles in [`cmake/Shaders.cmake`](cmake/Shaders.cmake).
- **OCCT runtime DLLs** must be on `PATH` (handled by `run.bat`).

## What you should see

A lit cube in the viewport that you can orbit (left-drag), pan
(middle/right-drag) and zoom (wheel). ImGui shows a **Toolbar** with a
*Create Box* button (rebuilds the solid at a new size — proving the
OCCT → tessellate → bgfx round-trip re-runs on command), a placeholder
**Feature Tree**, and a **Stats** panel (backend, FPS, vertex/triangle counts).

## Roadmap

M2 sketch + constraint solver · M3 extrude/revolve features + feature tree ·
M4 parameter system + dependency recompute · M5 assembly · M6 dynamic plugin
loading. Module and interface boundaries for these already exist in the tree.
