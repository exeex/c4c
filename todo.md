Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Moved the first real unqualified local typedef/value registration packet onto
`ParserLexicalScopeState`: unqualified names registered while a local binding
scope is active now bind only into the lexical scope tables instead of leaking
into the flat parser tables, and local user-typedef membership now rides a
scope-local side table so same-scope conflict checks still work. Added parser
regressions that prove local registrations stay visible through the lexical
facade inside scope, disappear after pop, and no longer bias later ctor-init
ambiguity probes after an inner block ends.

## Suggested Next
Continue step 2 by auditing the remaining local declaration forms that still
skip lexical push/pop or rely on flat tables for visibility, with
if-condition declarations and other non-block local scopes as the next likely
packet before any concept-name cleanup.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84. The first migration
packet should stay on unqualified local typedef/value visibility and treat the
string-backed `concept_names` cleanup as a follow-on once the scope-local
typedef/value route is proven.

## Proof
Ran `cmake --build --preset default` and
`ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`.
