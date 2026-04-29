Status: Active
Source Idea Path: ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Quarantine String Overloads and AST Projection Bridges

# Current Packet

## Just Finished

Step 4 repair of prior Step 3 fallout completed after the rejected baseline.
The rejected baseline exposed failures in qualified out-of-class method owner
selection, qualified current self-type parsing, the EASTL tuple forward-decl
recipe timeout, std-vector progress tests, and the forward-declared sibling
negative case.

`lookup_type_in_context` now preserves the Step 3 structured-primary rule for
normal token-backed namespace lookup, but restores a narrow rendered-type
projection when the projected typedef is a record type. Outside an active
record body, incomplete record projections remain allowed for forward-declared
template records such as EASTL tuple declarations; inside an active record body,
only complete records use that projection. `resolve_visible_type` also has an
active-record-only projection for current self-type spelling and incomplete
same-namespace sibling record tags inside a record body, so qualified self-type
spellings compare correctly while incomplete sibling forward declarations still
reach the existing incomplete object check. The broad
`qualified_key_in_context(..., create_missing_path=false)` rendered-spelling
fallback remains removed; no narrower key fallback was needed.

## Suggested Next

Step 4 is ready for supervisor validation/review with the rejected-baseline
fallout repaired.

## Watchouts

The restored fallback is intentionally not a general compatibility-spelling
authority path. Arbitrary rendered typedefs with valid TextIds remain rejected;
the repaired paths are record-type projection for namespace/type lookup,
active-record self projection, and active-record incomplete sibling projection
needed for C++ class parsing and incomplete-type diagnostics.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|cpp_eastl_tuple_fwd_decls_parse_recipe|cpp_positive_sema_qualified_namespaced_out_of_class_method_context_frontend_cpp|cpp_positive_sema_forward_declared_self_type_local_qualified_parse_cpp|cpp_parse_forward_declared_self_type_local_qualified_dump|cpp_negative_tests_bad_forward_declared_other_local_object|cpp_std_vector_parse_debug_progress|cpp_std_vector_parse_debug_progress_file' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`. The EASTL tuple recipe passed
in the repaired subset with the broad `qualified_key_in_context` rendered
fallback still removed.
