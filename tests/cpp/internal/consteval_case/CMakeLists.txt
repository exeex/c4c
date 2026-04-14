# Consteval integration tests -- extracted from root CMakeLists.txt

add_test(
  NAME cpp_c4_static_assert_if_constexpr_branch_unlocks_later
  COMMAND c4cll "${PROJECT_SOURCE_DIR}/tests/cpp/internal/consteval_case/if_constexpr_branch_unlocks_later.cpp"
)
set_tests_properties(cpp_c4_static_assert_if_constexpr_branch_unlocks_later PROPERTIES
  LABELS "internal;positive_case;cpp;c4_only"
)

add_test(
  NAME cpp_c4_static_assert_multistage_shape_chain
  COMMAND c4cll "${PROJECT_SOURCE_DIR}/tests/cpp/internal/consteval_case/multistage_shape_chain.cpp"
)
set_tests_properties(cpp_c4_static_assert_multistage_shape_chain PROPERTIES
  LABELS "internal;positive_case;cpp;c4_only"
)
