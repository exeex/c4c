Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by teaching the parser's
parameter-type probe about unresolved parenthesized declarators such as
`Value(other)` while still biasing known visible values like
`source(other)` toward direct-initialization expressions. The local
ctor-init/function-declaration probe now routes unresolved call-like
starters through the same declaration-first path as other unresolved
parameter-type shapes instead of treating every `identifier(...)`
starter as a value expression.

## Suggested Next
Continue `plan.md` step 2 by auditing the remaining constructor-init probe
cases that still rely on local token-shape disambiguation for unresolved
identifier starters, especially nested parenthesized pointer/reference
declarators and any other forms that should consult visible lexical
type-or-value bindings before falling back to unresolved declaration
shape heuristics.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. The constructor-init
probe now special-cases unresolved `identifier identifier`,
`identifier &/* identifier`, and unresolved parenthesized
`identifier(...)` starters differently; future cleanup should keep that
value-vs-type bias local to direct-init disambiguation instead of
re-expanding it into namespace-qualified lookup.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
