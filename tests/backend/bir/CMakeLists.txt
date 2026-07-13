add_executable(backend_lir_to_bir_interface_test
    "${BACKEND_TEST_ROOT}/bir/backend_lir_to_bir_interface_test.cpp"
)
target_include_directories(backend_lir_to_bir_interface_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_lir_to_bir_interface_test PRIVATE
    c4c_backend
)
set_target_properties(backend_lir_to_bir_interface_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
add_test(
  NAME backend_lir_to_bir_interface
  COMMAND $<TARGET_FILE:backend_lir_to_bir_interface_test>
)
set_tests_properties(backend_lir_to_bir_interface PROPERTIES
  LABELS "backend;bir;interface;cpp"
)
