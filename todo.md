Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Completed plan step 2's parser regression-fix packet: `parse_stmt` now forces
member-access statements that continue with `.` or `->` down the expression
path instead of misclassifying them as declarations. Added a parser regression
covering a visible type head followed by member access, while keeping the
previous `for` init-declaration and range-for lexical-scope coverage intact.

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
The member-access guard is intentionally narrow to `.` / `->` continuations;
keep future declaration-vs-expression fixes from regressing the existing
qualified-call and loop-scope coverage.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_eastl_vector_parse_recipe)$'`
with proof captured in `test_after.log`.
