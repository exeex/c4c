Status: Active
Source Idea Path: ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured Lookup and Disambiguation Paths

# Current Packet

## Just Finished

Completed the out-of-class constructor Plan Step 2 packet. The qualified
constructor declaration path now records owner segment `TextId`s and global
qualification while consuming the owner, then registers a structured
`QualifiedNameKey` for the constructor before any rendered
`qualified_ctor_name` fallback. `qualified_ctor_name` and `fn->name` remain
final spelling/display data.

## Suggested Next

Step 3 next packet: convert lookup/disambiguation sites that still consult
rendered `head_name`, `current_member_name`, or visible-name spelling so they
probe structured known-function keys first when parser identity is available.
Keep string lookup as an explicit compatibility fallback and add fallback
visibility where mismatches remain diagnosable.

## Watchouts

- Keep source-idea intent stable; routine findings belong in this file.
- Do not let rendered spelling decide disambiguation when structured identity
  is available.
- Avoid testcase-shaped special cases.
- Do not remove `fn->name` rendered spelling yet; it is final AST/display data,
  not the known-name authority store.
- This packet intentionally did not fold in Step 3 lookup/disambiguation work.

## Proof

Passed: `cmake --build build --target c4cll frontend_parser_tests >
test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R
'^(frontend_parser_tests|cpp_positive_sema_constructor_basic_cpp|cpp_positive_sema_constructor_self_ref_param_parse_cpp|cpp_positive_sema_default_ctor_basic_cpp|cpp_positive_sema_delegating_ctor_basic_cpp|cpp_positive_sema_cpp20_out_of_class_trailing_requires_runtime_cpp|cpp_positive_sema_ctor_init_out_of_class_dependent_typename_index_tuple_parse_cpp|cpp_positive_sema_ctor_init_dependent_typename_index_tuple_parse_cpp|cpp_positive_sema_ctor_init_delegating_unqualified_template_runtime_cpp|cpp_positive_sema_ctor_init_member_typedef_ctor_runtime_cpp|cpp_parse_constructor_self_ref_param_dump|cpp_parser_debug_ctor_init_tentative_lite|cpp_negative_tests_parser_debug_ctor_init_tentative_lite)$'
>> test_after.log 2>&1`. The proof rebuilt `c4cll` and
`frontend_parser_tests`, then passed 13/13 selected tests. Proof log:
`test_after.log`.
