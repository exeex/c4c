Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by tightening the block-scope
ctor-init/function-declaration probe in `parser_declarations.cpp` so
unresolved named-parameter forms like `Box copy(Value other);` still classify
as local function declarations, while unresolved value-expression starters
like `Box value(source & other);` stay on the direct-initialization path
instead of being over-classified as parameter declarations.

## Suggested Next
Continue `plan.md` step 2 by auditing the remaining constructor-init probe
cases that still rely on local token-shape disambiguation for unresolved
identifier starters, especially pointer/reference and call-like forms that
may still diverge from the parser-visible lexical typedef facade.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. The constructor-init
probe now special-cases unresolved `identifier identifier` and unresolved
`identifier &/* identifier` starters differently; future cleanup should keep
that value-vs-type bias local to direct-init disambiguation instead of
re-expanding it into namespace-qualified lookup.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' | tee test_after.log` passed. Proof log: `test_after.log`.
