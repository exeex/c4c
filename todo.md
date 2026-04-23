Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Completed plan step 2's `for`-statement local-scope packet: the parser now
opens a lexical binding scope around declaration-bearing `for` forms, keeps
init-declaration bindings alive through the loop condition/update/body, and
binds range-for loop variables only for the loop body before popping the scope
at statement exit. Added parser regressions for both init declarations and
range-for variables that verify the loop parses and the binding does not leak
after the statement.

## Suggested Next
Continue step 2 by auditing any remaining statement-shaped declaration forms
that still bypass `ParserLexicalScopeState`, then move on to concept-name
cleanup once the statement-scope coverage is stable.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84. Condition-declaration
scope should only stay alive for a successfully parsed declaration; failed
tentative parses must still unwind cleanly without leaving lexical bindings
behind. Range-for bindings should stay loop-lifetime local and should not leak
past the closing `)`/body boundary. Treat string-backed `concept_names` cleanup
as a later packet, not part of this scope-lifetime slice.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
with proof captured in `test_after.log`.
