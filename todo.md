Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Completed plan step 2's next non-block local-scope packet for `if` condition
declarations: the parser now opens a dedicated lexical binding scope only for
successful condition declarations, keeps the declared name visible while the
then/else arms parse, and drops that scope when the statement finishes instead
of leaking the binding into the flat parser tables. Added parser regressions
that prove the synthetic condition block still forms correctly and that the
condition-declared value no longer remains visible after the `if` statement.

## Suggested Next
Continue step 2 by auditing the remaining non-block local declaration forms
that still bypass `ParserLexicalScopeState`, especially other statement-shaped
scopes that may need lexical push/pop coverage before moving on to concept-name
cleanup.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84. Condition-declaration
scope should only stay alive for a successfully parsed declaration; failed
tentative parses must still unwind cleanly without leaving lexical bindings
behind. Treat string-backed `concept_names` cleanup as a later packet, not part
of this scope-lifetime slice.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
with proof captured in `test_after.log`.
