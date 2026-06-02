# OpenCASCADE (OCCT) acquisition.
#
# OCCT is *heavy*. Two modes, selected by MACAD_OCCT_SOURCE:
#   prebuilt -> find_package(OpenCASCADE). Point CMake at the SDK via
#               -DOpenCASCADE_DIR=<path>/cmake (the dir containing
#               OpenCASCADEConfig.cmake). Recommended for first bring-up.
#   fetch    -> FetchContent the OCCT sources and build them. Expect a very
#               long first build; only a minimal module set is enabled.
#
# In both cases this module defines an INTERFACE target `macad::occt` that the
# geometry layer links against, so the rest of the build is mode-agnostic.

add_library(macad_occt INTERFACE)
add_library(macad::occt ALIAS macad_occt)

if(MACAD_OCCT_SOURCE STREQUAL "prebuilt")
    find_package(OpenCASCADE REQUIRED)

    if(NOT OpenCASCADE_FOUND)
        message(FATAL_ERROR
            "OpenCASCADE not found. Set -DOpenCASCADE_DIR=<sdk>/cmake or "
            "switch -DMACAD_OCCT_SOURCE=fetch.")
    endif()

    # The toolkits we actually need for Milestone 1: modeling primitives +
    # meshing + topology/BRep data. Add more as later milestones require them.
    set(MACAD_OCCT_LIBS
        TKernel
        TKMath
        TKG2d
        TKG3d
        TKGeomBase
        TKBRep
        TKGeomAlgo
        TKTopAlgo
        TKPrim          # BRepPrimAPI_MakeBox
        TKMesh          # BRepMesh_IncrementalMesh
    )

    target_include_directories(macad_occt INTERFACE ${OpenCASCADE_INCLUDE_DIR})
    target_link_directories(macad_occt INTERFACE ${OpenCASCADE_LIBRARY_DIR})
    target_link_libraries(macad_occt INTERFACE ${MACAD_OCCT_LIBS})

    # Expose OCCT's runtime DLL dir (for copying next to the exe).
    set(MACAD_OCCT_BIN_DIR "${OpenCASCADE_BINARY_DIR}"
        CACHE PATH "OCCT toolkit runtime DLL dir" FORCE)

    # OCCT depends on 3rd-party DLLs (jemalloc, tbb, freetype, ...) shipped in a
    # sibling "3rdparty*" folder that the OCCT config does NOT advertise. Guess
    # it as a sibling of the install prefix; override with -DMACAD_OCCT_3RDPARTY_DIR.
    if(NOT MACAD_OCCT_3RDPARTY_DIR)
        get_filename_component(_occt_parent "${OpenCASCADE_INSTALL_PREFIX}" DIRECTORY)
        file(GLOB _occt_3rdparty_candidates "${_occt_parent}/3rdparty*")
        if(_occt_3rdparty_candidates)
            list(GET _occt_3rdparty_candidates 0 _occt_3rdparty_first)
            set(MACAD_OCCT_3RDPARTY_DIR "${_occt_3rdparty_first}"
                CACHE PATH "OCCT 3rd-party DLL root (contains <lib>/bin/*.dll)")
        endif()
    endif()

    message(STATUS "OCCT (prebuilt) ${OpenCASCADE_VERSION} from ${OpenCASCADE_INCLUDE_DIR}")
    message(STATUS "OCCT 3rd-party DLLs: ${MACAD_OCCT_3RDPARTY_DIR}")

elseif(MACAD_OCCT_SOURCE STREQUAL "fetch")
    include(FetchContent)
    message(WARNING "Building OCCT from source: this can take 30-60+ minutes.")

    # Trim the build to roughly what we need. OCCT's own options gate large
    # subsystems we explicitly do not use (visualization, samples, docs).
    set(BUILD_MODULE_Draw           OFF CACHE BOOL "" FORCE)
    set(BUILD_MODULE_Visualization  OFF CACHE BOOL "" FORCE)
    set(BUILD_MODULE_ApplicationFramework OFF CACHE BOOL "" FORCE)
    set(BUILD_MODULE_DataExchange   OFF CACHE BOOL "" FORCE)
    set(BUILD_DOC_Overview          OFF CACHE BOOL "" FORCE)
    set(USE_FREETYPE                OFF CACHE BOOL "" FORCE)
    set(USE_TK                      OFF CACHE BOOL "" FORCE)
    set(BUILD_LIBRARY_TYPE          "Static" CACHE STRING "" FORCE)

    FetchContent_Declare(occt
        GIT_REPOSITORY https://github.com/Open-Cascade-SAS/OCCT.git
        GIT_TAG        V8_0_0
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(occt)

    set(MACAD_OCCT_LIBS
        TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep
        TKGeomAlgo TKTopAlgo TKPrim TKMesh)

    target_include_directories(macad_occt INTERFACE
        ${occt_SOURCE_DIR}/src
        ${occt_BINARY_DIR}/include)
    target_link_libraries(macad_occt INTERFACE ${MACAD_OCCT_LIBS})

else()
    message(FATAL_ERROR
        "Invalid MACAD_OCCT_SOURCE='${MACAD_OCCT_SOURCE}' (expected prebuilt|fetch)")
endif()
