Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Reduce legacy lexical bridge lookup to compatibility-only support

# Current Packet

## Just Finished
Repaired the step-6 regression by keeping the visible type/value helpers
`TextId`-first while restoring the namespace-local declaration path that was
misclassified by the concept gate. The bridge-string fallback remains
compatibility-only at the tail of the lookup helpers.

## Suggested Next
Continue with the remaining step-6 cleanup only if a later packet needs to
trim any other legacy bridge path; this slice is complete.

## Watchouts
Keep `known_fn_names` and `struct_typedefs` on their structured path. The
concept gate must not fall back to treating visible typedefs as concepts again.

## Proof
Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_namespace_struct_type_basic_cpp|cpp_positive_sema_record_member_enum_parse_cpp)$'` and wrote the combined output to `test_after.log`.
The subset passed.
