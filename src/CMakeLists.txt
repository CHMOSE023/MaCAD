# Module layers. Dependency direction (enforced via link rules):
#   app -> {ui, render, geometry, sketch, core, plugin}
#   geometry -> {core, macad::occt}   (only layer that touches OCCT)
#   sketch   -> core                  (pure 2D model + solver; no OCCT/bgfx)
#   render/ui -> core                 (must NOT include OCCT)

add_subdirectory(core)
add_subdirectory(geometry)
add_subdirectory(sketch)
add_subdirectory(render)
add_subdirectory(plugin)
add_subdirectory(ui)
add_subdirectory(app)
