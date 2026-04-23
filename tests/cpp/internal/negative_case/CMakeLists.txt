# Tests from negative_case/ -- extracted from root CMakeLists.txt
set(_runner "${PROJECT_SOURCE_DIR}/tests/c/internal/cmake/run_negative_case.cmake")

# --- simple negative tests (glob) ---
file(GLOB _neg_srcs CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(src IN LISTS _neg_srcs)
  get_filename_component(stem "${src}" NAME_WE)
  set(test_name "cpp_negative_tests_${stem}")
  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${src}
            -DENFORCE_NEGATIVE=ON
            -P "${_runner}"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "internal;negative_case;cpp")
endforeach()

# --- parser_debug diagnostic format tests ---
add_test(
  NAME cpp_parser_debug_expr_stmt_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_expr_stmt_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_primary
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=token_index=7 token_kind=SEMI token_window=\"[5] KW_return 'return' | [6] PLUS '+' | >>[7] SEMI ';' | [8] RBRACE '}' | [9] EOF ''\""
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> parse_block -> parse_stmt -> parse_expr -> parse_assign_expr -> parse_ternary -> parse_unary -> parse_primary
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_expr_stmt_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_method_or_field_member
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_record_member_dispatch -> try_parse_record_method_or_field_member
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_param_default_rank
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_param_default_rank.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=expected=RPAREN
          -DEXPECT_STACK_SUBSTRING:STRING=parse_param
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_param_default_rank PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_type_like_rank
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_type_like_rank.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_using_member
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_record_member_dispatch -> try_parse_record_type_like_member_dispatch -> try_parse_record_using_member
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_type_like_rank PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_using_alias_leaf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_using_alias_leaf.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_using_member
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_record_member_dispatch -> try_parse_record_type_like_member_dispatch -> try_parse_record_using_member
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_using_alias_leaf PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_record_member_typedef_leaf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_record_member_typedef_leaf.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_typedef_member
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_record_member_dispatch -> try_parse_record_type_like_member_dispatch -> try_parse_record_typedef_member
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_record_member_typedef_leaf PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_qualified_type_top_level_params
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_top_level_params.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_cpp_scoped_base_type -> try_parse_qualified_base_type -> parse_top_level_parameter_list
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_top_level_params PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_qualified_type_template_arg_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_template_arg_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_cpp_scoped_base_type -> try_parse_qualified_base_type -> consume_qualified_type_spelling_with_typename -> consume_qualified_type_spelling -> parse_next_template_argument -> try_parse_template_type_arg -> try_parse_cpp_scoped_base_type -> try_parse_qualified_base_type -> consume_qualified_type_spelling_with_typename -> consume_qualified_type_spelling -> parse_top_level_parameter_list"
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_template_arg_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_qualified_type_dependent_typename_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_dependent_typename_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> parse_next_template_argument -> try_parse_template_type_arg -> try_parse_cpp_scoped_base_type -> parse_dependent_typename_specifier -> parse_top_level_parameter_list
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_dependent_typename_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_qualified_type_typename_spelling_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_typename_spelling_stack.cpp
          -DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list
          -DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> parse_next_template_argument -> try_parse_template_type_arg -> try_parse_cpp_scoped_base_type -> parse_dependent_typename_specifier -> consume_qualified_type_spelling_with_typename -> parse_top_level_parameter_list
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_typename_spelling_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_qualified_type_spelling_stack
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_spelling_stack.cpp
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] stack: -> parse_top_level -> try_parse_cpp_scoped_base_type -> try_parse_qualified_base_type -> consume_qualified_type_spelling_with_typename -> consume_qualified_type_spelling -> parse_next_template_argument -> try_parse_template_type_arg -> parse_top_level_parameter_list"
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_qualified_type_spelling_stack PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_tentative_template_arg_lifecycle
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_spelling_stack.cpp
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=tentative_rollback fn=try_parse_template_type_arg"
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=try_parse_template_type_arg line=15 col=23 detail=\"mode=heavy start="
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_tentative_template_arg_lifecycle PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_tentative_cli_only
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_qualified_type_spelling_stack.cpp
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug-tentative"
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=tentative_rollback fn=try_parse_template_type_arg"
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=try_parse_template_type_arg line=15 col=23 detail=\"mode=heavy start="
          "-DEXPECT_ABSENT_SUBSTRING:STRING=[pdebug] kind=enter fn=parse_top_level"
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_tentative_cli_only PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_sizeof_type_tentative_lite
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_sizeof_type_tentative_lite.cpp
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug-tentative"
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_primary"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_unary"
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_unary line=5 col=24 detail=\"mode=lite start="
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_sizeof_type_tentative_lite PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_if_condition_decl_tentative_lite
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_if_condition_decl_tentative_lite.cpp
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug-tentative"
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_primary"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_stmt"
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_stmt line=6 col=28 detail=\"mode=lite start="
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_if_condition_decl_tentative_lite PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_range_for_tentative_lite
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_range_for_tentative_lite.cpp
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug-tentative"
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_primary"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_stmt"
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_stmt line=8 col=26 detail=\"mode=lite start="
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_range_for_tentative_lite PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_while_condition_decl_tentative_lite
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_while_condition_decl_tentative_lite.cpp
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug-tentative"
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_primary"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_stmt"
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_stmt line=6 col=31 detail=\"mode=lite start="
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_while_condition_decl_tentative_lite PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_switch_condition_decl_tentative_lite
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_switch_condition_decl_tentative_lite.cpp
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug-tentative"
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_primary"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_stmt"
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=[pdebug] kind=tentative_commit fn=parse_stmt line=6 col=32 detail=\"mode=lite start="
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_switch_condition_decl_tentative_lite PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_ctor_init_tentative_lite
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_ctor_init_tentative_lite.cpp
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug-tentative"
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_primary"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=tentative_rollback fn=parse_local_decl line=11 col=14 detail=\"mode=lite start="
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_ctor_init_tentative_lite PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_injected_template_base_instantiation
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_injected_template_base_instantiation.cpp
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=injected_parse_begin fn=parse_base_type"
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=[pdebug] kind=injected_parse_end fn=parse_base_type"
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_injected_template_base_instantiation PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)

add_test(
  NAME cpp_parser_debug_injected_cli_only
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${INTERNAL_CPP_TEST_ROOT}/negative_case/parser_debug_injected_template_base_instantiation.cpp
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug-injected"
          "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=parse_top_level_parameter_list"
          "-DEXPECT_STACK_SUBSTRING:STRING=[pdebug] kind=injected_parse_begin fn=parse_base_type"
          "-DEXPECT_CONTEXT_SUBSTRING:STRING=[pdebug] kind=injected_parse_end fn=parse_base_type"
          "-DEXPECT_ABSENT_SUBSTRING:STRING=[pdebug] kind=enter fn=parse_top_level"
          -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_parser_debug_case.cmake"
)
set_tests_properties(cpp_parser_debug_injected_cli_only PROPERTIES
  LABELS "internal;negative_case;cpp;diagnostic_format"
)
