file(GLOB_RECURSE C4C_CODEGEN_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(SORT C4C_CODEGEN_SOURCES)

add_library(c4c_codegen STATIC ${C4C_CODEGEN_SOURCES})
target_include_directories(c4c_codegen
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/llvm"
        "${CMAKE_CURRENT_SOURCE_DIR}/lir"
        "${CMAKE_CURRENT_SOURCE_DIR}/shared"
    PRIVATE
        "${PROJECT_SOURCE_DIR}/src/backend"
        "${PROJECT_SOURCE_DIR}/src/backend/aarch64"
)
target_link_libraries(c4c_codegen PUBLIC c4c_frontend)
target_compile_features(c4c_codegen PUBLIC cxx_std_17)
set_target_properties(c4c_codegen PROPERTIES
    PREFIX ""
    OUTPUT_NAME "c4c_codegen"
)
