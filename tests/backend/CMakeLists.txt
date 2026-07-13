set(BACKEND_TEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# The active backend test graph is intentionally limited to interfaces owned by
# the post-legacy pipeline.  Archived MIR, preallocation, target, and emission
# tests remain in the tree as references but are not registered or compiled.
add_subdirectory(bir)

get_property(C4C_BACKEND_BIR_TARGETS
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bir"
  PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(c4c_backend_tests
  DEPENDS ${C4C_BACKEND_BIR_TARGETS}
)
