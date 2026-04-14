file(GLOB_RECURSE C4C_FRONTEND_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(SORT C4C_FRONTEND_SOURCES)

add_library(c4c_frontend STATIC ${C4C_FRONTEND_SOURCES})
target_include_directories(c4c_frontend
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/lexer"
        "${CMAKE_CURRENT_SOURCE_DIR}/parser"
        "${CMAKE_CURRENT_SOURCE_DIR}/sema"
        "${CMAKE_CURRENT_SOURCE_DIR}/hir"
        "${CMAKE_CURRENT_SOURCE_DIR}/preprocessor"
)
target_compile_definitions(c4c_frontend
    PUBLIC
        ENABLE_HEAVY_TENTATIVE_SNAPSHOT=$<IF:$<BOOL:${ENABLE_HEAVY_TENTATIVE_SNAPSHOT}>,1,0>
)
target_compile_features(c4c_frontend PUBLIC cxx_std_17)
set_target_properties(c4c_frontend PROPERTIES
    PREFIX ""
    OUTPUT_NAME "c4c_frontend"
)
