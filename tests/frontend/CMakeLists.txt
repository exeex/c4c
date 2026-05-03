set(frontend_test_targets frontend_lexer_tests frontend_parser_tests
                          frontend_parser_lookup_authority_tests
                          frontend_hir_lookup_tests
                          cpp_hir_template_type_arg_encoding_test
                          cpp_hir_static_member_base_metadata_test
                          cpp_hir_member_typedef_binding_metadata_test
                          cpp_hir_nested_member_typedef_record_def_metadata_test
                          cpp_hir_member_typedef_readiness_metadata_test
                          cpp_hir_member_typedef_origin_binding_metadata_test
                          cpp_hir_member_typedef_nested_readiness_metadata_test
                          cpp_hir_member_typedef_realized_owner_metadata_test
                          cpp_hir_parser_core_nominal_typespec_metadata_test
                          cpp_hir_parser_core_typedef_chain_metadata_test
                          cpp_hir_parser_core_struct_like_typedef_metadata_test
                          cpp_hir_parser_core_record_ctor_metadata_test
                          cpp_hir_parser_core_producer_metadata_test
                          cpp_hir_parser_declarations_residual_metadata_test
                          cpp_hir_parser_expressions_residual_metadata_test
                          cpp_hir_parser_support_residual_metadata_test
                          cpp_hir_parser_type_base_residual_metadata_test
                          cpp_hir_parser_type_helper_metadata_test
                          cpp_hir_parser_type_helper_residual_metadata_test
                          cpp_hir_template_pattern_match_metadata_test
                          cpp_hir_template_canonical_primary_origin_metadata_test
                          cpp_hir_template_realize_struct_metadata_test
                          cpp_hir_template_recover_identity_metadata_test
                          cpp_hir_value_args_residual_metadata_test
                          frontend_lir_global_type_ref_test
                          frontend_lir_function_signature_type_ref_test
                          frontend_lir_extern_decl_type_ref_test
                          frontend_lir_call_type_ref_test)
if(ENABLE_C4C_BACKEND)
  list(APPEND frontend_test_targets frontend_hir_tests)
endif()

