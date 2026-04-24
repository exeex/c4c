# Parse-only dump tests -- extracted from root CMakeLists.txt

add_test(
  NAME cpp_parse_record_member_underlying_type_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/record_member_underlying_type_parse.cpp"
)
set_tests_properties(cpp_parse_record_member_underlying_type_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(value\\)"
)

add_test(
  NAME cpp_parse_record_base_variable_template_value_arg_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/record_base_variable_template_value_arg_parse.cpp"
)
set_tests_properties(cpp_parse_record_base_variable_template_value_arg_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "StructDef\\(struct eastl::has_unique_object_representations_T_int\\)"
)

add_test(
  NAME cpp_parse_template_alias_empty_pack_default_arg_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/template_alias_empty_pack_default_arg_parse.cpp"
)
set_tests_properties(cpp_parse_template_alias_empty_pack_default_arg_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "StructDef\\(struct ns::has_equality\\)"
)

add_test(
  NAME cpp_parse_qualified_type_spelling_rollback_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/qualified_type_spelling_rollback_parse.cpp"
)
set_tests_properties(cpp_parse_qualified_type_spelling_rollback_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(kept\\)"
)

add_test(
  NAME cpp_parse_local_using_alias_statement_probe_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/local_using_alias_statement_probe_parse.cpp"
)
set_tests_properties(cpp_parse_local_using_alias_statement_probe_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Var\\(Alias::make\\).*Decl\\(current\\)|Decl\\(current\\).*Var\\(Alias::make\\)"
)

add_test(
  NAME cpp_parse_record_member_typedef_using_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/record_member_typedef_using_dump_parse.cpp"
)
set_tests_properties(cpp_parse_record_member_typedef_using_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(value\\).*Decl\\(next\\).*Decl\\(current\\).*Decl\\(slot\\).*Decl\\(cb\\)"
)

add_test(
  NAME cpp_parse_record_member_template_friend_cleanup_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/record_member_template_friend_cleanup_parse.cpp"
)
set_tests_properties(cpp_parse_record_member_template_friend_cleanup_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(next\\).*Decl\\(kept\\).*Decl\\(tail\\).*Decl\\(kept\\)"
)

add_test(
  NAME cpp_parse_record_member_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/record_member_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_record_member_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(kept\\)"
)

add_test(
  NAME cpp_parse_record_member_using_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/record_member_using_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_record_member_using_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(kept\\)"
)

add_test(
  NAME cpp_parse_record_member_requires_clause_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/record_member_requires_clause_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_record_member_requires_clause_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(kept\\)"
)

add_test(
  NAME cpp_parse_local_using_alias_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/local_using_alias_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_local_using_alias_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "InvalidStmt.*Decl\\(kept\\)|Decl\\(kept\\).*InvalidStmt"
)

add_test(
  NAME cpp_parse_top_level_storage_class_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_storage_class_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_storage_class_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_asm_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_asm_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_asm_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
)

add_test(
  NAME cpp_parse_gnu_asm_adjacent_string_template_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/gnu_asm_adjacent_string_template_parse.cpp"
)
set_tests_properties(cpp_parse_gnu_asm_adjacent_string_template_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "Asm\\(volatile\\)"
)

add_test(
  NAME cpp_parse_top_level_using_namespace_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_using_namespace_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_using_namespace_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
)

add_test(
  NAME cpp_parse_top_level_using_alias_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_using_alias_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_using_alias_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
)

add_test(
  NAME cpp_parse_top_level_using_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_using_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_using_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_extern_c_empty_block_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_extern_c_empty_block_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_extern_c_empty_block_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_empty_namespace_block_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_empty_namespace_block_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_empty_namespace_block_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_class_recovery_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_class_recovery_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_class_recovery_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "StructDef\\(struct kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_tag_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_tag_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_tag_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty;Program\\(6 items\\)"
)

add_test(
  NAME cpp_parse_top_level_forward_tag_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_forward_tag_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_forward_tag_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty;Program\\(3 items\\)"
)

add_test(
  NAME cpp_parse_top_level_tag_attr_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_tag_attr_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_tag_attr_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_concept_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_concept_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_concept_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_typedef_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_typedef_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_typedef_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_pragma_pack_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_pragma_pack_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_pragma_pack_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_pragma_gcc_visibility_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_top_level_asm_decl_preserves_following_decl_dump
  COMMAND c4cll --parse-only "/workspaces/c4c/tests/cpp/internal/parse_only_case/top_level_asm_decl_preserves_following_decl_parse.cpp"
)
set_tests_properties(cpp_parse_top_level_asm_decl_preserves_following_decl_dump PROPERTIES
  LABELS "internal;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(kept\\)"
  FAIL_REGULAR_EXPRESSION "Empty"
)

add_test(
  NAME cpp_parse_namespace_trailing_return_nested_variadic_alias_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/namespace_trailing_return_nested_variadic_alias_parse.cpp"
)
set_tests_properties(cpp_parse_namespace_trailing_return_nested_variadic_alias_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Function\\(eastl::f\\)"
  FAIL_REGULAR_EXPRESSION "expected=RBRACE got='<eof>'"
)

add_test(
  NAME cpp_parse_namespace_using_nested_visibility_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/namespace_using_nested_visibility_parse.cpp"
)
set_tests_properties(cpp_parse_namespace_using_nested_visibility_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "GlobalVar\\(outer::inner::value\\).*Function\\(bridge::read\\)"
)

add_test(
  NAME cpp_parse_namespace_qualified_local_decl_statement_probe_dump
  COMMAND c4cll --parse-only "${INTERNAL_CPP_TEST_ROOT}/parse_only_case/namespace_qualified_local_decl_statement_probe_parse.cpp"
)
set_tests_properties(cpp_parse_namespace_qualified_local_decl_statement_probe_dump PROPERTIES
  LABELS "internal;positive_case;cpp;parse"
  PASS_REGULAR_EXPRESSION "Decl\\(local\\).*Var\\(ns::value\\)|Var\\(ns::value\\).*Decl\\(local\\)"
)
