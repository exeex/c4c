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

add_executable(backend_lir_selected_pointer_authority_test
    "${BACKEND_TEST_ROOT}/bir/backend_lir_selected_pointer_authority_test.cpp"
)
target_include_directories(backend_lir_selected_pointer_authority_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_lir_selected_pointer_authority_test PRIVATE
    c4c_codegen
)
set_target_properties(backend_lir_selected_pointer_authority_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
add_test(
  NAME backend_lir_selected_pointer_authority
  COMMAND $<TARGET_FILE:backend_lir_selected_pointer_authority_test>
)
set_tests_properties(backend_lir_selected_pointer_authority PROPERTIES
  LABELS "backend;lir;authority;cpp"
)

add_executable(backend_bir_pipeline_identity_test
    "${BACKEND_TEST_ROOT}/bir/backend_bir_pipeline_identity_test.cpp"
)
target_include_directories(backend_bir_pipeline_identity_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_bir_pipeline_identity_test PRIVATE
    c4c_backend
)
set_target_properties(backend_bir_pipeline_identity_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
add_test(
  NAME backend_bir_pipeline_identity
  COMMAND $<TARGET_FILE:backend_bir_pipeline_identity_test>
)
set_tests_properties(backend_bir_pipeline_identity PROPERTIES
  LABELS "backend;bir;pipeline;identity;cpp"
)

add_executable(backend_bir_execution_control_test
    "${BACKEND_TEST_ROOT}/bir/backend_bir_execution_control_test.cpp"
)
target_include_directories(backend_bir_execution_control_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_bir_execution_control_test PRIVATE
    c4c_backend
)
set_target_properties(backend_bir_execution_control_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
add_test(
  NAME backend_bir_execution_control
  COMMAND $<TARGET_FILE:backend_bir_execution_control_test>
)
set_tests_properties(backend_bir_execution_control PROPERTIES
  LABELS "backend;bir;passes;execution-control;cpp"
)

add_executable(backend_bir_checkpoint_test
    "${BACKEND_TEST_ROOT}/bir/backend_bir_checkpoint_test.cpp"
)
target_include_directories(backend_bir_checkpoint_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_bir_checkpoint_test PRIVATE
    c4c_backend
)
set_target_properties(backend_bir_checkpoint_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
add_test(
  NAME backend_bir_checkpoint
  COMMAND $<TARGET_FILE:backend_bir_checkpoint_test>
)
set_tests_properties(backend_bir_checkpoint PROPERTIES
  LABELS "backend;bir;pipeline;checkpoint;cpp"
)
