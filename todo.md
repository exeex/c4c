Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce parser lexical scope state for the simplest local bindings

# Current Packet

## Just Finished
Advanced `plan.md` step 2 by teaching the local ctor-init/function-declaration
probe to classify qualified starters such as `ns::value(...)` and
`ns::value<T>(...)` before the generic parameter-list path. The direct-init
probe now keeps unresolved qualified starters on the declaration/type side
while still biasing known qualified visible values and function templates
toward constructor-style expression parsing.

## Suggested Next
Continue `plan.md` step 2 by checking whether the remaining local
declaration/expression ambiguity probes can reuse the same qualified
value-vs-type split without pulling namespace traversal back into the lexical
scope path. Focus on `using`-imported or global-qualified starter forms only
if they still bypass the structured helper.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. The shared helper
is only safe for declaration/type-head probes; expression-side heuristics must
still distinguish value bindings from type bindings. The constructor-init
probe now special-cases unresolved `identifier identifier`,
`identifier &/* identifier`, simple parenthesized `identifier(...)`, and
grouped pointer/reference `identifier((...))` starters differently, and it now
also short-circuits both unqualified and qualified value-like starters only
when the head resolves through visible value or known-function lookup. Future
cleanup should keep that value-vs-type bias local to direct-init
disambiguation instead of re-expanding it into namespace-qualified lookup.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_cxx_)' | tee test_after.log` passed. Proof log: `test_after.log`.
