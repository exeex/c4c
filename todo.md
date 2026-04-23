Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Completed plan step 2's remaining simple statement-condition packet for
`while` and `switch` declaration probes. Added lexical-scope-backed condition
declaration parsing for those statement forms, verified the synthetic block
wrapping keeps the local binding alive only through the parsed statement body,
and added focused parser-debug regressions for their tentative probe traces.

## Suggested Next
If no other statement-shaped declaration probes remain, move from step 2's
scope-lifetime packet to the first step 5 cleanup slice for string-backed
`concept_names` and other single-name lexical holdouts.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84. Condition-declaration
scope should only stay alive for a successfully parsed declaration; failed
tentative parses must still unwind cleanly without leaving lexical bindings
behind. `while` and `switch` now mirror the earlier `if` behavior through a
synthetic block wrapper, so future statement-condition cleanup should preserve
that lifetime contract instead of falling back to parser-wide flat tables.
Treat string-backed `concept_names` cleanup as a separate packet, not an
extension of this statement-scope slice.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parser_debug_if_condition_decl_tentative_lite|cpp_parser_debug_range_for_tentative_lite|cpp_parser_debug_while_condition_decl_tentative_lite|cpp_parser_debug_switch_condition_decl_tentative_lite)$'`
with proof captured in `test_after.log`.
