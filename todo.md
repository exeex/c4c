Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Re-run focused parser proof and widen only if the blast radius grows

# Current Packet

## Just Finished
Completed the step-6 cleanup by keeping the visible type/value helpers
`TextId`-first while restoring the namespace-local declaration path that was
misclassified by the concept gate. The bridge-string fallback remains
compatibility-only at the tail of the lookup helpers.

## Suggested Next
Run the step-7 proof with broader parser/frontend coverage before treating the
route as finished.

## Watchouts
Keep `known_fn_names` and `struct_typedefs` on their structured path. The
concept gate must not fall back to treating visible typedefs as concepts again.
The earlier narrow subset passed, but it was not sufficient for the current
diff breadth.

## Proof
Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_namespace_struct_type_basic_cpp|cpp_positive_sema_record_member_enum_parse_cpp)$'` and wrote the combined output to `test_after.log`.
The subset passed, but the review noted that broader parser/frontend proof is
still needed before this plan can be accepted as complete.
