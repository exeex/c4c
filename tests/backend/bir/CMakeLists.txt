add_executable(backend_bir_producer_view_contract_test
    "${BACKEND_TEST_ROOT}/bir/backend_bir_producer_view_contract_test.cpp"
)
target_include_directories(backend_bir_producer_view_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_compile_definitions(backend_bir_producer_view_contract_test PRIVATE
    C4C_BIR_PRODUCER_VIEW_HEADER="${PROJECT_SOURCE_DIR}/src/backend/bir/bir_producer_view.hpp"
)
target_link_libraries(backend_bir_producer_view_contract_test PRIVATE
    c4c_backend
)
set_target_properties(backend_bir_producer_view_contract_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
add_test(
  NAME backend_bir_producer_view_contract
  COMMAND $<TARGET_FILE:backend_bir_producer_view_contract_test>
)
set_tests_properties(backend_bir_producer_view_contract PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_bir_select_dependency_view_contract_test
    "${BACKEND_TEST_ROOT}/bir/backend_bir_select_dependency_view_contract_test.cpp"
)
target_include_directories(backend_bir_select_dependency_view_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_bir_select_dependency_view_contract_test PRIVATE
    c4c_backend
)
set_target_properties(backend_bir_select_dependency_view_contract_test PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
)
add_test(NAME backend_bir_select_dependency_view_contract
  COMMAND $<TARGET_FILE:backend_bir_select_dependency_view_contract_test>)
set_tests_properties(backend_bir_select_dependency_view_contract PROPERTIES
  LABELS "backend;internal;cpp")

add_executable(backend_bir_memory_publication_view_contract_test
    "${BACKEND_TEST_ROOT}/bir/backend_bir_memory_publication_view_contract_test.cpp"
)
target_include_directories(backend_bir_memory_publication_view_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_compile_definitions(backend_bir_memory_publication_view_contract_test PRIVATE
    C4C_BIR_MEMORY_VIEW_HEADER="${PROJECT_SOURCE_DIR}/src/backend/bir/bir_memory_access_view.hpp"
    C4C_BIR_PUBLICATION_VIEW_HEADER="${PROJECT_SOURCE_DIR}/src/backend/bir/bir_publication_view.hpp"
)
target_link_libraries(backend_bir_memory_publication_view_contract_test PRIVATE c4c_backend)
set_target_properties(backend_bir_memory_publication_view_contract_test PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
)
add_test(NAME backend_bir_memory_publication_view_contract
  COMMAND $<TARGET_FILE:backend_bir_memory_publication_view_contract_test>)
set_tests_properties(backend_bir_memory_publication_view_contract PROPERTIES
  LABELS "backend;internal;cpp")

add_executable(backend_bir_call_comparison_view_contract_test
    "${BACKEND_TEST_ROOT}/bir/backend_bir_call_comparison_view_contract_test.cpp"
)
target_include_directories(backend_bir_call_comparison_view_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_compile_definitions(backend_bir_call_comparison_view_contract_test PRIVATE
    C4C_BIR_CALL_VIEW_HEADER="${PROJECT_SOURCE_DIR}/src/backend/bir/bir_call_boundary_view.hpp"
    C4C_BIR_COMPARISON_VIEW_HEADER="${PROJECT_SOURCE_DIR}/src/backend/bir/bir_comparison_view.hpp"
)
target_link_libraries(backend_bir_call_comparison_view_contract_test PRIVATE c4c_backend)
set_target_properties(backend_bir_call_comparison_view_contract_test PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
)
add_test(NAME backend_bir_call_comparison_view_contract
  COMMAND $<TARGET_FILE:backend_bir_call_comparison_view_contract_test>)
set_tests_properties(backend_bir_call_comparison_view_contract PROPERTIES
  LABELS "backend;internal;cpp")

add_executable(backend_bir_return_control_flow_view_contract_test
    "${BACKEND_TEST_ROOT}/bir/backend_bir_return_control_flow_view_contract_test.cpp"
)
target_include_directories(backend_bir_return_control_flow_view_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_compile_definitions(backend_bir_return_control_flow_view_contract_test PRIVATE
    C4C_BIR_RETURN_VIEW_HEADER="${PROJECT_SOURCE_DIR}/src/backend/bir/bir_return_view.hpp"
    C4C_BIR_CONTROL_FLOW_VIEW_HEADER="${PROJECT_SOURCE_DIR}/src/backend/bir/bir_control_flow_view.hpp"
)
target_link_libraries(backend_bir_return_control_flow_view_contract_test PRIVATE c4c_backend)
set_target_properties(backend_bir_return_control_flow_view_contract_test PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
)
add_test(NAME backend_bir_return_control_flow_view_contract
  COMMAND $<TARGET_FILE:backend_bir_return_control_flow_view_contract_test>)
set_tests_properties(backend_bir_return_control_flow_view_contract PROPERTIES
  LABELS "backend;internal;cpp")

get_property(C4C_BIR_BACKEND_TESTS DIRECTORY PROPERTY TESTS)
foreach(test_name IN LISTS C4C_BIR_BACKEND_TESTS)
  set_property(TEST "${test_name}" APPEND PROPERTY LABELS bir)
endforeach()
