file(GLOB_RECURSE C4C_BACKEND_BIR_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/bir/*.cpp")
file(GLOB_RECURSE C4C_BACKEND_PREALLOC_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/prealloc/*.cpp")

set(C4C_BACKEND_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/backend.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/abi/x86_target_abi.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/api/x86_codegen_api.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/api/x86_codegen_api_assemble.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/core/x86_codegen_output.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/lowering/call_lowering.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/lowering/comparison_lowering.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/lowering/frame_lowering.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/lowering/memory_lowering.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/lowering/return_lowering.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/lowering/scalar_lowering.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/module/module_data_emit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/module/module_emit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/prepared_countdown_render.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/prepared_local_slot_render.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/prepared_param_zero_render.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/prepared_module_emit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/route_debug.cpp"
    ${C4C_BACKEND_BIR_SOURCES}
    ${C4C_BACKEND_PREALLOC_SOURCES}
)
list(SORT C4C_BACKEND_SOURCES)

add_library(c4c_backend STATIC ${C4C_BACKEND_SOURCES})
target_include_directories(c4c_backend
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/bir"
        "${CMAKE_CURRENT_SOURCE_DIR}/prealloc"
        "${PROJECT_SOURCE_DIR}/src/frontend"
        "${PROJECT_SOURCE_DIR}/src/frontend/lexer"
        "${PROJECT_SOURCE_DIR}/src/frontend/parser"
        "${PROJECT_SOURCE_DIR}/src/frontend/sema"
        "${PROJECT_SOURCE_DIR}/src/frontend/hir"
        "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor"
        "${PROJECT_SOURCE_DIR}/src/codegen"
        "${PROJECT_SOURCE_DIR}/src/codegen/lir"
        "${PROJECT_SOURCE_DIR}/src/codegen/shared"
)
target_link_libraries(c4c_backend PUBLIC c4c_frontend c4c_codegen c4c_target_profile)

target_compile_features(c4c_backend PUBLIC cxx_std_17)
set_target_properties(c4c_backend PROPERTIES
    PREFIX ""
    OUTPUT_NAME "c4c_backend"
)