foreach(target IN LISTS frontend_test_targets)
  add_executable(${target} "${CMAKE_CURRENT_SOURCE_DIR}/${target}.cpp")
  target_link_libraries(${target} PRIVATE c4c_frontend)
  set_target_properties(${target} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
endforeach()

target_link_libraries(frontend_lir_global_type_ref_test PRIVATE c4c_codegen)
target_link_libraries(frontend_lir_function_signature_type_ref_test PRIVATE c4c_codegen)
target_link_libraries(frontend_lir_extern_decl_type_ref_test PRIVATE c4c_codegen)
target_link_libraries(frontend_lir_call_type_ref_test PRIVATE c4c_codegen)

if(ENABLE_C4C_BACKEND)
  target_link_libraries(frontend_hir_tests PRIVATE c4c_codegen c4c_backend)
  add_test(NAME frontend_hir_tests    COMMAND frontend_hir_tests)
  set_tests_properties(frontend_hir_tests PROPERTIES LABELS "internal;hir")
endif()

add_test(NAME frontend_lexer_tests  COMMAND frontend_lexer_tests)
add_test(NAME frontend_parser_tests COMMAND frontend_parser_tests)
add_test(NAME frontend_parser_lookup_authority_tests
         COMMAND frontend_parser_lookup_authority_tests)
add_test(NAME frontend_hir_lookup_tests COMMAND frontend_hir_lookup_tests)
add_test(NAME cpp_hir_template_type_arg_encoding_structured_metadata
         COMMAND cpp_hir_template_type_arg_encoding_test)
add_test(NAME cpp_hir_static_member_base_structured_metadata
         COMMAND cpp_hir_static_member_base_metadata_test)
add_test(NAME cpp_hir_member_typedef_binding_structured_metadata
         COMMAND cpp_hir_member_typedef_binding_metadata_test)
add_test(NAME cpp_hir_nested_member_typedef_record_def_structured_metadata
         COMMAND cpp_hir_nested_member_typedef_record_def_metadata_test)
add_test(NAME cpp_hir_member_typedef_readiness_structured_metadata
         COMMAND cpp_hir_member_typedef_readiness_metadata_test)
add_test(NAME cpp_hir_member_typedef_origin_binding_structured_metadata
         COMMAND cpp_hir_member_typedef_origin_binding_metadata_test)
add_test(NAME cpp_hir_member_typedef_nested_readiness_structured_metadata
         COMMAND cpp_hir_member_typedef_nested_readiness_metadata_test)
add_test(NAME cpp_hir_member_typedef_realized_owner_structured_metadata
         COMMAND cpp_hir_member_typedef_realized_owner_metadata_test)
add_test(NAME cpp_hir_parser_core_nominal_typespec_structured_metadata
         COMMAND cpp_hir_parser_core_nominal_typespec_metadata_test)
add_test(NAME cpp_hir_parser_core_typedef_chain_structured_metadata
         COMMAND cpp_hir_parser_core_typedef_chain_metadata_test)
add_test(NAME cpp_hir_parser_core_struct_like_typedef_structured_metadata
         COMMAND cpp_hir_parser_core_struct_like_typedef_metadata_test)
add_test(NAME cpp_hir_parser_core_record_ctor_structured_metadata
         COMMAND cpp_hir_parser_core_record_ctor_metadata_test)
add_test(NAME cpp_hir_parser_core_producer_structured_metadata
         COMMAND cpp_hir_parser_core_producer_metadata_test)
add_test(NAME cpp_hir_parser_declarations_residual_structured_metadata
         COMMAND cpp_hir_parser_declarations_residual_metadata_test)
add_test(NAME cpp_hir_parser_expressions_residual_structured_metadata
         COMMAND cpp_hir_parser_expressions_residual_metadata_test)
add_test(NAME cpp_hir_parser_support_residual_structured_metadata
         COMMAND cpp_hir_parser_support_residual_metadata_test)
add_test(NAME cpp_hir_parser_type_base_residual_structured_metadata
         COMMAND cpp_hir_parser_type_base_residual_metadata_test)
add_test(NAME cpp_hir_parser_type_helper_structured_metadata
         COMMAND cpp_hir_parser_type_helper_metadata_test)
add_test(NAME cpp_hir_parser_type_helper_residual_structured_metadata
         COMMAND cpp_hir_parser_type_helper_residual_metadata_test)
add_test(NAME cpp_hir_template_pattern_match_structured_metadata
         COMMAND cpp_hir_template_pattern_match_metadata_test)
add_test(NAME cpp_hir_template_canonical_primary_origin_structured_metadata
         COMMAND cpp_hir_template_canonical_primary_origin_metadata_test)
add_test(NAME cpp_hir_template_realize_struct_structured_metadata
         COMMAND cpp_hir_template_realize_struct_metadata_test)
add_test(NAME cpp_hir_template_recover_identity_structured_metadata
         COMMAND cpp_hir_template_recover_identity_metadata_test)
add_test(NAME cpp_hir_value_args_residual_structured_metadata
         COMMAND cpp_hir_value_args_residual_metadata_test)
add_test(NAME frontend_lir_global_type_ref COMMAND frontend_lir_global_type_ref_test)
add_test(NAME frontend_lir_function_signature_type_ref
         COMMAND frontend_lir_function_signature_type_ref_test)
add_test(NAME frontend_lir_extern_decl_type_ref
         COMMAND frontend_lir_extern_decl_type_ref_test)
add_test(NAME frontend_lir_call_type_ref COMMAND frontend_lir_call_type_ref_test)
set_tests_properties(frontend_lexer_tests  PROPERTIES LABELS "internal;lex")
set_tests_properties(frontend_parser_tests PROPERTIES LABELS "internal;parse")
set_tests_properties(frontend_parser_lookup_authority_tests
                     PROPERTIES LABELS "internal;parse")
set_tests_properties(frontend_hir_lookup_tests PROPERTIES LABELS "internal;hir")
set_tests_properties(cpp_hir_template_type_arg_encoding_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_static_member_base_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_member_typedef_binding_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_nested_member_typedef_record_def_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_member_typedef_readiness_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_member_typedef_origin_binding_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_member_typedef_nested_readiness_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_member_typedef_realized_owner_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_core_nominal_typespec_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_core_typedef_chain_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_core_struct_like_typedef_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_core_record_ctor_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_core_producer_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_declarations_residual_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_expressions_residual_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_support_residual_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_type_base_residual_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_type_helper_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_parser_type_helper_residual_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_template_pattern_match_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_template_canonical_primary_origin_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_template_realize_struct_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_template_recover_identity_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(cpp_hir_value_args_residual_structured_metadata
                     PROPERTIES LABELS "internal;hir;cpp")
set_tests_properties(frontend_lir_global_type_ref PROPERTIES LABELS "internal;hir;lir")
set_tests_properties(frontend_lir_function_signature_type_ref PROPERTIES LABELS "internal;hir;lir")
set_tests_properties(frontend_lir_extern_decl_type_ref PROPERTIES LABELS "internal;hir;lir")
set_tests_properties(frontend_lir_call_type_ref PROPERTIES LABELS "internal;hir;lir")
