Status: Active
Source Idea Path: ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured Registration Paths

# Current Packet

## Just Finished

Completed the out-of-class operator Plan Step 2 packet. The qualified operator
declaration path now records owner segment `TextId`s while consuming the owner
and registers a structured `QualifiedNameKey` for the generated operator base
name before any rendered `qualified_op_name` fallback. `qualified_op_name` and
`fn->name` remain final spelling/display data.

## Suggested Next

Step 2 next packet: convert the out-of-class constructor declaration
registration path away from rendered `qualified_ctor_name` authority while
preserving final display spelling.

## Watchouts

- Keep source-idea intent stable; routine findings belong in this file.
- Do not let rendered spelling decide disambiguation when structured identity
  is available.
- Avoid testcase-shaped special cases.
- Out-of-class constructor registration is still a follow-up packet; it needs
  owner-name structure preserved instead of reparsing `qualified_ctor_name`.
- Do not remove `fn->name` rendered spelling yet; it is final AST/display data,
  not the known-name authority store.

## Proof

Passed: `cmake --build build --target c4cll frontend_parser_tests >
test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R
'^(frontend_parser_tests|cpp_positive_sema_operator_conversion_out_of_class_parse_cpp|cpp_positive_sema_operator_conversion_template_id_refqual_parse_cpp|cpp_positive_sema_qualified_operator_template_owner_parse_cpp|cpp_positive_sema_operator_decl_bool_parse_cpp|cpp_positive_sema_operator_decl_shift_qualified_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_operator_arrow_explicit_member_call_parse_cpp|cpp_positive_sema_free_operator_eq_overload_frontend_cpp|cpp_positive_sema_friend_inline_operator_parse_cpp|cpp_positive_sema_friend_relational_operator_parse_cpp|cpp_positive_sema_friend_spaceship_operator_parse_cpp|cpp_positive_sema_member_spaceship_operator_parse_cpp)$'
>> test_after.log 2>&1`. The proof rebuilt `c4cll` and
`frontend_parser_tests`, then passed 13/13 selected tests. Proof log:
`test_after.log`.
