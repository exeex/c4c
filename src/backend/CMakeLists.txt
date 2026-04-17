file(GLOB_RECURSE C4C_BACKEND_BIR_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/bir/*.cpp")
file(GLOB_RECURSE C4C_BACKEND_PREALLOC_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/prealloc/*.cpp")

set(C4C_BACKEND_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/backend.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/target.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/mir/x86/codegen/direct_bir.cpp"
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
target_link_libraries(c4c_backend PUBLIC c4c_frontend c4c_codegen)

target_compile_features(c4c_backend PUBLIC cxx_std_17)
set_target_properties(c4c_backend PROPERTIES
    PREFIX ""
    OUTPUT_NAME "c4c_backend"
)
