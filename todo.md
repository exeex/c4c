Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Completed the remaining step 2 parser statement-scope audit for the simple
`if` condition declaration probe and the simple `for`/range-for declaration
probes. Verified the tentative paths keep lexical binding lifetime bounded to
successful parses and that the targeted parser regressions still pass.

## Suggested Next
Continue step 2 by checking any remaining statement-shaped declaration probes
that still rely on tentative parsing, then move on to concept-name cleanup once
statement-scope coverage is stable.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84. Condition-declaration
scope should only stay alive for a successfully parsed declaration; failed
tentative parses must still unwind cleanly without leaving lexical bindings
behind. Range-for bindings should stay loop-lifetime local and should not leak
past the closing `)`/body boundary. Treat string-backed `concept_names` cleanup
as a later packet, not part of this scope-lifetime slice.
The new expression-routing branch is intentionally limited to visible value
heads with call-like suffixes; keep future declaration-vs-expression fixes from
regressing the existing qualified-call, member-access, and loop-scope coverage.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_for_decl_probe_rollback_parse_cpp|cpp_parser_debug_if_condition_decl_tentative_lite|cpp_parser_debug_range_for_tentative_lite)$'`
with proof captured in `test_after.log`.
